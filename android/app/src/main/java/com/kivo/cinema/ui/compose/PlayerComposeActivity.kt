package com.kivo.cinema.ui.compose

import android.media.AudioManager
import android.os.Bundle
import android.view.SurfaceHolder
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.foundation.layout.padding
import com.kivo.cinema.bridge.FfmpegRouting
import com.kivo.cinema.bridge.PlaybackSession
import com.kivo.cinema.ui.PlayerSurfaceView

/**
 * Compose 播放页宿主（213）。
 *
 * 与旧 [com.kivo.cinema.ui.PlayerActivity] 并存灰度（设计稿 D5：`kivo.ui=compose`）。
 * 会话生命周期复用 [PlaybackSession]——native 桥一行不动。
 * 视频层走 AndroidView(PlayerSurfaceView)——零拷贝路径一行不动（设计稿 D2）。
 *
 * M1 明确不做：字幕叠加的 Compose 集成（Android 侧 SubtitleOverlayView 仍可用，
 * Compose 集成后续卡）、音频分离控制、横竖屏切换。
 */
class PlayerComposeActivity : ComponentActivity() {

    private var playbackSession: PlaybackSession? = null
    private var surfaceReady = false
    private var playbackStarted = false

    private var mediaOverridePath = ""
    private var evidenceName = ""
    private var thumbnailReader: com.kivo.cinema.thumbnail.ThumbnailReader? = null
    private var stressDone = false

    // Compose 状态（D4：控制层 + 播放状态）
    private var isPlaying by mutableStateOf(false)
    private var controlsVisible by mutableStateOf(true)
    private var positionMs by mutableStateOf(0L)
    private var durationMs by mutableStateOf(0L)
    private var statusText by mutableStateOf("starting...")
    private var thumbnailBitmap by mutableStateOf<android.graphics.Bitmap?>(null)
    private var previewMs by mutableStateOf(-1L)
    private var danmakuOn by mutableStateOf(false)
    private var danmakuItems by mutableStateOf(listOf<com.kivo.cinema.danmaku.DanmakuItem>())

    private val positionPoll = object : Runnable {
        override fun run() {
            // 软解分支放最前：它与网络会话互斥，先判掉就不必让下面每条分支都写一次 `if`。
            val soft = smbSoftSession
            if (soft != null) {
                val dur = soft.durationUs()
                if (dur > 0) durationMs = dur / 1000
                val pos = soft.positionUs()
                if (pos >= 0) {
                    positionMs = (pos / 1000).let { if (durationMs > 0) it.coerceAtMost(durationMs) else it }
                    // 端到端速率实测。会话内部的 decode_ms/wall_us 记的是解码线程自己，
                    // 用户看到的"卡"还要叠加渲染、闸门与主线程这一层，只有站在播放页
                    // 量"媒体时间 / 墙钟时间"才是用户视角的那个数。
                    val nowMs = System.currentTimeMillis()
                    // 音频侧同一时刻的两个读数：`played` 是真正在响的位置，
                    // `position` 是解码写环的位置。两者之差就是环里排队的量。
                    // 只打 position 会把"排队的"误当成"已响的"，音画对齐永远差几百毫秒。
                    val aPlayed = smbSoftAudio?.playedUs() ?: -1L
                    // 音频出声之后，把它交给视频会话做主时钟：视频从此贴着声音走
                    // （超前就睡、落后就丢帧），不再各自守时。
                    // 音频还没出声时推 -1，让视频先按自身节拍跑起来，不必干等。
                    soft.setAudioClockUs(aPlayed)
                    if (softRateMarkMs == 0L) {
                        softRateMarkMs = nowMs
                        softRateMarkPosUs = pos
                        softAudioMarkUs = aPlayed
                    } else if (nowMs - softRateMarkMs >= 2000) {
                        val dMediaUs = pos - softRateMarkPosUs
                        val dWallUs = (nowMs - softRateMarkMs) * 1000
                        val ratio = if (dWallUs > 0) dMediaUs.toDouble() / dWallUs else 0.0
                        // 音频也要单独量速率。若 arate 不是 1.00，说明声音自己在快放/慢放，
                        // 那么无论视频怎么对齐都会越走越偏 —— 这是与"视频落后"完全不同的病因。
                        val dAudioUs = if (aPlayed >= 0 && softAudioMarkUs >= 0) {
                            aPlayed - softAudioMarkUs
                        } else -1L
                        val aRate = if (dAudioUs > 0 && dWallUs > 0) {
                            dAudioUs.toDouble() / dWallUs
                        } else 0.0
                        // **音画偏移**：正在响的声音位置 − 正在显示的画面前沿。
                        // 正值 = 声音超前于画面；负值 = 声音落后。人耳容忍约 ±40 ms。
                        val avDiffUs = if (aPlayed >= 0) aPlayed - pos else 0L
                        smbTrace("soft rate media=+${dMediaUs / 1000}ms wall=+${dWallUs / 1000}ms " +
                            "ratio=${"%.2f".format(java.util.Locale.US, ratio)} " +
                            "arate=${"%.2f".format(java.util.Locale.US, aRate)} " +
                            "avdiff=${avDiffUs / 1000}ms gated=$smbSoftAudioGated")
                        softRateMarkMs = nowMs
                        softRateMarkPosUs = pos
                        softAudioMarkUs = aPlayed
                    }
                }
                // 极端情况兜底闸门（**不再是常规同步手段**）。
                //
                // 常规同步已经交给"音频主时钟 + 视频丢帧/等待"（见 setAudioClockUs）。
                // 这里只保留一跳保险：视频彻底解不动时（例如设备完全不支持、
                // 单帧要几百毫秒），音频会一路跑到前面几十秒，那时必须停下来等，
                // 否则声音和画面差出分钟级，比"声音断一下"糟糕得多。
                //
                // 阈值从原来的 800 ms 提到 5 s：800 ms 是过去"各自守时"时代用来凑数的，
                // 现在视频会主动追音频，再按 800 ms 去停流只会打断正在对齐的过程，
                // 让音频时钟冻结、视频又追上来，变成来回振荡。
                val audioSync = smbSoftAudio
                if (audioSync != null && isPlaying && pos >= 0) {
                    val audioPos = audioSync.positionUs()
                    if (audioPos >= 0) {
                        val leadUs = audioPos - pos
                        if (!smbSoftAudioGated && leadUs > 5_000_000) {
                            audioSync.setPaused(true)
                            smbSoftAudioGated = true
                            smbTrace("soft audio gated: lead=$leadUs us")
                        } else if (smbSoftAudioGated && leadUs < 1_000_000) {
                            audioSync.setPaused(false)
                            smbSoftAudioGated = false
                            smbTrace("soft audio resumed: lead=$leadUs us")
                        }
                    }
                }
                if (!soft.isRunning()) {
                    // 线程退出只有两种结局：跑到 EOS，或读/解失败。位置是否到头是唯一
                    // 客户端可分辨的判据——不区分就会把"播完了"报成"中断"。
                    statusText = if (durationMs > 0 && positionMs >= durationMs - 1000) "播放结束" else "软解中断"
                    isPlaying = false
                    controlsVisible = true
                    return
                }
                if (System.currentTimeMillis() - lastTouchMs > 3000) controlsVisible = false
                android.os.Handler(mainLooper).postDelayed(this, 200)
                return
            }
            val mpvSession = mpvNetSession
            if (mpvSession != null) {
                // 253-P3：libmpv 内核的状态回显。与旧内核同一套字段语义。
                if (mpvSession.durationUs > 0) {
                    durationMs = mpvSession.durationUs / 1000
                }
                positionMs = (mpvSession.positionUs / 1000).coerceAtLeast(0)
                    .let { if (durationMs > 0) it.coerceAtMost(durationMs) else it }
                isPlaying = mpvSession.state == com.kivo.cinema.playback.MpvPlaybackSession.State.PLAYING
                when (mpvSession.state) {
                    com.kivo.cinema.playback.MpvPlaybackSession.State.FAILED -> {
                        statusText = "网络播放失败：${mpvSession.lastError}"
                        controlsVisible = true
                        return
                    }
                    com.kivo.cinema.playback.MpvPlaybackSession.State.ENDED -> {
                        statusText = "播放结束"
                        controlsVisible = true
                        return
                    }
                    else -> {
                        if (statusText.startsWith("网络中断")) {
                            statusText = "正在播放"
                        }
                    }
                }
                // 【必须重新排期】
                // 这一支是独立的轮询回路 —— 下面的 `postDelayed` 在网络分支里，
                // 提前 return 会跳过它，位置就永远停在 0（真机实测：画面在动、
                // 时长也对，但进度一直是 0:00:00）。
                //
                // 【控制条自动隐藏 —— mpv 分支原先漏了】
                // 软解分支与旧网络分支都有"3 秒无操作自动隐藏控制条"，
                // mpv 分支没有 ⇒ 控制条**永远显示**。
                // 后果不是"不好看"而是**遮住字幕**：控制条横在画面底部，
                // 而字幕正好在底部（真机取证：字幕已被渲染进 OSD 纹理，
                // 截图里却被控制条完全盖住，一度被误判成"字幕没渲染"）。
                if (System.currentTimeMillis() - lastTouchMs > 3000) {
                    controlsVisible = false
                }
                android.os.Handler(mainLooper).postDelayed(this, 100)
                return
            }
            val netSession = smbNetSession
            if (netSession != null) {
                durationMs = netSession.durationUs / 1000
                positionMs = (netSession.positionUs / 1000).coerceAtLeast(0)
                    .let { if (durationMs > 0) it.coerceAtMost(durationMs) else it }
                isPlaying = netSession.state == com.kivo.cinema.playback.NetworkPlaybackSession.State.PLAYING
                when (netSession.state) {
                    com.kivo.cinema.playback.NetworkPlaybackSession.State.FAILED -> {
                        statusText = "网络播放失败：${netSession.lastError}"
                        controlsVisible = true
                        return
                    }
                    com.kivo.cinema.playback.NetworkPlaybackSession.State.ENDED -> {
                        statusText = "播放结束"
                        controlsVisible = true
                        return
                    }
                    else -> {
                        when {
                            // 断网重连是**暂时**状态：画面停在最后一帧，明确告诉用户在等网络，
                            // 而不是让状态文字一直显示"正在播放"（用户会以为卡死了）。
                            netSession.buffering -> {
                                statusText = "网络中断，正在重连…"
                                controlsVisible = true
                            }
                            netSession.lastError.isNotBlank() -> {
                                statusText = netSession.lastError
                                controlsVisible = true
                            }
                            else -> {
                                // 重连成功后把状态文字收回来，否则会一直挂着"正在重连…"。
                                if (statusText.startsWith("网络中断")) {
                                    statusText = if (netSession.hasAudio) "正在播放"
                                    else "正在播放 · 片源没有音轨"
                                }
                                if (System.currentTimeMillis() - lastTouchMs > 3000) {
                                    controlsVisible = false
                                }
                            }
                        }
                    }
                }
                android.os.Handler(mainLooper).postDelayed(this, 100)
                return
            }
            val session = playbackSession ?: return
            // EOS 后时钟外推会越过片长（真机实测 205s/60s 片）——clamp 到时长
            positionMs = (session.positionUs() / 1000).coerceAtMost(durationMs)
            // 244 取证：每秒打一次原生会话的帧账与时间戳。
            // 「位置恒 0」到底是没解出帧，还是有帧但 pts 全 0 —— 只有这行能分开。
            if (com.kivo.cinema.BuildConfig.DEBUG) {
                val now = System.currentTimeMillis()
                if (now - lastSummaryLogMs >= 1000) {
                    lastSummaryLogMs = now
                    Log.i(TAG, "summary: ${runCatching { session.summary() }.getOrElse { "n/a" }}")
                }
            }
            // 3s 无操作自动隐藏（D4）；预览窗口期间不隐藏（取证 harness 需要）
            if (previewMs < 0 && System.currentTimeMillis() - lastTouchMs > 3000) {
                controlsVisible = false
            }
            android.os.Handler(mainLooper).postDelayed(this, 100)
        }
    }
    private var lastTouchMs = 0L
    /** 244 取证用：上次打印 summary 的时间（每秒一次）。 */
    private var lastSummaryLogMs = 0L

