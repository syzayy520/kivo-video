package com.kivo.cinema.ui.compose

import android.os.Bundle
import android.view.SurfaceHolder
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.kivo.cinema.bridge.PlaybackSession

/**
 * Compose 播放页的**组合根**（213 / 253-v3）。
 *
 * 这里只有三样东西：状态所有权、层叠接线、生命周期。业务一律在扩展文件里：
 * 起播 [PlayerSessionStart] / [PlayerSmbStart]，回显 [PlayerSessionBridge]，
 * 收尾 [PlayerSessionTeardown]，动作 [PlayerActionRouter]，手势 [PlayerGestureRouter]，
 * 面板写入 [PlayerPanelRouter]，投屏 [PlayerCastBridge]，取证 [PlayerProbeHooks]。
 *
 * 【为什么状态留在 Activity】面板数据源来自会话，而会话生命周期归 Activity 管；
 * 放 Composable 里则重建即丢选择。扩展函数只把**行为**搬出去，所有权不动。
 *
 * 视频层走 AndroidView(PlayerSurfaceView) —— 零拷贝路径一行不动（D2）。
 */
class PlayerComposeActivity : ComponentActivity() {

    // ---- 会话与画面 ----
    internal var playbackSession: PlaybackSession? = null
    internal var surfaceReady = false
    internal var playbackStarted = false

    /**
     * 最近一次拿到的 SurfaceHolder。
     * 状态层"重试"要重新起播，而 onSurfaceAvailable 只在面变化时回调，
     * 重试那一刻不会再回调一次。
     */
    internal var lastSurfaceHolder: SurfaceHolder? = null
    internal var mediaOverridePath = ""
    internal var evidenceName = ""
    internal var thumbnailReader: com.kivo.cinema.thumbnail.PreviewFrameReader? = null

    // ---- 播放状态（Compose）----
    internal var isPlaying by mutableStateOf(false)
    internal var controlsVisible by mutableStateOf(true)
    internal var positionMs by mutableStateOf(0L)
    internal var durationMs by mutableStateOf(0L)
    internal var statusText by mutableStateOf("starting...")
    internal var thumbnailBitmap by mutableStateOf<android.graphics.Bitmap?>(null)
    /** 预览帧产不出时的可见原因（不许静默降级）；空 = 正常或尚未请求。 */
    internal var thumbnailUnavailableReason by mutableStateOf("")
    internal var previewMs by mutableStateOf(-1L)
    /** 拖动进行中（onScrub 置真、onSeek 置假）：弹幕让位给预览浮窗。 */
    internal var scrubbing by mutableStateOf(false)
    internal var danmakuOn by mutableStateOf(false)
    internal var danmakuItems by mutableStateOf(listOf<com.kivo.cinema.danmaku.DanmakuItem>())

    // ---- v3 新增态（听视频 / 屏幕锁定 / 缓冲进度 / 面板刷新）----
    /** 听视频态（形态13）：vid=no，音频继续。 */
    internal var audioOnly by mutableStateOf(false)
    /** 弹幕显示设置（v3.1）：本地态，面板 stepper 直写；hasItems 由侧载结果填。 */
    internal var danmakuSettings by mutableStateOf(com.kivo.cinema.danmaku.DanmakuSettings())
    /** 已缓冲比例 = (位置+demuxer-cache-time)/片长，1Hz 采样。 */
    internal var bufferedFraction by mutableStateOf(0f)
    /** 屏幕锁定（§1.5）：chrome 全收、只留挂锁；双击/长按保留。 */
    internal var screenLocked by mutableStateOf(false)
    internal var lockHintVisible by mutableStateOf(false)
    /** 写侧每次成功写入 +1，驱动 rememberPanelItems 重算。 */
    internal var panelRevision = 0
    /** v3.1：PiP 小窗态 —— 小窗内 chrome 全收（PlayerOverlay 按它门控）。 */
    internal var pipActive by mutableStateOf(false)
    /** 本次进入请求是否真的收到 mode-changed（看门狗判静默拒绝用）。 */
    internal var pipEnterConfirmed = false

    // ---- 外挂字幕导入（§1.6：SAF 选文件 → 拷缓存 → sub-add select）----
    internal val subtitleImportLauncher = registerForActivityResult(
        androidx.activity.result.contract.ActivityResultContracts.OpenDocument(),
    ) { uri -> if (uri != null) importSubtitleFile(uri) }

