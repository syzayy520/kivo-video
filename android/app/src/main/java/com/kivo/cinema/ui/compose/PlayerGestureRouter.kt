package com.kivo.cinema.ui.compose

import com.kivo.cinema.ui.compose.player.PlayerHudKind
import com.kivo.cinema.ui.compose.player.PlayerMotion
import com.kivo.cinema.ui.design.AppearancePrefs

/**
 * 手势数学：竖滑亮度/音量、长按快进（调速 + 上滑锁定）。
 *
 * 【为什么与动作路由分开】这里是**连续量**的计算（位移→比例→系统值），
 * 动作路由是**离散事件**的分发。两者的测试方式完全不同：
 * 前者要核对手势阈值与还原行为，后者要核对每个按钮是否有响应。
 */

/** 竖滑起点：左半屏亮度、右半屏音量。 */
internal fun PlayerComposeActivity.onVerticalDragStart(x: Float) {
    // 全屏上下滑动调整音量和亮度（264）关闭时，竖滑不接管（不弹 HUD、不改系统值）。
    if (!playbackPrefs.verticalSwipeEnabled) return
    cancelHudHide()
    val w = resources.displayMetrics.widthPixels.toFloat().coerceAtLeast(1f)
    val kind = if (x < w / 2f) PlayerHudKind.BRIGHTNESS else PlayerHudKind.VOLUME
    // 单独调整播放亮度（263）关闭时，左半屏竖滑不接管亮度（保持系统亮度）。
    if (kind == PlayerHudKind.BRIGHTNESS && !playbackPrefs.independentBrightness) return
    hudKind = kind
    hudFraction = currentHudFraction(kind)
}

/** 竖滑过程：向上滑 = 增大。半屏高 55% 走完全量程。 */
internal fun PlayerComposeActivity.onVerticalDrag(dragAmount: Float) {
    val kind = hudKind ?: return
    val h = resources.displayMetrics.heightPixels.toFloat().coerceAtLeast(1f)
    val next = (hudFraction - dragAmount / (h * 0.55f)).coerceIn(0f, 1f)
    hudFraction = next
    applyHudValue(kind, next)
}

/**
 * 手势结束：HUD 停留 [PlayerMotion.hudHoldMs] 后淡出（"停止 1s→淡出"）。
 *
 * 【为什么用可撤销令牌】旧版每次结束都 new 一个 Handler.postDelayed 且从不撤销：
 * 连续两次竖滑时，第一次的计时器会在第二次还在滑的时候把 HUD 收掉（提前消失）。
 * 现在复用 decorView 的队列，结束/重新开始时都先 removeCallbacks 上一个令牌。
 */
internal fun PlayerComposeActivity.onVerticalDragEnd() {
    cancelHudHide()
    val token = Runnable { hudKind = null; hudHideToken = null }
    hudHideToken = token
    window.decorView.postDelayed(token, PlayerMotion.hudHoldMs.toLong())
}

/** 撤销待执行的 HUD 淡出（重新开始滑动或再次结束时调用）。 */
private fun PlayerComposeActivity.cancelHudHide() {
    hudHideToken?.let { window.decorView.removeCallbacks(it) }
    hudHideToken = null
}

/**
 * 长按快进开始（longPress≥0.5s → 标签淡入）。
 *
 * 【为什么底栏区域不响应】进度条在手势层之上，长按会抢走它的拖拽 ——
 * 在进度条上长按是"想 seek"，不是"想快进"。起点落在底部条内直接忽略。
 */
internal fun PlayerComposeActivity.onLongPressStart(x: Float, y: Float) {
    // 长按快进总开关（264）；锁定态是否允许由 lockAllowGestures 门控。
    if (!playbackPrefs.longPressEnabled) return
    if (screenLocked && !playbackPrefs.lockAllowGestures) return
    val h = resources.displayMetrics.heightPixels.toFloat()
    if (y > h - resources.displayMetrics.density * 96f) return
    val mpv = mpvNetSession ?: return
    val base = playbackPrefs.longPressBaseSpeed
    speedBeforeLongPress = mpv.currentSpeedOrNull() ?: 1.0
    longPressStartX = x
    longPressStartY = y
    longPressSpeed = base
    longPressLocked = false
    longPressActive = true
    applySpeed(base)
    haptic(android.view.HapticFeedbackConstants.LONG_PRESS)
}

