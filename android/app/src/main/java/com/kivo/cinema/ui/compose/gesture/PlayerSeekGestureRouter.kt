package com.kivo.cinema.ui.compose

import android.os.SystemClock
import kotlin.math.roundToLong

internal fun PlayerComposeActivity.updateScrub(targetMs: Long) {
    if (durationMs <= 0L) return
    val starting = !scrub.active
    if (starting) scrub.begin(positionMs.coerceIn(0L, durationMs))
    val target = targetMs.coerceIn(0L, durationMs)
    lastTouchMs = System.currentTimeMillis()
    scrubbing = true
    if (!starting && scrub.targetMs == target) return
    scrub.update(target)
    previewMs = target
    previewRequestedMs = -1L
    thumbnailReader?.invalidate()
    thumbnailBitmap = null
    thumbnailUnavailableReason = if (thumbnailReader == null) "此片源暂不支持预览帧" else "预览帧准备中…"
    urlPrep?.source?.yieldSource(1500L)
}

internal fun PlayerComposeActivity.commitScrub(targetMs: Long) {
    if (!scrub.active || durationMs <= 0L) return
    val target = targetMs.coerceIn(0L, durationMs)
    updateScrub(target)
    scrubbing = false
    scrub.finish()
    seekTo(target, waitLandingFirstSlice = true)
    if (playbackPrefs.scrubPreview) {
        previewRequestedMs = target
        thumbnailReader?.seekTo(target * 1000)
    }
}

internal fun PlayerComposeActivity.cancelScrub() {
    scrubbing = false
    scrub.cancel()
    thumbnailReader?.invalidate()
    thumbnailBitmap = null
    previewMs = -1L
    previewRequestedMs = -1L
}

internal fun PlayerComposeActivity.onHorizontalDragStart(hSeek: HandSeekState) {
    if (!playbackPrefs.horizontalSeekEnabled) return
    if (durationMs <= 0L) {
        showToast("时长尚未就绪，暂不能调整进度")
        return
    }
    hSeek.begin(positionMs.coerceIn(0L, durationMs))
    controlsVisible = true
    updateScrub(hSeek.startMs)
}

internal fun PlayerComposeActivity.onHorizontalDrag(hSeek: HandSeekState, dx: Float) {
    if (!hSeek.active || !scrub.active || durationMs <= 0L) return
    val width = window.decorView.width.toFloat().coerceAtLeast(1f)
    val spanMs = (durationMs * 0.1).coerceIn(15_000.0, 300_000.0)
    // 每步从已夹紧的落点推进；边界外继续拖动不会积成反向死区。
    hSeek.targetMs = (hSeek.targetMs + (dx / width * spanMs).roundToLong()).coerceIn(0L, durationMs)
    hSeek.accumMs = hSeek.targetMs - hSeek.startMs
    updateScrub(hSeek.targetMs)
    val now = SystemClock.uptimeMillis()
    if (now - hSeek.lastAt >= 150L) {
        hSeek.lastAt = now
        seekTo(hSeek.targetMs, waitLandingFirstSlice = false)
    }
}

internal fun PlayerComposeActivity.onHorizontalDragEnd(hSeek: HandSeekState, commit: Boolean) {
    if (!hSeek.active) return
    val target = hSeek.targetMs
    hSeek.clear()
    if (commit) commitScrub(target) else cancelScrub()
}