    companion object {
        private const val TAG = "KivoCompose"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        readProbeExtras()
        android.os.Handler(mainLooper).post(watchPoll)   // 237-A：观看数据域轮询

        setContent {
            MaterialTheme(colorScheme = darkColorScheme()) {
                Box(modifier = Modifier.fillMaxSize().background(Color.Black)) {
                    PlayerScreen(
                        onSurfaceReady = { holder -> maybeStartPlayback(holder) },
                        onSurfaceLost = { onSurfaceLost() },
                        onTap = {
                            lastTouchMs = System.currentTimeMillis()
                            controlsVisible = !controlsVisible
                        },
                        onDoubleTap = {
                            lastTouchMs = System.currentTimeMillis()
                            togglePlayPause()
                        },
                    )
                    // 诊断状态（M1 临时：K40 logcat 不可用的实时替代）
                    androidx.compose.material3.Text(
                        text = statusText,
                        color = Color.White,
                        fontSize = androidx.compose.ui.unit.TextUnit(10f, androidx.compose.ui.unit.TextUnitType.Sp),
                        modifier = Modifier.align(androidx.compose.ui.Alignment.TopStart)
                            .background(Color.Black.copy(alpha = 0.7f))
                            .padding(4.dp)
                    )
                    if (controlsVisible) {
                        PlayerControls(
                            isPlaying = isPlaying,
                            positionMs = positionMs,
                            durationMs = durationMs,
                            onPlayPause = { togglePlayPause() },
                            danmakuVisible = danmakuOn && danmakuItems.isNotEmpty(),
                            onDanmakuToggle = { danmakuOn = !danmakuOn },
                            casting = castDevice != null,
                            onCastClick = { openCastSheet() },
                            onSeek = { ms ->
                                val soft = smbSoftSession
                                when {
                                    soft != null -> {
                                        soft.seekTo(ms * 1000)
                                        // 声音是独立链路，必须一起定位，否则画面对了声音还是旧的。
                                        smbSoftAudio?.seekTo(ms * 1000)
                                        positionMs = ms
                                    }
                                    mpvNetSession != null -> mpvNetSession?.seekTo(ms * 1000)
                                    smbNetSession != null -> smbNetSession?.seekTo(ms * 1000)
                                    else -> playbackSession?.seekTo(ms * 1000)
                                }
                            },
                            onScrub = { ms ->
                                lastTouchMs = System.currentTimeMillis()
                                thumbnailReader?.seekTo(ms * 1000)
                            },
                            thumbnailBitmap = thumbnailBitmap,
                            previewMs = previewMs,
                            modifier = Modifier.align(androidx.compose.ui.Alignment.BottomCenter),
                        )
                    }
                    // 弹幕层（223）：Canvas overlay 在最顶层（Box 最后一个子元素）
                    com.kivo.cinema.danmaku.DanmakuOverlay(
                        items = danmakuItems,
                        positionMs = positionMs,
                        visible = danmakuOn,
                        modifier = Modifier.fillMaxSize()
                    )
                    // 投屏设备 sheet（230）：置于弹幕之上（Box 最后一个子元素）
                    com.kivo.cinema.dlna.CastSheet(
                        visible = castSheetVisible,
                        scanning = castScanning,
                        devices = castDevices,
                        message = castMessage,
                        onSelect = { onCastSelect(it) },
                        onRescan = { openCastSheet() },
                        onDismiss = { castSheetVisible = false },
                    )
                }
            }
        }
    }

