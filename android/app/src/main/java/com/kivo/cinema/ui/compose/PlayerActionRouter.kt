package com.kivo.cinema.ui.compose

import com.kivo.cinema.ui.compose.player.DoubleTapZone
import com.kivo.cinema.ui.compose.player.resolutionTier

/**
 * 动作路由：顶栏 / 底栏 / 状态层的点击 → 会话调用。
 *
 * 【为什么单独一个文件】这里只有"点哪儿调哪儿"，没有面板内容计算、没有手势数学。
 * 三者混在一个 Activity 里时，改一个图标语义要在两千行里翻找。
 */

/** 顶栏片名：优先传入标题，否则文件名。 */
internal fun PlayerComposeActivity.currentTitle(): String {
    intent.getStringExtra("kivo.label")?.takeIf { it.isNotBlank() }?.let { return it }
    intent.getStringExtra("kivo.smbPath")?.takeIf { it.isNotBlank() }?.let { return it.substringAfterLast('/') }
    intent.getStringExtra("kivo.playurl")?.takeIf { it.isNotBlank() }?.let { return it.substringAfterLast('/') }
    return "正在播放"
}

/**
 * 底栏画质标签（REX 的 "4K" 位置）。
 *
 * 用 nativeOrNull：本函数在 Compose 重组时调用，会话未起时也会走到，
 * 会抛的 native() 直接崩掉整个界面。标错比不标更糟，所以够不上就不标。
 */
internal fun PlayerComposeActivity.resolutionLabel(): String {
    val native = mpvNetSession?.nativeOrNull() ?: return lastResolutionLabel
    val width = runCatching { native.property("video-params/w")?.toIntOrNull() }.getOrNull()
    val height = runCatching { native.property("video-params/h")?.toIntOrNull() }.getOrNull()
    if (width == null || height == null || width <= 0 || height <= 0) return lastResolutionLabel
    // 分档唯一规则在 PlayerResolution：宽银幕（1920×870）也报 1080P，与资源面板一致。
    val label = resolutionTier(width, height)
    // 听视频态 vid=no 读不到 video-params ⇒ 沿用最后一次真值（REX 0172 保留 4K 标）。
    if (label.isNotEmpty()) lastResolutionLabel = label
    return label.ifEmpty { lastResolutionLabel }
}

/** 统一 seek 入口（四条链路）。 */
internal fun PlayerComposeActivity.seekTo(ms: Long) {
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
}