    // ---- 覆盖层状态 ----
    /** 当前打开的面板（空 = 都没开）。 */
    internal var activePanel by mutableStateOf("")
    /** 刚执行过的动作（顶栏图标 1.5s 高亮反馈）。 */
    internal var activeAction by mutableStateOf("")
    internal var flashZone by mutableStateOf<com.kivo.cinema.ui.compose.player.DoubleTapZone?>(null)
    internal var toastText by mutableStateOf("")
    internal var hudKind by mutableStateOf<com.kivo.cinema.ui.compose.player.PlayerHudKind?>(null)
    internal var hudFraction by mutableStateOf(0f)
    /** 竖滑 HUD 的延时淡出令牌：再次滑动前先撤销上一个，避免旧计时器提前收掉 HUD。 */
    internal var hudHideToken: Runnable? = null
    /** 顶栏实时网速，来自 mpv `cache-speed`（bytes/s、1s 窗口），不是自己估的。 */
    internal var speedText by mutableStateOf("")
    /** 分辨率标签缓存：听视频态读不到 video-params 时沿用最后真值。 */
    internal var lastResolutionLabel = ""

    // ---- 长按快进 ----
    internal var longPressActive by mutableStateOf(false)
    internal var longPressSpeed by mutableStateOf(2.0)
    internal var longPressLocked by mutableStateOf(false)
    internal var speedBeforeLongPress by mutableStateOf(1.0)
    internal var longPressStartX by mutableStateOf(0f)
    internal var longPressStartY by mutableStateOf(0f)

    // ---- 轮询节流 ----
    internal var lastTouchMs = 0L
    internal var lastSpeedSampleMs = 0L
    internal var lastSummaryLogMs = 0L

    // ---- 网络 / 软解链路（三条互斥，同一次播放只有一条视频链路）----
    internal var smbNetSession: com.kivo.cinema.playback.NetworkPlaybackSession? = null
    /** libmpv 内核会话（253-P3）。引擎由 PlaybackEngine 选择。 */
    internal var mpvNetSession: com.kivo.cinema.playback.MpvPlaybackSession? = null
    internal var smbSoftSession: com.kivo.cinema.bridge.FfmpegPlaybackSession? = null
    /** 软解音频：TrueHD/AC3/DTS 没有平台解码器，只能自己解（独立 fd + 独立 demux）。 */
    internal var smbSoftAudio: com.kivo.cinema.playback.SoftAudioSession? = null
    /** 音频 fd 的所有者。丢了它 fd 就被回收，音频线程立刻读到 EIO。 */
    internal var smbSoftAudioPfd: android.os.ParcelFileDescriptor? = null
    internal var smbSoftAudioGated = false
    internal var smbSoftRange: com.kivo.cinema.net.RangeFd? = null

    /**
     * 音频专用字节源 —— **必须与视频那个分开**。
     * SmbByteSource 只有一把锁和一个 1 MiB 预读窗口；视频按帧读、音频按 64 KiB 读，
     * 偏移相差几 MB，共用就互相顶掉窗口 ⇒ 每次读都变成一次 SMB 往返（~95ms）还要排队。
     * 观感就是几帧一动（用户报的"卡成 PPT"）。
     */
    internal var smbSoftAudioSource: com.kivo.cinema.smb.SmbByteSource? = null
    internal var smbSoftAudioRange: com.kivo.cinema.net.RangeFd? = null
    /** v3.1：SMB 缩略图独立读路径（拖动预览帧）。与主链路零共用，收尾时关闭。 */
    internal var smbThumbSource: com.kivo.cinema.smb.SmbByteSource? = null
    internal var softRateMarkMs = 0L
    internal var softRateMarkPosUs = -1L
    internal var softAudioMarkUs = -1L
    /** 本次播放是不是 SMB 片源（投屏分支按它判断；243 之后没有本地 SmbSource 字段可看）。 */
    internal var smbPlayback = false
    internal val closingNetworks = mutableSetOf<com.kivo.cinema.playback.NetworkPlaybackSession>()
    internal var finishRequested = false
    internal var networkGeneration = 0
    internal var networkPreparationCancelled: java.util.concurrent.atomic.AtomicBoolean? = null

    // ---- 观看数据域（237-A）----
    internal val watchStore by lazy { com.kivo.cinema.stats.WatchStore(this) }
    internal var watchLastPosMs = 0L
    internal var watchAccumMs = 0L