    // PlayerSurfaceView.Listener 的回调（由 AndroidView 工厂里的匿名 Listener 转发）
    fun onSurfaceAvailable(holder: SurfaceHolder) {
        if (finishRequested || isDestroyed) return
        surfaceReady = true
        maybeStartPlayback(holder)
    }

    fun onSurfaceLost() {
        surfaceReady = false
        val network = smbNetSession
        if (network?.state == com.kivo.cinema.playback.NetworkPlaybackSession.State.PREPARING ||
            networkPreparationCancelled != null) {
            closeNetworkPlayback()
            playbackStarted = false
            isPlaying = false
        } else {
            network?.rebind(null)
            playbackSession?.rebind(null)
        }
    }

    private fun maybeStartPlayback(holder: SurfaceHolder) {
        if (finishRequested || isDestroyed) return
        // surface 回调本身就是"面可用"的证明——不再依赖 Listener 的 surfaceReady 标志
        surfaceReady = true
        if (playbackStarted) {
            mpvNetSession?.rebind(holder.surface)
            smbNetSession?.rebind(holder.surface)
            playbackSession?.rebind(holder.surface)
            return
        }
        val smbSourceId = intent.getStringExtra("kivo.sourceId")
        val smbPath = intent.getStringExtra("kivo.smbPath")
        if (!smbSourceId.isNullOrBlank() && !smbPath.isNullOrBlank()) {
            startSmbPlayback(holder, smbSourceId, smbPath, intent.getStringExtra("kivo.smbShare").orEmpty())
            return
        }

        // 220 AC5：通用 URL 直开（file:// 本地对照 / http:// 本地代理）——
        // 与 SMB 路径同会话类同解码器，传输层唯一变量。
        val playUrl = intent.getStringExtra("kivo.playurl")
        if (playUrl != null) {
            startUrlPlayback(holder, playUrl,
                intent.getStringExtra("kivo.label") ?: playUrl,
                intent.getStringExtra("kivo.routing") ?: "file")
            return
        }

        val media = mediaOverridePath.takeIf { it.isNotBlank() }
            ?.let { java.io.File(it) }?.takeIf { it.isFile }
            ?: locateTestMedia() ?: return
        playbackStarted = true

        statusText = "surface ok, probing..."
        val routing = FfmpegRouting.probe(media)
        statusText = "route=${routing.decision} reason=${routing.reason}"
        Log.i(TAG, "routing: ${routing.decision} (${routing.reason})")

        val localSession = PlaybackSession(0, media, media.name)
        playbackSession = localSession

        statusText = "openVideo..."
        val rc = localSession.openVideo(holder.surface)
        statusText = "openVideo rc=$rc lastError=${localSession.lastError}"
        if (rc != 0) {
            Log.e(TAG, "openVideo failed rc=$rc: ${localSession.lastError}")
            localSession.close()
            playbackSession = null
            finish()
            return
        }

        // Audio must start before pacing so native selects the existing audio master clock.
        val audioProbe = com.kivo.cinema.bridge.MediaTrackProbe.inspectAudioTrack(media)
        if (audioProbe.isFailure) {
            statusText = "无法检查音轨，已停止播放：${audioProbe.exceptionOrNull()?.javaClass?.simpleName}"
            localSession.close()
            playbackSession = null
            controlsVisible = true
            return
        }
        val hasAudio = audioProbe.getOrThrow()
        if (hasAudio) {
            val audioRc = localSession.openAudio()
            if (audioRc != 0) {
                statusText = "音频无法启动：${localSession.lastError.ifBlank { "错误码 $audioRc" }}"
                localSession.close()
                playbackSession = null
                controlsVisible = true
                return
            }
        }

        // 节拍（字幕 position_us 的时间源；不加节拍时 position_us 恒 -1）
        localSession.configurePacing(targetFps = 0, allowDrop = true)
        localSession.configurePipeline(split = false, present = true, atTime = true)

        statusText = "starting..."
        val startRc = localSession.start()
        statusText = "start rc=$startRc"
        if (startRc != 0) {
            Log.e(TAG, "start failed rc=$startRc")
            localSession.close()
            playbackSession = null
            finish()
            return
        }
        isPlaying = true
        lastTouchMs = System.currentTimeMillis()
        android.os.Handler(mainLooper).post(positionPoll)
        // 时长：一次性 extractor 探测（215 拖动预览需要；顺带修掉控制条时长恒 0:00:00 的 213 缺口）
        durationMs = probeDurationUs(media.absolutePath) / 1000
        statusText = if (hasAudio) "正在播放" else "正在播放 · 片源没有音轨"

        // 215：独立缩略图读取器（懒初始化；与主会话完全解耦）
        val reader = com.kivo.cinema.thumbnail.ThumbnailReader(media.absolutePath)
        thumbnailReader = reader
        reader.onThumbnailReady = { _, bitmap ->
            android.os.Handler(mainLooper).post { thumbnailBitmap = bitmap }
        }

        // 215 取证 harness：`kivo.evidence=p215` 时自动跑 60 点扫描 + 主链路对照
        if (evidenceName == "p215") scheduleP215Stress()

        // 214：会话所有权转移给 Foreground Service——Activity 销毁后会话不死。
        com.kivo.cinema.playback.PlaybackSessionHolder.offer(localSession)
        startForegroundService(
            android.content.Intent(this, com.kivo.cinema.playback.PlaybackService::class.java)
        )
        Log.i(TAG, "playback started: ${media.absolutePath} (service adopted)")
    }

