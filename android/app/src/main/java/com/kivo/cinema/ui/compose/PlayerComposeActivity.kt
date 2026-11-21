package com.kivo.cinema.ui.compose

import android.os.Bundle
import android.view.SurfaceHolder
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.kivo.cinema.bridge.PlaybackSession
import com.kivo.cinema.ui.compose.player.PlayerPhase
import com.kivo.cinema.ui.compose.player.PlayerPhaseHolder

class PlayerComposeActivity : ComponentActivity() {
    internal var playbackSession: PlaybackSession? = null
    internal var surfaceReady = false
    internal var playbackStarted = false
    internal var lastSurfaceHolder: SurfaceHolder? = null
    internal var mediaOverridePath = ""
    internal var evidenceName = ""
    internal var thumbnailReader: com.kivo.cinema.thumbnail.PreviewFrameReader? = null
    // 首帧前只记预热意图，避免第二条读源连接拖慢主内核起播。
    internal var pendingThumbPrewarm = false

    internal var isPlaying by mutableStateOf(false)
    internal var controlsVisible by mutableStateOf(true)
    internal var positionMs by mutableStateOf(0L)
    internal var durationMs by mutableStateOf(0L)
    internal var statusText by mutableStateOf("starting...")
    internal val phaseHolder = PlayerPhaseHolder { armBufferWatchdog() }
    internal var phase: PlayerPhase
        get() = phaseHolder.phase
        set(value) { phaseHolder.phase = value }
    internal var thumbnailBitmap by mutableStateOf<android.graphics.Bitmap?>(null)
    internal var thumbnailUnavailableReason by mutableStateOf("")
    internal var previewMs by mutableStateOf(-1L)
    // 预览请求去重：settle 效果与取证探针共用，谁先发 seekTo 谁记录，另一方跳过。
    internal var previewRequestedMs = -1L
    internal val scrub = com.kivo.cinema.ui.compose.player.seek.PlayerScrubState()
    internal var scrubbing by mutableStateOf(false)
    internal var danmakuOn by mutableStateOf(false)
    internal var danmakuItems by mutableStateOf(listOf<com.kivo.cinema.danmaku.DanmakuItem>())
    internal var audioOnly by mutableStateOf(false)
    internal var danmakuSettings by mutableStateOf(com.kivo.cinema.danmaku.DanmakuSettings())
    internal var bufferedFraction by mutableStateOf(0f)
    internal var screenLocked by mutableStateOf(false)
    internal var lockHintVisible by mutableStateOf(false)
    // 必须是 Compose 状态：写侧（stepper/重置/导入）靠 `panelRevision++` 通知读侧重算，
    // 普通 Int 的 ++ 不触发重组，暂停调样式时面板会永远停在旧快照（P9 返工 FAIL-1 读侧根因）。
    internal var panelRevision by mutableStateOf(0)
    /** P9 · I：不支持字幕的播放模式只提示一次，不每次重进都弹。 */
    internal var subtitleHintShown = false
    internal var pipActive by mutableStateOf(false)
    internal var pipEnterConfirmed = false
    internal val landingSeek = LandingSeekState()
    internal var pausedByBackground = false

    internal val subtitleImportLauncher = registerForActivityResult(
        androidx.activity.result.contract.ActivityResultContracts.OpenDocument(),
    ) { uri -> if (uri != null) importSubtitleFile(uri) }

    /**
     * 本会话**最近一次**成功挂上的字幕（库内 fileKey = 文件名去掉扩展名）。
     *
     * 【为什么要记在 App 状态里】换编码时需要知道"当前字幕是哪一份"，而真机实测
     * `track-list/count` 与 `current-tracks/sub/<字段>` 都可能读不到（只有 `sid` 可读）。
     * 挂载那一刻我们自己就知道路径，记下来比依赖内核属性稳。
     * 用户改选别的字幕轨时由 PlayerSubtitleImport.attachImported 覆盖。
     */
    internal var lastSubtitleFileKey: String? = null
    internal var activePanel by mutableStateOf("")
    internal var activeAction by mutableStateOf("")
    internal var flashZone by mutableStateOf<com.kivo.cinema.ui.compose.player.DoubleTapZone?>(null)
    internal var flashEventId by mutableStateOf(0L)
    internal var gestureEpoch by mutableStateOf(0L)
    internal var toastText by mutableStateOf("")
    internal var hudKind by mutableStateOf<com.kivo.cinema.ui.compose.player.PlayerHudKind?>(null)
    internal var hudFraction by mutableStateOf(0f)
    internal var hudOriginFraction by mutableStateOf(0f)
    internal var hudDragging by mutableStateOf(false)
    internal var hudRequestedFraction = 0f
    internal var autoNextScheduled = false
    internal var nextPrewarmed = false
    internal var episodeSeasonPick by mutableStateOf(-1)
    internal var embyPlayingReported = false
    internal var embyReportedAtMs = 0L
    internal var hudHideToken: Runnable? = null
    internal var speedText by mutableStateOf("")
    internal var lastResolutionLabel = ""
    internal var longPressActive by mutableStateOf(false)
    internal var longPressSpeed by mutableStateOf(2.0)
    internal var longPressLocked by mutableStateOf(false)
    internal val speedGesture by lazy { PlayerSpeedGestureController(this) }
    internal var lastTouchMs = 0L
    internal var lastSpeedSampleMs = 0L
    internal var lastSummaryLogMs = 0L