internal fun PlayerComposeActivity.togglePlayPause() {
    lastTouchMs = System.currentTimeMillis()
    smbSoftSession?.let { soft ->
        val nowPaused = !soft.isPaused()
        if (soft.setPaused(nowPaused) == 0) {
            isPlaying = !nowPaused
            smbSoftAudio?.setPaused(nowPaused)
            // 用户主动暂停/播放优先于音画闸门：不清掉闸门状态，
            // 恢复播放后闸门还记着"已压住"，音频就再也起不来。
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
    // mpv 链路：setPaused 写 pause yes/no，isPlaying 由桥轮询 state 同步。
    // 漏了这条时暂停键在 mpv 路径静默无反应（真机实测死钮，2026-11-12）。
    mpvNetSession?.let { mpv ->
        val nowPaused = isPlaying
        Thread { mpv.setPaused(nowPaused) }.start()
        return
    }
    val session = playbackSession ?: return
    val result = if (isPlaying) session.pause() else session.resume()
    if (result == 0) isPlaying = !isPlaying
    else { statusText = "播放控制失败：${session.lastError}"; controlsVisible = true }
}

/** 顶栏 8 个动作入口（语义序见 player-ui-v3 §1.1）。 */
internal fun PlayerComposeActivity.onTopAction(kind: String) {
    lastTouchMs = System.currentTimeMillis()
    when (kind) {
        // 投屏（v3.1：DLNA 直达设备 sheet；AirPlay 专有协议已整条移除）。
        "cast" -> openCastSheet()
        "danmaku" -> {
            // 铁律：没有弹幕文件时不许"开了但什么都不显示"，明说原因。
            if (!danmakuOn && !danmakuSettings.hasItems) {
                showToast("未找到弹幕文件（danmaku/<片名>.xml）")
                return
            }
            danmakuOn = !danmakuOn
            flashAction("danmaku")
            showToast(if (danmakuOn) "弹幕已开启" else "弹幕已关闭")
        }
        // 圈弹长按 = 弹幕操作面板（形态8，◐ 触发方式待 iPhone 补证）。
        "danmaku-panel" -> togglePanel("danmakuOps")
        "pip" -> enterPictureInPicture()
        "clapper" -> togglePanel("skip")
        "expand" -> togglePanel("quality")
        // 圈波形＝音轨列表（形态4，REX 0162 锚点；不是均衡器）。
        "audio" -> togglePanel("audio")
        "captions" -> togglePanel("subtitle")
        // 耳机＝听视频态（形态13，REX 0172）：短按切 vid=no/auto，控制条保留。
        "audioMode" -> setAudioOnly(!audioOnly)
    }
}

/** 顶栏主页：回资源库（播放页留在回退栈里）。 */
internal fun PlayerComposeActivity.onHome() {
    lastTouchMs = System.currentTimeMillis()
    runCatching {
        startActivity(android.content.Intent(this, com.kivo.cinema.library.MediaLibraryActivity::class.java))
    }.onFailure { showToast("无法打开资源库：${it.message}") }
}

/**
 * BACK 层级（v3.1b §九）：面板 → 投屏 sheet → 控制条 → 退出播放。
 * 此前全 app 没有 BACK 处理：面板开着按 BACK 直接 finish（真机实测）。
 * 返回 true = 已消费；false = 交给默认 finish。
 */
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

/**
 * 拖动预览左边的相机钮：截当前帧为剧照（mpv `screenshot-to-file`）。
 * 落在私有 captures 目录 —— 不申请存储权限，也不往相册里塞东西。
 */
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

/**
 * 屏幕锁定（§1.5 / §二）。入口 ◐：长按播放键（REX 入口未录到，见设计稿 §六）。
 * 加锁后 chrome 全收、只留挂锁；双击/长按通道保留（M03-S07）。
 */
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

/** 锁定态单击 = 仅唤出挂锁（2.5s 后自隐，◐ 停留时长待补证）。 */
internal fun PlayerComposeActivity.onLockHint() {
    lastTouchMs = System.currentTimeMillis()
    showLockHint()
}

private fun PlayerComposeActivity.showLockHint() {
    lockHintVisible = true
    android.os.Handler(mainLooper).postDelayed({ lockHintVisible = false }, 2_500)
}

/** 选集带右上的"顺序播放"：剧集列表未接入 ⇒ 明说，不静默。 */
internal fun PlayerComposeActivity.onEpisodeOrder() {
    lastTouchMs = System.currentTimeMillis()
    showToast("剧集列表尚未接入资源库，暂不能切换播放顺序")
}

/** 底栏右侧文字按钮。 */
internal fun PlayerComposeActivity.onBottomTextAction(kind: String) {
    lastTouchMs = System.currentTimeMillis()
    togglePanel(kind)
}

internal fun PlayerComposeActivity.togglePanel(kind: String) {
    activePanel = if (activePanel == kind) "" else kind
}

/** 状态层"重试" = 重新起播当前源。 */
internal fun PlayerComposeActivity.onRetry() {
    lastTouchMs = System.currentTimeMillis()
    statusText = "正在重连…"
    controlsVisible = true
    val holder = lastSurfaceHolder ?: return
    playbackStarted = false
    maybeStartPlayback(holder)
}

/**
 * 双击（左=快退 / 中=播放暂停 / 右=快进）。
 * 双击时画面在动，用户看不出这次点击干了什么 ⇒ 居中图标闪 0.6s 是零成本确认。
 */
internal fun PlayerComposeActivity.onDoubleTap(zone: DoubleTapZone) {
    lastTouchMs = System.currentTimeMillis()
    flashZone = zone
    android.os.Handler(mainLooper).postDelayed({ flashZone = null }, 600)
    when (zone) {
        DoubleTapZone.FORWARD -> seekTo(positionMs + PlayerComposeActivity.DOUBLE_TAP_SEEK_MS)
        DoubleTapZone.BACKWARD -> seekTo((positionMs - PlayerComposeActivity.DOUBLE_TAP_SEEK_MS).coerceAtLeast(0L))
        DoubleTapZone.TOGGLE_PLAY -> togglePlayPause()
    }
}

/**
 * "下一集"。铁律：不许静默降级。
 * 剧集列表还没接进资源库 ⇒ 明确告诉用户，而不是点了没反应。
 */
internal fun PlayerComposeActivity.onNextEpisode() {
    lastTouchMs = System.currentTimeMillis()
    showToast("剧集列表尚未接入资源库，暂不能切换下一集")
}

/** 轻提示：1.8s 后自动消失（REX M08-03 停留 1.0–2.0s）。 */
internal fun PlayerComposeActivity.showToast(message: String) {
    toastText = message
    android.os.Handler(mainLooper).postDelayed({ toastText = "" }, 1_800)
}

/** 顶栏动作的短暂高亮（切换成功的零成本确认）。 */
internal fun PlayerComposeActivity.flashAction(kind: String) {
    activeAction = kind
    android.os.Handler(mainLooper).postDelayed({ activeAction = "" }, 1_500)
}