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
    scheduleHudHide()
}

/** 安排 HUD 在 [PlayerMotion.hudHoldMs] 后淡出（横竖滑共用，317）。 */
private fun PlayerComposeActivity.scheduleHudHide() {
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
    // 317：横滑 seek 的 HUD 比例由目标时刻算，这里不参与。
    PlayerHudKind.SEEK_FORWARD, PlayerHudKind.SEEK_BACKWARD -> 0f
}

internal fun PlayerComposeActivity.applyHudValue(kind: PlayerHudKind, value: Float) {
    when (kind) {
        // 317：seek 不改系统值（真正的跳转在 onHorizontalDragEnd 里）。
        PlayerHudKind.SEEK_FORWARD, PlayerHudKind.SEEK_BACKWARD -> Unit
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

// ---- 317 · 全屏横滑 = 快进/快退 ----

/** 实时 seek 的节流间隔（毫秒）。 */
private const val REALTIME_SEEK_INTERVAL_MS = 150L

/**
 * 横滑开始：记下起点，HUD 立刻接管。
 *
 * 【状态从哪来】[hSeek] 由调用方 `remember { HandSeekState() }` 持有（B4-T3）。
 * 旧版这四个字段是**顶层 `private var`**（进程级），第二次进播放器会沿用上次的起点/目标
 * ⇒ "第一次横滑没反应"或"一滑就跳到上次的位置"。状态随 Composition 生死，这里只做数学。
 *
 * 【为什么按"时长的 10%"定比例尺】固定秒数对短片太粗（一屏就过半）、对长片太慢
 * （两小时的片滑到手酸）。取时长 10% 并要求全屏宽走完，再夹在 [15s, 300s]：
 * · 2 分钟短片 ⇒ 15s（下限，够细）
 * · 30 分钟     ⇒ 180s
 * · 2 小时      ⇒ 300s（上限，一屏 5 分钟）
 */
internal fun PlayerComposeActivity.onHorizontalDragStart(hSeek: HandSeekState) {
    if (!playbackPrefs.horizontalSeekEnabled) return
    if (durationMs <= 0) return
    hSeek.begin(positionMs)
    // 【必须显示控制栏】缩略图浮窗挂在底部进度条上（PlayerSeekTrack 的 AnimatedVisibility），
    // 控制栏隐藏时浮窗也跟着不在 ⇒ 横滑就看不到预览画面。这里强制拉起，
    // 松手后由 lastTouchMs 的 3s 自动隐藏逻辑收掉。
    controlsVisible = true
    lastTouchMs = System.currentTimeMillis()
    cancelHudHide()
    hudKind = PlayerHudKind.SEEK_FORWARD
    hudFraction = (positionMs.toFloat() / durationMs).coerceIn(0f, 1f)
    hudSeekTargetMs = positionMs
}

/** 横滑过程：只更新目标与 HUD，**不 seek**（与进度条拖动同口径）。 */
internal fun PlayerComposeActivity.onHorizontalDrag(hSeek: HandSeekState, dx: Float) {
    if (!playbackPrefs.horizontalSeekEnabled) return
    // D7-1：用 active 判是否进行中；startMs==0（片头）现在是合法起点，不再被吞。
    if (!hSeek.active || durationMs <= 0L) return
    val w = resources.displayMetrics.widthPixels.toFloat().coerceAtLeast(1f)
    val spanMs = (durationMs * 0.1f).coerceIn(15_000f, 300_000f)
    hSeek.accumMs += (dx / w * spanMs).toLong()
    hSeek.targetMs = (hSeek.startMs + hSeek.accumMs).coerceIn(0L, durationMs)
    hudKind = if (hSeek.accumMs >= 0) PlayerHudKind.SEEK_FORWARD else PlayerHudKind.SEEK_BACKWARD
    hudFraction = (hSeek.targetMs.toFloat() / durationMs).coerceIn(0f, 1f)
    hudSeekTargetMs = hSeek.targetMs
    hudSeekDeltaMs = hSeek.accumMs

    // ★实时预览（对齐 SenPlayer）：复用进度条拖动那套缩略图浮窗。
    // `previewMs` 是 PlayerSeekTrack 里浮窗的显示条件（>=0 即显示），
    // 它同时决定浮窗比例位置 —— 所以横滑和拖进度条看到的是同一个浮窗。
    previewMs = hSeek.targetMs
    thumbnailReader?.seekTo(hSeek.targetMs * 1000)

    // ★实时跳转（对齐 SenPlayer 的"边滑边看"）。
    //
    // 【为什么节流 150ms】每帧都 seek 会把内核的落点预取打爆（每次 seek 都触发
    // prefetchSeek + 内核跳读），而 6~7 次/秒已经足够让画面跟手。
    // 【为什么不等落点】实时跟手要的是"立刻有反应"，等落点那套（v42）会让
    // 每次跳都卡 100~1200ms ⇒ 拖动变成幻灯片。最终落点在松手时补一次精确 seek。
    val now = android.os.SystemClock.uptimeMillis()
    if (now - hSeek.lastAt >= REALTIME_SEEK_INTERVAL_MS) {
        hSeek.lastAt = now
        seekTo(hSeek.targetMs, waitLandingFirstSlice = false)
    }
}

/** 横滑结束：commit=true 才真的跳；取消则什么都不做。 */
internal fun PlayerComposeActivity.onHorizontalDragEnd(hSeek: HandSeekState, commit: Boolean) {
    if (!playbackPrefs.horizontalSeekEnabled) return
    val target = hSeek.targetMs
    // 先归零再 seek：即使下面抛异常，状态也已回到"没有进行中的横滑"。
    hSeek.clear()
    // 收起预览浮窗（与拖进度条松手同一口径）。
    previewMs = -1L
    if (commit && durationMs > 0L) {
        // 松手补一次**精确** seek：带落点等待，消除实时节流带来的最后一点偏差。
        seekTo(target, waitLandingFirstSlice = true)
    }
    scheduleHudHide()
}
