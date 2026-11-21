package com.kivo.cinema.ui.compose

import com.kivo.cinema.ui.compose.player.PlayerPanelStyle
import com.kivo.cinema.ui.compose.player.PlayerPhase
import com.kivo.cinema.ui.compose.player.resolutionTier
import com.kivo.cinema.ui.compose.player.toastIsError

internal fun PlayerComposeActivity.currentTitle(): String {
    intent.getStringExtra("kivo.label")?.takeIf { it.isNotBlank() }?.let { return it }
    intent.getStringExtra("kivo.smbPath")?.takeIf { it.isNotBlank() }?.let { return it.substringAfterLast('/') }
    intent.getStringExtra("kivo.playurl")?.takeIf { it.isNotBlank() }?.let { return it.substringAfterLast('/') }
    return "正在播放"
}

internal fun PlayerComposeActivity.resolutionLabel(): String {
    val native = mpvNetSession?.nativeOrNull() ?: return lastResolutionLabel
    val width = runCatching { native.property("video-params/w")?.toIntOrNull() }.getOrNull()
    val height = runCatching { native.property("video-params/h")?.toIntOrNull() }.getOrNull()
    if (width == null || height == null || width <= 0 || height <= 0) return lastResolutionLabel
    val label = resolutionTier(width, height)
    if (label.isNotEmpty()) lastResolutionLabel = label
    return label.ifEmpty { lastResolutionLabel }
}

internal fun PlayerComposeActivity.togglePlayPause() {
    lastTouchMs = System.currentTimeMillis()
    smbSoftSession?.let { soft ->
        val nowPaused = !soft.isPaused()
        if (soft.setPaused(nowPaused) == 0) {
            isPlaying = !nowPaused
            smbSoftAudio?.setPaused(nowPaused)
            smbSoftAudioGated = nowPaused
        } else {
            showToast("播放控制失败：${soft.nativeError()}")
            controlsVisible = true
        }
        return
    }
    smbNetSession?.let { network ->
        network.setPaused(network.state == com.kivo.cinema.playback.NetworkPlaybackSession.State.PLAYING)
        return
    }
    mpvNetSession?.let { mpv ->
        val nowPaused = isPlaying
        Thread { mpv.setPaused(nowPaused) }.start()
        return
    }
    val session = playbackSession ?: return
    val result = if (isPlaying) session.pause() else session.resume()
    if (result == 0) isPlaying = !isPlaying
    else {
        showToast("播放控制失败：${session.lastError}")
        controlsVisible = true
    }
}

internal fun PlayerComposeActivity.onTopAction(kind: String) {
    lastTouchMs = System.currentTimeMillis()
    // P9 取证（dev-only）：确认顶部图标点击是否到达路由层。
    if (intent.getBooleanExtra("kivo.touchLog", false)) smbTrace("router topAction=$kind panel=$activePanel")
    when (kind) {
        // P10 §17：投屏浮卡没有「收起」堆在底部，再点图标即关闭（toggle）。
        "cast" -> if (castSheetVisible) castSheetVisible = false else openCastSheet()
        "danmaku" -> {
            if (!danmakuOn && !danmakuSettings.hasItems) {
                showToast("未找到弹幕文件（danmaku/<片名>.xml）")
                return
            }
            danmakuOn = !danmakuOn
            flashAction("danmaku")
            showToast(if (danmakuOn) "弹幕已开启" else "弹幕已关闭")
        }
        "danmaku-panel" -> togglePanel("danmakuOps")
        "pip" -> enterPictureInPicture()
        "clapper" -> togglePanel("skip")
        "expand" -> togglePanel("quality")
        "audio" -> togglePanel("audio")
        "captions" -> togglePanel("subtitle")
        "audioMode" -> setAudioOnly(!audioOnly)
    }
}

internal fun PlayerComposeActivity.onHome() {
    lastTouchMs = System.currentTimeMillis()
    runCatching {
        startActivity(android.content.Intent(this, com.kivo.cinema.library.MediaLibraryActivity::class.java))
    }.onFailure { showToast("无法打开资源库：${it.message}") }
}