    // ---- 243：SMB 片源播放（libsmb2 → SmbByteSource → RangeFd → 解码器 fd）----
    // 传输层由 RangeFd + SmbByteSource 持有，Activity 只持有会话；不在这里再存读句柄。
    private var smbNetSession: com.kivo.cinema.playback.NetworkPlaybackSession? = null
    /**
     * 253-P3：libmpv 内核会话。
     *
     * 与 [smbNetSession] **互斥**：同一次播放只有一条视频链路。
     * 引擎选择由 [com.kivo.cinema.playback.PlaybackEngine] 决定
     * （系统属性 `debug.kivo.engine` = `mpv` / `legacy`）。
     */
    private var mpvNetSession: com.kivo.cinema.playback.MpvPlaybackSession? = null
    /**
     * 软解会话（VC-1 / 平台无解码器时走这条）。
     *
     * 与 [smbNetSession] **互斥**：同一次播放只有一条视频链路。分成两个字段而不是
     * 一个联合类型，是因为两者的控制面不同（进度来自另一套计数），
     * 混成一个类型只会让每个调用点都写一次 `if`。
     */
    private var smbSoftSession: com.kivo.cinema.bridge.FfmpegPlaybackSession? = null
    /** 软解音频（独立 fd + 独立 demux）：TrueHD/AC3/DTS 没有平台解码器，只能自己解。 */
    private var smbSoftAudio: com.kivo.cinema.playback.SoftAudioSession? = null
    /** 音频自己那个 fd 的所有者。丢了它 fd 就被回收，音频线程立刻读到 EIO。 */
    private var smbSoftAudioPfd: android.os.ParcelFileDescriptor? = null
    /** 音频是否被音画闸门压住了（视频追不上实时时主动停流）。 */
    private var smbSoftAudioGated = false
    /** 软解片源的字节源：RangeFd 由它产出，软解会话释放后才能关。 */
    private var smbSoftRange: com.kivo.cinema.net.RangeFd? = null
    /**
     * 音频专用的字节源 —— **必须与视频那个分开**。
     *
     * `SmbByteSource` 只有一把锁和一个 1 MiB 预读窗口。视频按帧读、音频按 64 KiB 读，
     * 两者偏移在文件里相差几 MB，共用一个源就会互相把窗口顶掉：每次读都不命中，
     * 每次读都变成一次 1 MiB 的 SMB 往返（设备侧约 11 MB/s，即每次 ~95 ms），
     * 而且还要排队等同一把锁。观感就是几帧一动 —— 用户报的"卡成 PPT"。
     * 各开一条连接、各持一个窗口，顺序读才能命中自己那一段。
     */
    private var smbSoftAudioSource: com.kivo.cinema.smb.SmbByteSource? = null
    private var smbSoftAudioRange: com.kivo.cinema.net.RangeFd? = null
    /** 端到端速率采样的上一个落点（媒体时间 / 墙钟），只用于诊断日志。 */
    private var softRateMarkMs = 0L
    private var softRateMarkPosUs = -1L

    /** 上次采样时"正在响的"音频位置（µs）。-1 = 还没有可用读数。 */
    private var softAudioMarkUs = -1L
    /**
     * 当前这次播放是不是 SMB 片源。
     *
     * 投屏分支要按它判断：243 之后 SMB 走 `SmbByteSource → RangeFd`，
     * 已经没有"本地的 SmbSource 字段"可看，只能靠路由标记。
     */
    private var smbPlayback = false
    // Main-thread ownership includes sessions already closing after Surface loss.
    private val closingNetworks = mutableSetOf<com.kivo.cinema.playback.NetworkPlaybackSession>()
    private var finishRequested = false

    private fun smbTrace(msg: String) {
        Log.i(TAG, msg)
        try {
            // 写内部目录而不是 external：真机实测 `getExternalFilesDir(null)` 可能为 null，
            // 那一刻 `File(null, "evidence")` 会静默失败 —— 日志等于没有，
            // 排障时反而被"没有日志"误导（252 踩过）。
            val dir = java.io.File(filesDir, "evidence").apply { mkdirs() }
            java.io.File(dir, "p220-status.log")
                .appendText("${System.currentTimeMillis()} $msg\n")
        } catch (e: Exception) { }
    }

    private var networkGeneration = 0
    private var networkPreparationCancelled: java.util.concurrent.atomic.AtomicBoolean? = null