/** 长按拖动：左右滑调速（0.5x…3x）；上滑超阈值 ⇒ 锁定（松手不恢复）。两能力各由 264 开关门控。 */
internal fun PlayerComposeActivity.onLongPressDrag(x: Float, y: Float) {
    if (!longPressActive) return
    // 屏幕锁定态：长按快进保留，但调速/上滑锁定 ❌。
    if (screenLocked) return
    // 快进时向上滑动锁定（264）：关闭时不进入锁定态。
    if (playbackPrefs.longPressSwipeLock) {
        val lockPx = PlayerComposeActivity.LOCK_UP_THRESHOLD_DP * resources.displayMetrics.density
        if (!longPressLocked && (longPressStartY - y) >= lockPx) {
            longPressLocked = true
            haptic(android.view.HapticFeedbackConstants.VIRTUAL_KEY)
            showToast("已锁定快进 ${formatSpeed(longPressSpeed)}，点按画面解除")
        }
    }
    if (longPressLocked) return
    // 快进时左右滑动调整速度（264）：关闭时保持起步倍速。每 320px 一倍速。
    if (!playbackPrefs.longPressSwipeSpeed) return
    val next = (playbackPrefs.longPressBaseSpeed + (x - longPressStartX) / 320f)
        .coerceIn(0.5, 3.0)
    if (next != longPressSpeed) {
        longPressSpeed = next
        applySpeed(next)
    }
}

/** 长按结束：未锁定且恢复策略为「长按前速度」时还原（264 长按松手时恢复至）。 */
internal fun PlayerComposeActivity.onLongPressEnd() {
    if (!longPressActive) return
    longPressActive = false
    if (!longPressLocked && playbackPrefs.longPressRestore == "previous") applySpeed(speedBeforeLongPress)
}

/** 解除快进锁定（点按画面时调用）。 */
internal fun PlayerComposeActivity.unlockLongPress() {
    if (!longPressLocked) return
    longPressLocked = false
    applySpeed(speedBeforeLongPress)
    showToast("已解除快进锁定")
}

internal fun PlayerComposeActivity.applySpeed(value: Double) {
    val mpv = mpvNetSession ?: return
    if (!mpv.setSpeed(value)) showToast("快进失败：${mpv.lastError}")
}

/** 速度标签（1.5 / 2 / 2.5 这种干净写法）。 */
internal fun PlayerComposeActivity.formatSpeed(value: Double): String {
    val text = if (value % 1.0 == 0.0) value.toInt().toString() else value.toString()
    return "${text}x"
}

/** 触感反馈（267）：始终/自动/禁用三态门控，自动跟随系统低电量。 */
internal fun PlayerComposeActivity.haptic(constant: Int) {
    if (!AppearancePrefs.hapticAllowed(this)) return
    window.decorView.performHapticFeedback(constant)
}

internal fun PlayerComposeActivity.currentHudFraction(kind: PlayerHudKind): Float = when (kind) {
    PlayerHudKind.BRIGHTNESS -> currentWindowBrightness()
    PlayerHudKind.VOLUME -> currentMediaVolume()
}

internal fun PlayerComposeActivity.applyHudValue(kind: PlayerHudKind, value: Float) {
    when (kind) {
        PlayerHudKind.BRIGHTNESS -> {
            // 窗口级亮度：改系统全局会让整台设备变暗，退出还得手动调回来。
            // 窗口级只影响本播放器，退出即失效，天然满足"退出还原系统亮度"。
            val params = window.attributes
            params.screenBrightness = value
            window.attributes = params
        }
        PlayerHudKind.VOLUME -> {
            val am = audioManager()
            val max = am.getStreamMaxVolume(android.media.AudioManager.STREAM_MUSIC)
            if (max > 0) {
                am.setStreamVolume(android.media.AudioManager.STREAM_MUSIC, (value * max).toInt(), 0)
            }
        }
    }
}

/** 窗口级亮度为 -1（跟随系统）时取系统亮度。 */
internal fun PlayerComposeActivity.currentWindowBrightness(): Float {
    val windowBrightness = window.attributes.screenBrightness
    if (windowBrightness >= 0f) return windowBrightness
    return runCatching {
        android.provider.Settings.System.getInt(
            contentResolver, android.provider.Settings.System.SCREEN_BRIGHTNESS,
        ) / 255f
    }.getOrDefault(0.5f)
}

internal fun PlayerComposeActivity.currentMediaVolume(): Float {
    val am = audioManager()
    val max = am.getStreamMaxVolume(android.media.AudioManager.STREAM_MUSIC)
    if (max <= 0) return 0f
    return am.getStreamVolume(android.media.AudioManager.STREAM_MUSIC).toFloat() / max
}

internal fun PlayerComposeActivity.audioManager(): android.media.AudioManager =
    getSystemService(android.content.Context.AUDIO_SERVICE) as android.media.AudioManager