    internal var smbNetSession: com.kivo.cinema.playback.NetworkPlaybackSession? = null
    internal var mpvNetSession: com.kivo.cinema.playback.MpvPlaybackSession? = null
    internal var smbSoftSession: com.kivo.cinema.bridge.FfmpegPlaybackSession? = null
    internal var smbSoftAudio: com.kivo.cinema.playback.SoftAudioSession? = null
    internal var smbSoftAudioPfd: android.os.ParcelFileDescriptor? = null
    internal var smbSoftAudioGated = false
    internal var smbSoftRange: com.kivo.cinema.net.RangeFd? = null
    // 音频独占读源，不能与视频共享单锁和预读窗口。
    internal var smbSoftAudioSource: com.kivo.cinema.smb.SmbByteSource? = null
    internal var smbSoftAudioRange: com.kivo.cinema.net.RangeFd? = null
    internal var netThumbSource: com.kivo.cinema.net.ByteSource? = null
    internal var softRateMarkMs = 0L
    internal var softRateMarkPosUs = -1L
    internal var softAudioMarkUs = -1L
    internal var smbPlayback = false
    internal val closingNetworks = mutableSetOf<com.kivo.cinema.playback.NetworkPlaybackSession>()
    internal var finishRequested = false
    internal var networkGeneration = 0
    internal var networkPreparationCancelled: java.util.concurrent.atomic.AtomicBoolean? = null
    internal var urlPrep: PlayerUrlPrep? = null

    internal val watchStore by lazy { com.kivo.cinema.stats.WatchStore(this) }
    internal val playbackPrefs by lazy { com.kivo.cinema.playback.PlaybackPrefs(this) }
    internal var watchLastPosMs = 0L
    internal var watchAccumMs = 0L
    internal val castController by lazy { com.kivo.cinema.dlna.CastController(this) }
    internal var castSheetVisible by mutableStateOf(false)
    internal var castScanning by mutableStateOf(false)
    internal var castDevices by mutableStateOf(listOf<com.kivo.cinema.dlna.DlnaDevice>())
    internal var castMessage by mutableStateOf("")
    internal var castDevice by mutableStateOf<com.kivo.cinema.dlna.DlnaDevice?>(null)
    internal var playUrlOverride = ""
    internal var stressDurationUs = -1L
    // removeCallbacks 必须拿同一个 Runnable 实例。
    internal val positionPoll: Runnable = PlayerPositionPoll(this)
    internal val watchPoll: Runnable = PlayerWatchPoll(this)

    internal companion object {
        internal const val TAG = "KivoCompose"
        internal const val LOCK_UP_THRESHOLD_DP = 60f
        private var livePlayer: java.lang.ref.WeakReference<PlayerComposeActivity>? = null
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        com.kivo.cinema.net.NetDiag.attach(application)
        livePlayer?.get()?.takeIf { it !== this }?.finish()
        livePlayer = java.lang.ref.WeakReference(this)
        window.addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        com.kivo.cinema.ui.compose.player.applyImmersivePlayerWindow(this)
        readProbeExtras()
        urlPrep = prepareUrlSource()
        scheduleThumbDump()
        scheduleThumbProbe()
        scheduleThumbScan()
        android.os.Handler(mainLooper).post(watchPoll)
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
        com.kivo.cinema.net.NetDiag.log("[prep] surface 就绪")
        maybeStartPlayback(holder)
    }

    fun prepareHdrSurface(holder: SurfaceHolder) {
        val stack = com.kivo.cinema.playback.PlaybackStack.resolve(this, intent)
        val result = if (stack.hdrOutput) {
            com.kivo.cinema.playback.HdrSurfaceConfig.enableHdrSurface(holder, holder.surface)
        } else {
            com.kivo.cinema.playback.HdrSurfaceConfig.disableHdrSurface(holder, holder.surface)
        }
        com.kivo.cinema.net.NetDiag.log("[hdr] ${stack.describe()} ⇒ $result")
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

    override fun onPause() {
        cancelPlayerGestures()
        super.onPause()
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (!hasFocus) cancelPlayerGestures()
    }

    override fun onStop() {
        super.onStop()
        onPlayerStop()
        if (!playbackPrefs.backgroundAudio) com.kivo.cinema.playback.EmbyPlaybackReporter.onStopped(this)
        // 首帧后的快照不含后续 seek 和长稳数据，退出时补齐。
        runCatching {
            com.kivo.cinema.playback.PlaybackMetrics.finish(applicationContext, mpvNetSession?.nativeOrNull())
        }
    }

    override fun onResume() {
        super.onResume()
        onPlayerResume()
    }

    override fun finish() {
        if (finishRequested) return
        cancelPlayerGestures()
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
        var remaining = pending.size
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
        cancelPlayerGestures()
        if (livePlayer?.get() === this) livePlayer = null
        android.os.Handler(mainLooper).removeCallbacks(watchPoll)
        runCatching { flushWatch() }
        android.os.Handler(mainLooper).removeCallbacks(positionPoll)
        if (castDevice != null) {
            Thread { runCatching { castController.stop() } }.start()
            castDevice = null
        }
        thumbnailReader?.release()
        thumbnailReader = null
        closeNetworkPlayback()
        speedGesture.shutdown()
        // 内核停止之后才能释放它仍可能读取的源。
        runCatching { urlPrep?.source?.close() }
        urlPrep = null
        playbackSession?.close()
        playbackSession = null
        super.onDestroy()
    }
}
