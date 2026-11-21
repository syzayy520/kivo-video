package com.kivo.cinema.ui.compose.player.seek

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.compose.player.seekPreviewHeight
import com.kivo.cinema.ui.compose.player.seekPreviewWidth

/**
 * 轨道盒窗口坐标 → 预览卡落位的唯一几何来源。
 *
 * 卡挂在 overlay 根 Box 而不是轨道盒内：真机 25–32 号证据里，
 * 轨道盒（BoxWithConstraints）内的 align/offset/padding/自布局四种落位
 * 都被恒定加偏（约 ±60dp），根 Box 的 align+padding（HUD/toast 同款）实测正确。
 */
internal class PlayerSeekPreviewAnchor {
    var trackBounds by mutableStateOf<Rect?>(null)
}

/** 返回卡左上角 (left, top)：中心跟滑块、两端夹在轨道内、底边贴轨道上沿 10dp。 */
internal fun seekPreviewLeftTop(
    track: Rect,
    durationMs: Long,
    targetMs: Long,
    showFrame: Boolean,
    density: Float,
): Pair<Dp, Dp> {
    val widthDp = seekPreviewWidth(durationMs, (track.width / density).dp)
    val widthPx = widthDp.value * density
    val heightPx = seekPreviewHeight(widthDp, showFrame).value * density
    val inset = 7f * density
    val f = if (durationMs > 0L) targetMs.toFloat() / durationMs else 0f
    val centerX = track.left + inset + (track.width - 2f * inset) * f.coerceIn(0f, 1f)
    val left = (centerX - widthPx / 2f)
        .coerceIn(track.left, (track.right - widthPx).coerceAtLeast(track.left))
    val top = (track.top - 10f * density - heightPx).coerceAtLeast(0f)
    return (left / density).dp to (top / density).dp
}