internal fun PlayerComposeActivity.handleBack(): Boolean {
    when {
        activePanel.isNotEmpty() -> {
            activePanel = ""
            lastTouchMs = System.currentTimeMillis()
        }
        castSheetVisible -> {
            castSheetVisible = false
            lastTouchMs = System.currentTimeMillis()
        }
        controlsVisible -> controlsVisible = false
        else -> return false
    }
    return true
}

internal fun PlayerComposeActivity.onCapture() {
    lastTouchMs = System.currentTimeMillis()
    val mpv = mpvNetSession
    if (mpv == null) {
        showToast("内核尚未就绪，暂时无法截图")
        return
    }
    val dir = java.io.File(filesDir, "captures").apply { mkdirs() }
    val file = java.io.File(dir, "kivo-${System.currentTimeMillis()}.png")
    Thread {
        val ok = mpv.command("screenshot-to-file", "\"${file.absolutePath}\" video")
        runOnUiThread {
            showToast(if (ok) "剧照已保存：captures/${file.name}" else "截图失败：${mpv.lastError}")
        }
    }.start()
}

internal fun PlayerComposeActivity.onToggleLock() {
    lastTouchMs = System.currentTimeMillis()
    screenLocked = !screenLocked
    activePanel = ""
    if (screenLocked) {
        controlsVisible = false
        showLockHint()
        haptic(android.view.HapticFeedbackConstants.LONG_PRESS)
        showToast("已锁定屏幕，点按左下挂锁解除")
    } else {
        lockHintVisible = false
        controlsVisible = true
    }
}

internal fun PlayerComposeActivity.onLockHint() {
    lastTouchMs = System.currentTimeMillis()
    showLockHint()
}

private fun PlayerComposeActivity.showLockHint() {
    lockHintVisible = true
    android.os.Handler(mainLooper).postDelayed({ lockHintVisible = false }, 2_500)
}

internal fun PlayerComposeActivity.onEpisodeOrder() {
    lastTouchMs = System.currentTimeMillis()
    showToast("剧集列表尚未接入资源库，暂不能切换播放顺序")
}

internal fun PlayerComposeActivity.onBottomTextAction(kind: String) {
    lastTouchMs = System.currentTimeMillis()
    if (intent.getBooleanExtra("kivo.touchLog", false)) smbTrace("router bottomAction=$kind panel=$activePanel")
    togglePanel(kind)
}

internal fun PlayerComposeActivity.togglePanel(kind: String) {
    activePanel = if (activePanel == kind) "" else kind
    if (intent.getBooleanExtra("kivo.touchLog", false)) smbTrace("router togglePanel->$activePanel")
}

internal fun PlayerComposeActivity.onRetry() {
    lastTouchMs = System.currentTimeMillis()
    statusText = "正在重连…"
    phase = PlayerPhase.Buffering
    controlsVisible = true
    val holder = lastSurfaceHolder ?: run {
        phase = PlayerPhase.Failed("播放窗口已失效，请退出后重新进入")
        return
    }
    val oldMpv = mpvNetSession
    closeNetworkPlayback()
    playbackStarted = false
    Thread({
        oldMpv?.awaitClosed(3_000)
        android.os.Handler(mainLooper).post {
            if (finishRequested || isDestroyed) return@post
            maybeStartPlayback(holder)
        }
    }, "KivoPlayerRetry").start()
}

internal fun PlayerComposeActivity.onNextEpisode() {
    lastTouchMs = System.currentTimeMillis()
    showToast("剧集列表尚未接入资源库，暂不能切换下一集")
}

/**
 * 提示停留：成功 1.8s / 失败 2.6s（P10 §3）。判据与琥珀点同源（[toastIsError]），
 * 避免"带错误标记却一闪而过"。
 */
internal fun PlayerComposeActivity.showToast(
    message: String,
    durationMs: Long = if (toastIsError(message)) PlayerPanelStyle.toastErrorMs
    else PlayerPanelStyle.toastSuccessMs,
) {
    toastText = message
    android.os.Handler(mainLooper).postDelayed({ toastText = "" }, durationMs)
}

internal fun PlayerComposeActivity.flashAction(kind: String) {
    activeAction = kind
    android.os.Handler(mainLooper).postDelayed({ activeAction = "" }, 1_500)
}
