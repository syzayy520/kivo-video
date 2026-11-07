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
            val netSession = smbNetSession
            if (netSession != null) {
                // 220b：硬解会话的位置上报
                positionMs = (netSession.positionUs / 1000).coerceAtMost(durationMs)
                controlsVisible = true  // 诊断模式：常显
                android.os.Handler(mainLooper).postDelayed(this, 100)
                return
            }
            val ffSession = smbFfSession
            if (ffSession != null) {
                // 220：SMB 路径的位置/结束上报；durationMs 兜底（open 后异步才可知）
                if (durationMs <= 0) {
                    val d = ffSession.durationUs() / 1000
                    if (d > 0) durationMs = d
                }
                positionMs = (ffSession.positionUs() / 1000).coerceAtMost(durationMs)
                if (!ffSession.isRunning() && smbStarted != 0L) {
                    val reason = runCatching { ffSession.stopAndCollectEvidence() }
                        .getOrElse { "stop failed" }
                    // 证据落盘（断连/结束原因可复算）
                    try {
                        val dir = java.io.File(getExternalFilesDir(null), "evidence").apply { mkdirs() }
                        java.io.File(dir, "p220-end.json").writeText(reason)
                    } catch (e: Exception) { }
                    statusText = "SMB 播放结束: ${reason.takeLast(60)}"
                    smbFfSession = null
                    runCatching { smbProxy?.close() }
                    runCatching { smbSource?.close() }
                    isPlaying = false
                    controlsVisible = true
                    return
                }
                // SMB v1：控制条常显（自动隐藏留到 UI 卡统一做——tap 翻转与
                // 100ms 轮询的时序在截屏取证里互相干扰，本轮先保证判据可读）
                controlsVisible = true
                android.os.Handler(mainLooper).postDelayed(this, 100)
                return
            }
            val session = playbackSession ?: return
            // EOS 后时钟外推会越过片长（真机实测 205s/60s 片）——clamp 到时长
            positionMs = (session.positionUs() / 1000).coerceAtMost(durationMs)
            // 3s 无操作自动隐藏（D4）；预览窗口期间不隐藏（取证 harness 需要）
            if (previewMs < 0 && System.currentTimeMillis() - lastTouchMs > 3000) {
                controlsVisible = false
            }
            android.os.Handler(mainLooper).postDelayed(this, 100)
        }
    }
    private var lastTouchMs = 0L

    companion object {
        private const val TAG = "KivoCompose"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        readProbeExtras()

        setContent {
            MaterialTheme(colorScheme = darkColorScheme()) {
                Box(modifier = Modifier.fillMaxSize().background(Color.Black)) {
                    PlayerScreen(
                        onSurfaceReady = { holder -> maybeStartPlayback(holder) },
                        onSurfaceLost = { surfaceReady = false },
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
                                smbNetSession?.seekTo(ms * 1000)
                                    ?: smbFfSession?.seekTo(ms * 1000)
                                    ?: playbackSession?.seekTo(ms * 1000)
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
        surfaceReady = true
        maybeStartPlayback(holder)
    }

    fun onSurfaceLost() {
        surfaceReady = false
    }

    private fun maybeStartPlayback(holder: SurfaceHolder) {
        // surface 回调本身就是"面可用"的证明——不再依赖 Listener 的 surfaceReady 标志
        surfaceReady = true
        if (playbackStarted) return
        val media = mediaOverridePath.takeIf { it.isNotBlank() }
            ?.let { java.io.File(it) }?.takeIf { it.isFile }
            ?: locateTestMedia() ?: return
        playbackStarted = true

        val smbJson = intent.getStringExtra("kivo.smb") ?: ""
        if (smbJson.isNotBlank()) {
            startSmbPlayback(holder, smbJson)
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
        statusText = "playing ${media.name}"

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

    // ---- 220：SMB 片源播放（libsmb2 → 本地 Range 代理 → FFmpeg http）----
    private var smbSource: com.kivo.cinema.smb.Smb2Native.Source? = null
    private var smbProxy: com.kivo.cinema.net.LocalRangeProxy? = null
    private var smbFfSession: com.kivo.cinema.bridge.FfmpegPlaybackSession? = null
    private var smbNetSession: com.kivo.cinema.playback.NetworkPlaybackSession? = null
    private var smbStarted = 0L

    private fun smbTrace(msg: String) {
        Log.i(TAG, msg)
        try {
            val dir = java.io.File(getExternalFilesDir(null), "evidence").apply { mkdirs() }
            java.io.File(dir, "p220-status.log")
                .appendText("${System.currentTimeMillis()} $msg\n")
        } catch (e: Exception) { }
    }

    private fun startSmbPlayback(holder: SurfaceHolder, smbJson: String) {
        val media = mediaOverridePath.takeIf { it.isNotBlank() }
            ?.let { java.io.File(it) }?.takeIf { it.isFile }
        val displayName = media?.name ?: "smb"
        playbackStarted = true
        smbTrace("startSmbPlayback begin")

        val cfg = com.kivo.cinema.smb.SmbConfig.fromJson(smbJson)
        if (cfg == null) {
            statusText = "bad kivo.smb json"
            finish()
            return
        }
        statusText = "smb connect ${cfg.host}/${cfg.share}..."
        val source = com.kivo.cinema.smb.Smb2Native.Source(cfg)
        try {
            source.connect()
            smbSource = source
            val opened = source.openForRead(cfg.path)
            smbTrace("smb size=${opened.size()}")
            statusText = "smb size=${opened.size()}"
            val proxy = com.kivo.cinema.net.LocalRangeProxy(opened)
            smbProxy = proxy
            val port = proxy.start()
            val url = "http://127.0.0.1:$port/kivo"
            smbTrace("proxy port=$port url=$url")
            statusText = "proxy :$port, opening..."

            // 220b：硬解优先（MediaExtractor http URL → MediaCodec Surface），
            // 失败回退 220 v1 软解 URL 通道（G9 reason 路由出版）
            val netSession = com.kivo.cinema.playback.NetworkPlaybackSession(url, displayName)
            smbNetSession = netSession
            val netRc = netSession.start(holder.surface)
            if (netRc == 0) {
                smbTrace("network harddec start ok ${netSession.width}x${netSession.height}")
                durationMs = netSession.durationUs / 1000
                statusText = "playing smb://${cfg.host}/${cfg.share}/${cfg.path} (harddec)"
                isPlaying = true
                smbStarted = System.currentTimeMillis()
                lastTouchMs = smbStarted
                loadDanmaku(cfg.path)
                android.os.Handler(mainLooper).post(positionPoll)
                // 诊断：40s 后停 + 落节拍曲线（220b 速率验证）
                android.os.Handler(mainLooper).postDelayed({
                    smbNetSession?.let { ns ->
                        val curve = ns.paceCurve()
                        val json = org.json.JSONArray()
                        curve.forEach { (w, m) -> json.put(listOf(w, m)) }
                        try {
                            val dir = java.io.File(getExternalFilesDir(null), "evidence").apply { mkdirs() }
                            java.io.File(dir, "p220b-pace.json").writeText(json.toString())
                        } catch (e: Exception) { }
                        statusText = "pace diagnostic written (${curve.size} samples)"
                    }
                }, 40000)
                return
            }
            smbTrace("harddec failed rc=$netRc err=${netSession.lastError} - softdec fallback")
            statusText = "硬解不可用（${netSession.lastError}），回退软解…"
            runCatching { netSession.stop() }
            smbNetSession = null

            val fsession = com.kivo.cinema.bridge.FfmpegPlaybackSession(
                media ?: java.io.File("."),
                displayName,
                routingDecision = "ffmpeg",
                routingReason = "smb via local range proxy (220)"
            )
            smbFfSession = fsession
            val rc = fsession.openUrlVideo(url, holder.surface)
            smbTrace("openUrlVideo rc=$rc err=${fsession.lastError}")
            if (rc != 0) {
                Log.e(TAG, "openUrlVideo failed rc=$rc: ${fsession.lastError}")
                statusText = "SMB open failed: ${fsession.lastError}"
                proxy.close()
                source.close()
                finish()
                return
            }
            val startRc = fsession.start()
            smbTrace("start rc=$startRc")
            if (startRc != 0) {
                statusText = "smb start failed rc=$startRc"
                runCatching { fsession.stopAndCollectEvidence() }
                finish()
                return
            }
            isPlaying = true
            smbStarted = System.currentTimeMillis()
            lastTouchMs = smbStarted
            durationMs = fsession.durationUs() / 1000
            statusText = "playing smb://${cfg.host}/${cfg.share}/${cfg.path}"
            Log.i(TAG, "SMB playback started: $url durationMs=$durationMs")
            // positionPoll：SMB 会话的位置/断连上报全靠它
            android.os.Handler(mainLooper).post(positionPoll)
            // 诊断：40s 后自动停（读节拍曲线）——220 定位节拍失效用
            android.os.Handler(mainLooper).postDelayed({
                smbFfSession?.let { s ->
                    val ev = runCatching { s.stopAndCollectEvidence() }.getOrElse { "stop failed" }
                    try {
                        val dir = java.io.File(getExternalFilesDir(null), "evidence").apply { mkdirs() }
                        java.io.File(dir, "p220-pace-720p.json").writeText(ev)
                    } catch (e: Exception) { }
                    statusText = "pace diagnostic written"
                }
            }, 40000)
        } catch (e: Exception) {
            Log.e(TAG, "smb failed", e)
            smbTrace("EXCEPTION ${e.javaClass.name}: ${e.message}")
            statusText = "SMB failed: ${e.message}"
            runCatching { source.close() }
            runCatching { smbProxy?.close() }
            finish()
        }
    }

    /** 通用 URL 直开（AC5 对照测量用）：与 SMB 路径共用 FfmpegPlaybackSession。 */
    private fun startUrlPlayback(holder: SurfaceHolder, url: String, label: String,
                                 routing: String) {
        playbackStarted = true
        smbTrace("url-play begin url=$url")

        // 220b 路由：硬解优先（MediaExtractor http URL → MediaCodec Surface）
        val netSession = com.kivo.cinema.playback.NetworkPlaybackSession(url, label)
        smbNetSession = netSession
        val netRc = netSession.start(holder.surface)
        if (netRc == 0) {
            smbTrace("url-play harddec ok ${netSession.width}x${netSession.height}")
            durationMs = netSession.durationUs / 1000
            statusText = "playing $url (harddec)"
            isPlaying = true
            smbStarted = System.currentTimeMillis()
            lastTouchMs = smbStarted
            android.os.Handler(mainLooper).post(positionPoll)
            return
        }
        smbTrace("url-play harddec failed rc=$netRc err=${netSession.lastError} → softdec")
        runCatching { netSession.stop() }
        smbNetSession = null

        // 软解回退（220 v1 通道）
        val fsession = com.kivo.cinema.bridge.FfmpegPlaybackSession(
            java.io.File(label), label,
            routingDecision = "ffmpeg",
            routingReason = "playurl: $url (harddec failed: ${netSession.lastError})"
        )
        smbFfSession = fsession
        val rc = fsession.openUrlVideo(url, holder.surface)
        smbTrace("url-play openUrlVideo rc=$rc")
        if (rc != 0) {
            statusText = "URL open failed: ${fsession.lastError}"
            fsession.close()
            smbFfSession = null
            finish()
            return
        }
        val started = fsession.start()
        smbTrace("url-play start rc=$started")
        if (started != 0) {
            runCatching { fsession.stopAndCollectEvidence() }
            finish()
            return
        }
        isPlaying = true
        smbStarted = System.currentTimeMillis()
        lastTouchMs = smbStarted
        durationMs = fsession.durationUs() / 1000
        statusText = "playing $url (softdec)"
        android.os.Handler(mainLooper).post(positionPoll)
    }

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
        val session = playbackSession ?: return
        isPlaying = !isPlaying
        if (isPlaying) session.resume() else session.pause()
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
                smbSource != null -> -2   // SMB 片源投屏留 v2（需第二 SmbByteSource）
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
        android.os.Handler(mainLooper).removeCallbacks(positionPoll)
        // 230：离场收投屏（Stop SOAP + 关 LAN 代理），渲染端不悬挂
        if (castDevice != null) {
            Thread { runCatching { castController.stop() } }.start()
            castDevice = null
        }
        smbFfSession?.let { s -> runCatching { s.stopAndCollectEvidence() } }
        smbFfSession = null
        smbNetSession?.let { s -> runCatching { s.stop() } }
        smbNetSession = null
        runCatching { smbProxy?.close() }
        smbProxy = null
        runCatching { smbSource?.close() }
        smbSource = null
        thumbnailReader?.release()
        thumbnailReader = null
        playbackSession?.close()
        playbackSession = null
        super.onDestroy()
    }
}