    // ---- 投屏（230）----
    internal val castController by lazy { com.kivo.cinema.dlna.CastController(this) }
    internal var castSheetVisible by mutableStateOf(false)
    internal var castScanning by mutableStateOf(false)
    internal var castDevices by mutableStateOf(listOf<com.kivo.cinema.dlna.DlnaDevice>())
    internal var castMessage by mutableStateOf("")
    internal var castDevice by mutableStateOf<com.kivo.cinema.dlna.DlnaDevice?>(null)
    internal var playUrlOverride = ""

    // ---- 取证 harness（215）----
    internal var stressDurationUs = -1L

    /**
     * 轮询必须是**同一个实例**：onDestroy 要 removeCallbacks，
     * 每次 new 一个就摘不掉旧任务，Activity 销毁后还在跑。
     */
    internal val positionPoll: Runnable = PlayerPositionPoll(this)
    internal val watchPoll: Runnable = PlayerWatchPoll(this)

    internal companion object {
        internal const val TAG = "KivoCompose"
        /** 双击快进/快退步长。 */
        internal const val DOUBLE_TAP_SEEK_MS = 10_000L
        /** 长按快进起步速度。 */
        internal const val LONG_PRESS_BASE_SPEED = 2.0
        /** 上滑锁定阈值（dp）。 */
        internal const val LOCK_UP_THRESHOLD_DP = 60f

        /**
         * 进程内唯一播放页。新页诞生即 finish 旧页：旧页退后台只走 onStop，
         * 而本页没有 onStop 暂停语义（214 后台播放 / PiP 都靠会话活着），
         * 不finish 旧页就是两条音频叠播（真机实测 2026-11-12，用户报）。
         */
        private var livePlayer: java.lang.ref.WeakReference<PlayerComposeActivity>? = null
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        livePlayer?.get()?.takeIf { it !== this }?.finish()
        livePlayer = java.lang.ref.WeakReference(this)
        window.addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        // 沉浸式全屏：播放器里状态栏与导航条都是"切画面"，黑底电影尤其刺眼。
        com.kivo.cinema.ui.compose.player.applyImmersivePlayerWindow(this)
        readProbeExtras()
        scheduleThumbDump()
        scheduleThumbProbe()
        scheduleThumbScan()
        android.os.Handler(mainLooper).post(watchPoll)
        // BACK 层级：面板 → 投屏 sheet → 控制条 → 退出（v3.1b §九）。
        onBackPressedDispatcher.addCallback(this, object : androidx.activity.OnBackPressedCallback(true) {
            override fun handleOnBackPressed() {
                if (!handleBack()) {
                    isEnabled = false
                    onBackPressedDispatcher.onBackPressed()
                }
            }
        })

        setContent { PlayerRoot() }
    }

    fun onSurfaceAvailable(holder: SurfaceHolder) {
        if (finishRequested || isDestroyed) return
        surfaceReady = true
        lastSurfaceHolder = holder
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

    override fun onPictureInPictureModeChanged(
        isInPictureInPictureMode: Boolean,
        newConfig: android.content.res.Configuration,
    ) {
        super.onPictureInPictureModeChanged(isInPictureInPictureMode, newConfig)
        onPipModeChanged(isInPictureInPictureMode)
    }

    override fun finish() {
        if (finishRequested) return
        finishRequested = true
        // Surface 必须活到串行化的 native 所有者停止释放为止。
        closeNetworkPlayback()
        val pending = closingNetworks.toList()
        if (pending.isEmpty()) {
            super.finish()
            return
        }
        isPlaying = false
        statusText = "正在关闭播放…"
        var remaining = pending.size   // 下面的回调一律在主线程计数
        pending.forEach { network ->
            network.stop {
                android.os.Handler(mainLooper).post {
                    if (--remaining == 0 && !isDestroyed) finishAfterPlaybackClosed()
                }
            }
        }
    }

    private fun finishAfterPlaybackClosed() { super.finish() }

    override fun onDestroy() {
        if (livePlayer?.get() === this) livePlayer = null
        android.os.Handler(mainLooper).removeCallbacks(watchPoll)
        runCatching { flushWatch() }
        android.os.Handler(mainLooper).removeCallbacks(positionPoll)
        // 离场收投屏（Stop SOAP + 关 LAN 代理），渲染端不悬挂。
        if (castDevice != null) {
            Thread { runCatching { castController.stop() } }.start()
            castDevice = null
        }
        // 先停预览帧内核（它借着 thumbSource），再关源：顺序反了就是读已关闭的源。
        thumbnailReader?.release()
        thumbnailReader = null
        closeNetworkPlayback()
        playbackSession?.close()
        playbackSession = null
        super.onDestroy()
    }
}