    /**
     * 只收 `sourceId + 共享内路径`（243 D5）：凭据从加密存储读，**不再经过 Intent**。
     * 旧 `kivo.smb` JSON 通道已删除——它会把明文密码带进 Intent。
     */
    private fun startSmbPlayback(holder: SurfaceHolder, sourceId: String, filePath: String, shareOverride: String = "") {
        playbackStarted = true
        statusText = "正在连接SMB…"
        val generation = ++networkGeneration
        val cancelled = java.util.concurrent.atomic.AtomicBoolean(false)
        networkPreparationCancelled = cancelled
        Thread({
            var source: com.kivo.cinema.smb.SmbByteSource? = null
            try {
                val record = com.kivo.cinema.sources.storage.SourceStore(this)
                    .read().getOrNull()?.firstOrNull { it.id == sourceId }
                    ?: throw IllegalStateException("找不到这个资源库，可能已被移除")
                val parsed = com.kivo.cinema.smb.SmbAddress.parse(record.address)
                // 共享可能是用户在浏览页里选的、源记录里没写（枚举形态）；页内选的名字随 Intent 带来。
                val address = if (shareOverride.isNotBlank()) parsed.copy(share = shareOverride) else parsed
                check(!address.needsShare) { "这个资源库还没有选择共享" }
                // SmbByteSource 自己负责连接、预读与断线重连（243 D3/借鉴 F:\kivo 音乐播放器）。
                val opened = com.kivo.cinema.smb.SmbByteSource.open(
                    address,
                    com.kivo.cinema.smb.SmbCredential(
                        username = record.username, password = record.password),
                    filePath,
                    // 250 D3：断网时 socket 不会立刻报错，只能"读失败后问一句有没有网"。
                    // 不注入这个，界面上要等满读超时才知道断了。
                    online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) },
                    // 252：读画像必须落到**能读出来的地方**。这台设备（MIUI）过滤应用
                    // logcat，实测 50 秒只剩 74 行，Log.i 写的读画像一行都看不到 ——
                    // 而"卡顿是网络还是解码"只有这份画像能回答。
                    reportSink = { line -> smbTrace("smbread $line") },
                )
                source = opened
                smbTrace("prepared open size=${opened.size()}")
                check(!cancelled.get())
                // 243 D3：SMB 直接产 fd，不再经本地 HTTP 代理。
                val range = com.kivo.cinema.net.RangeFd(
                    opened,
                    getSystemService(android.os.storage.StorageManager::class.java),
                    online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) },
                )
                // 起播前判定链路：**网络片源不再做容器探测**（252）。
                //
                // 真机事故：探测用 `MediaExtractor` 走 SMB fd 解析容器，而 `Shelter`
                // （12.19 GB）头部没有可用索引，解析器于是从头线性扫完整片 ——
                // 实测 15.66 MB/s × 13 分钟不返回，播放页永远停在「正在连接SMB…」。
                // 加截止时间只能止损；探测的唯一价值是那条罕见路径（平台没有该编码的
                // 解码器，如 VC-1）。
                //
                // 改成"先按历史默认走平台解码，解不了再退软解"：正常文件零成本，
                // 异常文件在**真正解码那一刻**才退，不必为了判断把整片读一遍。
                val routing = com.kivo.cinema.bridge.FfmpegRouting.Decision(
                    useFfmpeg = false,
                    decision = "mediacodec",
                    reason = "网络片源跳过容器探测（252）；平台解不了再退软解",
                )
                smbTrace("routing=${routing.decision} reason=${routing.reason}")
                runOnUiThread {
                    if (cancelled.get() || finishRequested || isDestroyed || generation != networkGeneration || !surfaceReady) {
                        Thread { runCatching { range.close() }; runCatching { opened.close() } }.start()
                        return@runOnUiThread
                    }
                    networkPreparationCancelled = null
                    // 253-P3：内核选择。mpv 内核直接吃 ByteSource（SmbByteSource），
                    // 不经过 RangeFd —— mpv 通过 HTTP 代理按 offset 读，与 fd 无关。
                    // 两条链路互斥，共享同一个 opened（SMB 源）。
                    val engine = com.kivo.cinema.playback.PlaybackEngine.select()
                    if (engine == com.kivo.cinema.playback.PlaybackEngine.Engine.MPV) {
                        smbTrace("engine=mpv")
                        startMpvPlayback(holder, opened)
                        return@runOnUiThread
                    }
                    if (routing.useFfmpeg) {
                        // 软解是顺序流式读（FFmpeg 一个读指针往前走），预读命中率实测 100%，
                        // 开了才能跑到 24 fps。
                        opened.setPrefetchEnabled(true)
                        startSoftPlayback(holder, range, routing, filePath, generation, cancelled,
                            address,
                            com.kivo.cinema.smb.SmbCredential(
                                username = record.username, password = record.password))
                    } else {
                        // 硬解由 MediaExtractor 驱动，访问是"突然要一大片"的突发形态：
                        // 预读提前读的数据大半用不上，还会跟主读抢同一条 SMB 链路，
                        // 实测帧率 24.00 → 20.27、末尾出现 567 ms 卡顿。这里关掉，
                        // 让块缓存独自承担（命中率 91~95%）。
                        opened.setPrefetchEnabled(false)
                        val net = com.kivo.cinema.playback.NetworkPlaybackSession(this, range)
                        // 252：平台解不了这个编码（如 VC-1）时退软解，而不是停在报错上。
                        // 这条兜底取代了"起播前探测"——判断成本从"读完整片"降到"解不出第一帧"。
                        startNetworkPlayback(holder, net, "smb") {
                            // 平台解不了这个编码（实测 VC-1：`unsupported codec id 70`）→ 重开一条源走软解。
                            // **不能复用原 range**：会话关闭时会把 source 一起关掉，再开 fd 只会得到
                            // "handle is null"（真机踩过）。代价是多一次连接，只发生在这条罕见路径上。
                            smbTrace("platform decode failed (${net.lastError}) -> ffmpeg (rebind)")
                            runCatching { net.stop() }
                            val credentials = com.kivo.cinema.smb.SmbCredential(
                                record.username, record.password,
                            )
                            Thread {
                                try {
                                    val fresh = com.kivo.cinema.smb.SmbByteSource.open(
                                        address, credentials, filePath,
                                        online = {
                                            com.kivo.cinema.net.NetworkLinkMonitor
                                                .isOnline(applicationContext)
                                        },
                                    )
                                    fresh.setPrefetchEnabled(true)
                                    val freshRange = com.kivo.cinema.net.RangeFd(
                                        fresh,
                                        getSystemService(android.os.storage.StorageManager::class.java),
                                        online = {
                                            com.kivo.cinema.net.NetworkLinkMonitor
                                                .isOnline(applicationContext)
                                        },
                                    )
                                    runOnUiThread {
                                        if (cancelled.get() || finishRequested || isDestroyed) {
                                            runCatching { freshRange.close() }
                                            return@runOnUiThread
                                        }
                                        startSoftPlayback(
                                            holder, freshRange,
                                            com.kivo.cinema.bridge.FfmpegRouting.Decision(
                                                true, "ffmpeg", "platform decode failed; fallback (252)",
                                            ),
                                            filePath, generation, cancelled, address, credentials,
                                        )
                                    }
                                } catch (e: Exception) {
                                    runOnUiThread {
                                        statusText = "这个片源无法播放：${e.message?.take(80)}"
                                        controlsVisible = true
                                    }
                                }
                            }.start()
                        }
                    }
                    loadDanmaku(filePath)
                }
            } catch (e: Exception) {
                runCatching { source?.close() }
                runOnUiThread {
                    if (!cancelled.get() && !finishRequested && !isDestroyed && generation == networkGeneration) {
                        networkPreparationCancelled = null
                        statusText = e.message?.takeIf { it.isNotBlank() }
                            ?: "SMB连接失败（${e.javaClass.simpleName}）"
                        isPlaying = false
                        controlsVisible = true
                    }
                }
            }
        }, "KivoSmbPrepare").start()
    }

    /**
     * 253-P3：libmpv 内核起播。
     *
     * 与 [startNetworkPlayback] 的分工：旧内核用 fd + MediaExtractor；mpv 内核用
     * HTTP 代理 + FFmpeg 的 demux。两条链路**互斥**。
     *
     * 传输层复用同一个 [com.kivo.cinema.smb.SmbByteSource] —— 块窗口/预读/断网恢复
     * 都在它里面，代理只是把 readAt 翻译成 HTTP Range。
     */
    private fun startMpvPlayback(
        holder: SurfaceHolder,
        source: com.kivo.cinema.smb.SmbByteSource,
    ) {
        if (finishRequested || isDestroyed) return
        playbackStarted = true
        smbPlayback = true
        statusText = "正在准备内核播放…"
        val session = com.kivo.cinema.playback.MpvPlaybackSession(this, source)
        mpvNetSession = session
        session.start(holder.surface) { result ->
            if (finishRequested || isDestroyed || mpvNetSession !== session) return@start
            if (result == 0) {
                // 时长：mpv 的 duration 属性在媒体加载完成后才有值。
                // 不回填的话进度条会一直显示 0:00:00 / 0:00:00（用户没法拖进度）。
                if (session.durationUs > 0) {
                    durationMs = session.durationUs / 1000
                }
                statusText = "正在播放"
                isPlaying = true
                lastTouchMs = System.currentTimeMillis()
                android.os.Handler(mainLooper).post(positionPoll)
                applyProbeSubtitle(session)
            } else {
                statusText = "网络视频无法播放：${session.lastError}"
                isPlaying = false
                controlsVisible = true
            }
        }
    }

    /**
     * 253-P6-2 取证钩子：按 intent 指定的轨道 id 选字幕，并可 seek + 冻结。
     *
     * 【为什么这是一个取证钩子而不是功能】
     * 字幕的**完整交互**（字幕列表、开关、样式）属于播放页 UI，按用户要求之后做。
     * 但"mpv 内核能不能把字幕画到画面上"这件事必须**现在**有证据 ——
     * 它是换内核的已知代价（字幕不显示）。
     * 而内核渲染只差一个"选哪条轨"的动作，用 intent extra 驱动就够了，
     * 不必为此先做一整套 UI。
     *
     * 【为什么需要"冻结"】
     * 字幕只在有对白的时刻出现。截图必须落在那个时刻内，
     * 而播放是持续的 —— 宿主机无法精确掐准截图时机。
     * 所以这里 seek 之后**暂停**：画面停住，字幕停在屏幕上，
     * 宿主什么时候截都能截到同一帧。
     *
     * 用法：
     * ```
     * am start -n com.kivo.cinema/.ui.compose.PlayerComposeActivity \
     *   --es kivo.sourceId <id> --es kivo.smbPath <path> \
     *   --ei kivo.subtitleId 2 --ei kivo.seekSec 14 --ez kivo.freeze true
     * ```
     */
    private fun applyProbeSubtitle(session: com.kivo.cinema.playback.MpvPlaybackSession) {
        val subtitleId = intent.getIntExtra("kivo.subtitleId", -1)
        val seekSec = intent.getIntExtra("kivo.seekSec", -1)
        val freeze = intent.getBooleanExtra("kivo.freeze", false)
        if (seekSec > 0) {
            android.os.Handler(mainLooper).postDelayed({
                session.seekTo(seekSec * 1_000_000L)
                if (freeze) {
                    // 等落位完成再停 —— 立刻 pause 可能停在跳转途中。
                    android.os.Handler(mainLooper).postDelayed({
                        session.setPaused(true)
                        smbTrace("probe freeze at ${seekSec}s")
                    }, 2_500)
                }
            }, 1_500)
        }
        if (subtitleId < 0) return
        android.os.Handler(mainLooper).postDelayed({
            val ok = session.selectSubtitle(subtitleId)
            smbTrace("probe subtitle id=$subtitleId ok=$ok")
            statusText = if (ok) {
                "字幕轨 #$subtitleId 已选中"
            } else {
                "字幕轨 #$subtitleId 选中失败：${session.lastError}"
            }
        }, 2_000)
    }

    /** Emby and SMB share the same native A/V pipeline. Preparation never blocks main. */
    private fun startUrlPlayback(holder: SurfaceHolder, url: String, label: String,
                                 routing: String) {
        startNetworkPlayback(holder,
            com.kivo.cinema.playback.NetworkPlaybackSession(this, url), routing)
    }

    /** SMB 走 fd 直连（243 D3）：RangeFd(SmbByteSource) → ParcelFileDescriptor → 解码器。 */
    private fun startNetworkPlayback(holder: SurfaceHolder,
                                     netSession: com.kivo.cinema.playback.NetworkPlaybackSession,
                                     routing: String,
                                     onPlatformFailed: (() -> Unit)? = null) {
        if (finishRequested || isDestroyed) return
        playbackStarted = true
        // 投屏分支要按"这次播放是不是 SMB"来判断，不能再看 smbSource（243 之后它恒空）。
        smbPlayback = routing == "smb"
        statusText = "正在准备网络播放…"
        smbNetSession = netSession
        netSession.start(holder.surface) { result ->
            if (finishRequested || isDestroyed || smbNetSession !== netSession) return@start
            if (result == 0) {
                durationMs = netSession.durationUs / 1000
                statusText = if (netSession.hasAudio) "正在播放" else "正在播放 · 片源没有音轨"
                isPlaying = true
                lastTouchMs = System.currentTimeMillis()
                android.os.Handler(mainLooper).post(positionPoll)
            } else {
                // 平台解不了就走软解兜底（有的话）；没有兜底才报到界面上。
                val fallback = onPlatformFailed
                if (fallback != null) {
                    fallback()
                    return@start
                }
                statusText = "网络视频无法播放：${netSession.lastError}"
                isPlaying = false
                controlsVisible = true
            }
        }
    }

    /**
     * 软解起播（平台没有这个编码的解码器时走这条，例如 VC-1）。
     *
     * 与 [startNetworkPlayback] 的分工：硬解会话内部自己管线程与轮询；软解要
     * "先 open 再 start"两段，且 open 会读文件头（SMB 上是一次网络往返），
     * 所以整段放后台线程，只有结果回主线程落状态。
     *
     * fd 所有权：`range.openDescriptor()` 产出的描述符由会话持有到 [FfmpegPlaybackSession.close]，
     * RangeFd 本身由 Activity 持有到 [closeNetworkPlayback]——顺序反了就是读到已关闭的描述符。
     */
    private fun startSoftPlayback(
        holder: SurfaceHolder,
        range: com.kivo.cinema.net.RangeFd,
        routing: com.kivo.cinema.bridge.FfmpegRouting.Decision,
        filePath: String,
        generation: Int,
        cancelled: java.util.concurrent.atomic.AtomicBoolean,
        address: com.kivo.cinema.smb.SmbAddress,
        credential: com.kivo.cinema.smb.SmbCredential,
    ) {
        playbackStarted = true
        smbPlayback = true
        statusText = "正在用软解打开…"
        smbSoftRange = range
        val surface = holder.surface
        if (surface == null || !surface.isValid) {
            statusText = "播放窗口还没准备好"
            isPlaying = false
            controlsVisible = true
            return
        }
        Thread({
            val label = filePath.substringAfterLast('/').ifBlank { "smb-media" }
            val session = com.kivo.cinema.bridge.FfmpegPlaybackSession(
                label,
                { range.openDescriptor() },
                range.size,
                routing.decision,
                routing.reason,
            )
            var failure: String? = null
            try {
                val openRc = session.openVideo(surface)
                if (openRc != 0) {
                    failure = "软解打不开这个片源：${session.nativeError().ifBlank { "rc=$openRc" }}"
                } else {
                    val startRc = session.start()
                    if (startRc != 0) {
                        failure = "软解启动失败：${session.nativeError().ifBlank { "rc=$startRc" }}"
                    }
                }
            } catch (e: Exception) {
                failure = "软解启动异常（${e.javaClass.simpleName}）"
            }
            val reason = failure
            if (reason != null) {
                runCatching { session.close() }
                runOnUiThread {
                    if (cancelled.get() || finishRequested || isDestroyed || generation != networkGeneration) return@runOnUiThread
                    networkPreparationCancelled = null
                    statusText = reason
                    isPlaying = false
                    controlsVisible = true
                }
                return@Thread
            }
            runOnUiThread {
                if (cancelled.get() || finishRequested || isDestroyed || generation != networkGeneration) {
                    Thread { runCatching { session.close() } }.start()
                    return@runOnUiThread
                }
                networkPreparationCancelled = null
                smbSoftSession = session
                smbSoftAudioGated = false
                softRateMarkMs = 0L
                softRateMarkPosUs = -1L
                softAudioMarkUs = -1L
                val dur = session.durationUs()
                durationMs = if (dur > 0) dur / 1000 else 0L
                statusText = "正在播放 · 软解"
                isPlaying = true
                lastTouchMs = System.currentTimeMillis()
                android.os.Handler(mainLooper).post(positionPoll)
                // 音频在**独立线程 + 独立字节源**上挂载。理由见 [smbSoftAudioSource]：
                // 和视频共用一个 SmbByteSource 会让两边互相顶掉 1 MiB 预读窗口，每次读
                // 都退化成一次 SMB 往返。另外 open() 本身是一次网络往返，不能放主线程。
                Thread({
                    mountSoftAudio(label, address, credential, filePath, generation)
                }, "KivoSmbSoftAudio").start()
            }
        }, "KivoSmbSoftPrepare").start()
    }

    /**
     * 挂软解音频。**用自己的 SmbByteSource / RangeFd / fd**，不复用视频那条。
     *
     * 复用会让 `SmbByteSource` 唯一那个 1 MiB 窗口在"视频偏移"和"音频偏移"之间反复
     * 失效 —— 两个偏移在文件里相差几 MB，等于每次读都不命中、每读一次就 1 MiB 往返，
     * 还要排同一把锁。用户看到的就是几帧一动的 PPT。
     */
    private fun mountSoftAudio(
        label: String,
        address: com.kivo.cinema.smb.SmbAddress,
        credential: com.kivo.cinema.smb.SmbCredential,
        filePath: String,
        generation: Int,
    ) {
        var src: com.kivo.cinema.smb.SmbByteSource? = null
        var audioRange: com.kivo.cinema.net.RangeFd? = null
        var fd: android.os.ParcelFileDescriptor? = null
        try {
            src = com.kivo.cinema.smb.SmbByteSource.open(
                address, credential, filePath,
                online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) },
            )
            // 音频轨读取同样是顺序流式（FFmpeg demux 一个读指针），必须开预读。
            // 关掉的话每 1 MiB 都要同步等一次网络往返（实测 fill_avg 213 ms），
            // 音频环随时可能被抽干，表现就是隔几秒断一下。
            src.setPrefetchEnabled(true)
            audioRange = com.kivo.cinema.net.RangeFd(
                src, getSystemService(android.os.storage.StorageManager::class.java),
                online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) })
            fd = audioRange.openDescriptor()
            val audio = com.kivo.cinema.playback.SoftAudioSession(label, fd, audioRange.size)
            if (audio.start() != 0) {
                smbTrace("soft audio start failed: ${audio.describeFailure()}")
                runCatching { audio.close() }
                runCatching { fd.close() }
                runCatching { audioRange.close() }
                runCatching { src.close() }
                return
            }
            val keepSrc = src
            val keepRange = audioRange
            val keepFd = fd
            runOnUiThread {
                // 竞态：音频连上时用户可能已经返回/换了片源。
                if (generation != networkGeneration || smbSoftSession == null) {
                    Thread({
                        runCatching { audio.close() }
                        runCatching { keepFd.close() }
                        runCatching { keepRange.close() }
                        runCatching { keepSrc.close() }
                    }, "KivoSmbSoftAudioDrop").start()
                    return@runOnUiThread
                }
                smbSoftAudio = audio
                smbSoftAudioPfd = keepFd
                smbSoftAudioRange = keepRange
                smbSoftAudioSource = keepSrc
                smbTrace("soft audio mounted: own source + own window")
            }
        } catch (e: Exception) {
            smbTrace("soft audio source failed: ${e.message?.take(120)}")
            runCatching { fd?.close() }
            runCatching { audioRange?.close() }
            runCatching { src?.close() }
        }
    }

    private fun closeNetworkPlayback() {
        ++networkGeneration
        networkPreparationCancelled?.set(true)
        networkPreparationCancelled = null
        val network = smbNetSession
        smbNetSession = null
        if (network != null) {
            closingNetworks += network
            network.stop {
                android.os.Handler(mainLooper).post { closingNetworks -= network }
            }
        }
        // 253-P3：libmpv 内核会话。与旧内核互斥，但收尾路径要一并处理。
        val mpv = mpvNetSession
        mpvNetSession = null
        if (mpv != null) {
            mpv.stop()
        }
        // 软解的 close() 会 join 解码线程（内部还可能有阻塞读），不能放主线程。
        // 顺序固定：先停会话（它借用的 fd 还活着），再关 RangeFd（fd 的所有者）。
        val soft = smbSoftSession
        smbSoftSession = null
        val softAudio = smbSoftAudio
        smbSoftAudio = null
        val softAudioPfd = smbSoftAudioPfd
        smbSoftAudioPfd = null
        smbSoftAudioGated = false
        val softRange = smbSoftRange
        smbSoftRange = null
        val softAudioRange = smbSoftAudioRange
        smbSoftAudioRange = null
        val softAudioSource = smbSoftAudioSource
        smbSoftAudioSource = null
        softRateMarkMs = 0L
        softRateMarkPosUs = -1L
        softAudioMarkUs = -1L
        if (soft != null || softRange != null || softAudio != null ||
            softAudioRange != null || softAudioSource != null) {
            Thread({
                // 先停两个会话（它们借用的 fd 还活着），再逐层回收 fd / RangeFd / 字节源。
                runCatching { softAudio?.close() }
                runCatching { soft?.close() }
                runCatching { softAudioPfd?.close() }
                runCatching { softAudioRange?.close() }
                runCatching { softAudioSource?.close() }
                runCatching { softRange?.close() }
            }, "KivoSmbSoftClose").start()
        }
    }

    override fun finish() {
        if (finishRequested) return
        finishRequested = true
        // Keep the Activity's Surface alive until the serialized native owner stops releasing.
        closeNetworkPlayback()
        val pending = closingNetworks.toList()
        if (pending.isEmpty()) {
            super.finish()
            return
        }
        isPlaying = false
        statusText = "正在关闭播放…"
        var remaining = pending.size // callbacks below always count on main
        pending.forEach { network -> network.stop {
            android.os.Handler(mainLooper).post {
                if (--remaining == 0 && !isDestroyed) finishAfterPlaybackClosed()
            }
        } }
    }

    private fun finishAfterPlaybackClosed() { super.finish() }

    private fun loadDanmaku(mediaPath: String) {
        try {
            val baseName = mediaPath.substringAfterLast('/').substringBeforeLast('.')
            val localFile = java.io.File(getExternalFilesDir(null), "danmaku/$baseName.xml")
            smbTrace("danmaku try load: ${localFile.absolutePath} exists=${localFile.exists()}")
            if (localFile.exists()) {
                val parsed = com.kivo.cinema.danmaku.DanmakuParser.parse(
                    localFile.bufferedReader()
                )
                danmakuItems = parsed
                danmakuOn = parsed.isNotEmpty()
                smbTrace("danmaku loaded ${parsed.size} items, danmakuOn=$danmakuOn")
            } else {
                smbTrace("danmaku XML not found: ${localFile.absolutePath}")
            }
        } catch (e: Exception) {
            smbTrace("danmaku load error: ${e.message}")
            Log.w(TAG, "danmaku load failed", e)
        }
    }

    private fun togglePlayPause() {
        lastTouchMs = System.currentTimeMillis()
        smbSoftSession?.let { soft ->
            val nowPaused = !soft.isPaused()
            if (soft.setPaused(nowPaused) == 0) {
                isPlaying = !nowPaused
                smbSoftAudio?.setPaused(nowPaused)
                // 用户主动暂停/播放优先于音画闸门：把闸门状态清掉，
                // 否则恢复播放后闸门还记着"已压住"，音频就再也起不来了。
                smbSoftAudioGated = nowPaused
            } else {
                statusText = "播放控制失败：${soft.nativeError()}"
                controlsVisible = true
            }
            return
        }
        smbNetSession?.let { network ->
            network.setPaused(network.state == com.kivo.cinema.playback.NetworkPlaybackSession.State.PLAYING)
            return
        }
        val session = playbackSession ?: return
        val result = if (isPlaying) session.pause() else session.resume()
        if (result == 0) isPlaying = !isPlaying
        else { statusText = "播放控制失败：${session.lastError}"; controlsVisible = true }
    }

    // ---- 237-A：观看数据域（进度 + 统计时长）----
    private val watchStore by lazy { com.kivo.cinema.stats.WatchStore(this) }
    private var watchLastPosMs = 0L
    private var watchAccumMs = 0L

    private val watchPoll = object : Runnable {
        override fun run() {
            if (isPlaying && durationMs > 0) {
                val delta = positionMs - watchLastPosMs
                if (delta in 1..1500) watchAccumMs += delta   // seek 大跳不计入观看时长
            }
            watchLastPosMs = positionMs
            if (watchAccumMs >= 10_000) {
                watchStore.report(watchKey(), positionMs, durationMs, watchAccumMs)
                watchAccumMs = 0
            }
            android.os.Handler(mainLooper).postDelayed(this, 1000)
        }
    }

    private fun watchKey(): String =
        currentMediaPath().ifBlank { playUrlOverride }

    private fun flushWatch() {
        if (watchLastPosMs > 0 && durationMs > 0) {
            watchStore.report(watchKey(), positionMs, durationMs, watchAccumMs)
            watchAccumMs = 0
        }
    }

    // ---- 230：DLNA 投屏（按钮→sheet→CastController）----
    private val castController by lazy { com.kivo.cinema.dlna.CastController(this) }
    private var castSheetVisible by mutableStateOf(false)
    private var castScanning by mutableStateOf(false)
    private var castDevices by mutableStateOf(
        listOf<com.kivo.cinema.dlna.DlnaDevice>())
    private var castMessage by mutableStateOf("")
    private var castDevice by mutableStateOf<com.kivo.cinema.dlna.DlnaDevice?>(null)

    /** 投屏的 URL 直传来源（kivo.playurl 会话时非空；D4 URL 直传策略）。 */
    private var playUrlOverride = ""

    private fun openCastSheet() {
        smbTrace("cast sheet open, scanning begin")
        castSheetVisible = true
        castScanning = true
        castMessage = ""
        Thread {
            val list = try {
                castController.scan { line -> smbTrace("cast $line") }
            } catch (e: Exception) {
                Log.w(TAG, "dlna scan failed", e)
                smbTrace("cast scan EXC ${e.javaClass.name}: ${e.message}")
                emptyList()
            }
            smbTrace("cast scan done: ${list.size} devices ${list.joinToString(";") { it.friendlyName }}")
            runOnUiThread {
                castDevices = list
                castScanning = false
                if (list.isEmpty()) castMessage = "4s 内未发现渲染器"
            }
        }.start()
    }

    private fun onCastSelect(device: com.kivo.cinema.dlna.DlnaDevice) {
        smbTrace("cast select ${device.friendlyName} ctrl=${device.controlUrl}")
        castMessage = "连接 ${device.friendlyName}…"
        Thread {
            val title = java.io.File(
                if (currentMediaPath().isNotBlank()) currentMediaPath()
                else playUrlOverride).name.ifBlank { "kivo" }
            val rc = when {
                smbPlayback -> -2   // SMB 片源投屏留 v2（需要第二条 SmbByteSource 给渲染器取流）
                playUrlOverride.isNotBlank() ->
                    castController.castUrl(device, playUrlOverride, title)
                else -> castController.castSource(
                    device, com.kivo.cinema.net.FileByteSource(currentMediaPath()), title)
            }
            runOnUiThread {
                if (rc == 0) {
                    castDevice = device
                    castSheetVisible = false
                    // D6：渲染端成为主屏，本地暂停（能 pause 的会话才暂停——
                    // FfmpegPlaybackSession 无 pause API，软解会话继续本地播，偏差记 status 文档）
                    runCatching { playbackSession?.pause() }
                    statusText = "投屏中：${device.friendlyName}"
                } else {
                    castMessage = if (rc == -2) "SMB 片源投屏暂未开放"
                                  else "投屏失败（rc=$rc）"
                }
            }
        }.start()
    }

    // ---- 215 取证 harness（`kivo.evidence=p215`）：60 点扫描 + 主链路 writeStats 对照 ----
    private var stressDurationUs = -1L

    private fun scheduleP215Stress() {
        android.os.Handler(mainLooper).postDelayed({ runP215Stress(0) }, 2000)
    }

    /** 片长：harness 自己开一次性 extractor 读 KEY_DURATION。
     *  不经 [thumbnailReader]——reader 的第一次使用必须留给第一个 seekTo（AC1 冷启动含 init 才诚实）。 */
    private fun probeDurationUs(path: String): Long {
        return try {
            val ex = android.media.MediaExtractor()
            ex.setDataSource(path)
            var dur = -1L
            for (i in 0 until ex.trackCount) {
                val f = ex.getTrackFormat(i)
                if (f.getString(android.media.MediaFormat.KEY_MIME)?.startsWith("video/") == true) {
                    dur = f.getLong(android.media.MediaFormat.KEY_DURATION)
                    break
                }
            }
            ex.release()
            dur
        } catch (e: Exception) {
            -1L
        }
    }

    private fun runP215Stress(index: Int) {
        val session = playbackSession ?: return
        val reader = thumbnailReader ?: return
        val evidenceDir = java.io.File(getExternalFilesDir(null), "evidence").apply { mkdirs() }
        if (index == 0) {
            statusText = "p215 baseline writeStats..."
            session.writeStats("${evidenceDir.absolutePath}/p215-stats-baseline.json")
            stressDurationUs = probeDurationUs(currentMediaPath())
        }
        if (index >= 60) {
            // 扫描完：补一次已缓存位置（cache_hit 通道），然后收证据
            statusText = "p215 scan done, cache-hit probe..."
            reader.seekTo(stressDurationUs / 4)   // 60 点覆盖全片，1/4 处必已缓存
            android.os.Handler(mainLooper).postDelayed({ finishP215Stress(evidenceDir) }, 1500)
            return
        }
        if (index == 30 && evidenceName == "p215") {
            // 预览演示窗口：与拖动共用同一条浮窗 UI（3s，给截图留确定性窗口）
            previewMs = (stressDurationUs / 2) / 1000
            controlsVisible = true
            android.os.Handler(mainLooper).postDelayed({ previewMs = -1L }, 4000)
        }
        if (stressDurationUs > 0) {
            val posUs = ((index + 0.5) / 60.0 * stressDurationUs).toLong()
            reader.seekTo(posUs)
        }
        statusText = "p215 scan ${index + 1}/60"
        android.os.Handler(mainLooper).postDelayed({ runP215Stress(index + 1) }, 250)
    }

    private fun currentMediaPath(): String {
        mediaOverridePath.takeIf { it.isNotBlank() }?.let { return it }
        return locateTestMedia()?.absolutePath ?: ""
    }

    private fun finishP215Stress(evidenceDir: java.io.File) {
        val session = playbackSession ?: return
        val reader = thumbnailReader ?: return
        session.writeStats("${evidenceDir.absolutePath}/p215-stats-stress.json")
        val thumb = org.json.JSONObject(reader.evidenceJson())
        val meta = org.json.JSONObject()
        meta.put("device_model", android.os.Build.MODEL)
        meta.put("device_product", android.os.Build.PRODUCT)
        meta.put("media", mediaOverridePath.ifBlank { "located-test-media" })
        meta.put("stress_points", 60)
        meta.put("scan_spacing_ms", 250)
        val root = org.json.JSONObject()
        root.put("schema", "kivo.p215.thumbnail/1")
        root.put("generated_at_unix_ms", System.currentTimeMillis())
        root.put("meta", meta)
        root.put("thumbnail", thumb)
        val out = java.io.File(evidenceDir, "p215-thumb.json")
        out.writeText(root.toString(2))
        stressDone = true
        statusText = "p215 evidence written: ${out.name}"
        Log.i(TAG, "p215 evidence: $out")
    }

    private fun readProbeExtras() {
        mediaOverridePath = intent.getStringExtra("kivo.media") ?: ""
        evidenceName = intent.getStringExtra("kivo.evidence") ?: ""
        playUrlOverride = intent.getStringExtra("kivo.playurl") ?: ""
    }

    private fun locateTestMedia(): java.io.File? {
        val dir = java.io.File(getExternalFilesDir(null), "media")
        return dir.listFiles()?.filter { it.isFile }?.minByOrNull { it.name }
    }

    override fun onDestroy() {
        android.os.Handler(mainLooper).removeCallbacks(watchPoll)
        runCatching { flushWatch() }
        android.os.Handler(mainLooper).removeCallbacks(positionPoll)
        // 230：离场收投屏（Stop SOAP + 关 LAN 代理），渲染端不悬挂
        if (castDevice != null) {
            Thread { runCatching { castController.stop() } }.start()
            castDevice = null
        }
        closeNetworkPlayback()
        thumbnailReader?.release()
        thumbnailReader = null
        playbackSession?.close()
        playbackSession = null
        super.onDestroy()
    }
}
