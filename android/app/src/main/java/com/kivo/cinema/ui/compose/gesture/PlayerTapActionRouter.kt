package com.kivo.cinema.ui.compose

import com.kivo.cinema.ui.compose.player.DoubleTapZone

/**
 * 双击动作路由：左=快退 / 中=播放暂停 / 右=快进，步长与动作由操控设置决定。
 * 光痕由 flashEventId 驱动（同侧重复双击每次重播）；清除带守卫，旧事件不清新事件。
 */
internal fun PlayerComposeActivity.onDoubleTap(zone: DoubleTapZone) {
    if (screenLocked && !playbackPrefs.lockAllowGestures) return
    val action = when (zone) {
        DoubleTapZone.FORWARD, DoubleTapZone.BACKWARD -> playbackPrefs.doubleTapSidesAction
        DoubleTapZone.TOGGLE_PLAY -> playbackPrefs.doubleTapCenterAction
    }
    if (action == "none") return
    lastTouchMs = System.currentTimeMillis()
    flashZone = zone
    val event = ++flashEventId
    android.os.Handler(mainLooper).postDelayed({
        if (flashEventId == event) flashZone = null
    }, 600)
    when (zone) {
        // 边界处 seekTo 会把目标夹回端点 ⇒ 内核收到"seek 到当前位置"静默不动。
        // 光痕照播会误导成"跳了"，所以边界必须给可见原因（不静默）。
        DoubleTapZone.FORWARD ->
            if (durationMs > 0L && positionMs >= durationMs - 250L) showToast("已到片尾，无法再快进")
            else seekTo(positionMs + playbackPrefs.seekForwardSeconds * 1000L)
        DoubleTapZone.BACKWARD ->
            if (positionMs <= 250L) showToast("已到片头，无法再快退")
            else seekTo(positionMs - playbackPrefs.seekBackwardSeconds * 1000L)
        DoubleTapZone.TOGGLE_PLAY -> togglePlayPause()
    }
}