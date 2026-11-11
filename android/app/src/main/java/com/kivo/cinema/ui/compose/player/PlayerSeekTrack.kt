package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.gestures.detectHorizontalDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp
import kotlin.math.roundToInt

/**
 * 进度条轨道（player-ui-v2.md §3.2）。
 *
 * ## 形态：细轨 + **默认不显示滑块**
 * Material3 的 `Slider` 那个大圆头滑块在横屏下太重、而且会盖住画面。
 * 参考（REX）用的是 3.5pt 细轨、**完全没有滑块**。
 * 我们比参考多一步：**拖动时才出现一个 8dp 小圆点** ——
 * 参考的用户不知道往哪按；圆点只在拖动期间出现，不干扰观看。
 *
 * ## 交互
 * · 单击轨道 ⇒ 跳到该位置（`onSeek`）
 * · 横向拖动 ⇒ 实时回报位置（`onScrub`），松手才真正 seek（`onSeek`）
 */
@Composable
internal fun PlayerSeekTrack(
    fraction: Float,
    durationMs: Long,
    dragging: Boolean,
    onScrub: (Long) -> Unit,
    onSeek: (Long) -> Unit,
    onDragStart: () -> Unit,
    onDragEnd: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val density = LocalDensity.current
    Canvas(
        modifier = modifier
            .fillMaxWidth()
            .height(24.dp)
            .pointerInput(durationMs) {
                detectTapGestures(
                    onTap = { offset ->
                        if (durationMs > 0) {
                            val f = (offset.x / size.width).coerceIn(0f, 1f)
                            onSeek((f * durationMs).roundToInt().toLong())
                        }
                    },
                )
            }
            .pointerInput(durationMs) {
                detectHorizontalDragGestures(
                    onDragStart = { offset ->
                        onDragStart()
                        if (durationMs > 0) {
                            val f = (offset.x / size.width).coerceIn(0f, 1f)
                            onScrub((f * durationMs).roundToInt().toLong())
                        }
                    },
                    onHorizontalDrag = { change, _ ->
                        if (durationMs > 0) {
                            val f = (change.position.x / size.width).coerceIn(0f, 1f)
                            onScrub((f * durationMs).roundToInt().toLong())
                        }
                    },
                    onDragEnd = {
                        onDragEnd()
                    },
                    onDragCancel = { onDragEnd() },
                )
            },
    ) {
        // 轨道在 24dp 高的画布里垂直居中（3.5dp 厚 ⇒ 上下各留 10.25dp）
        val trackHeight = with(density) { PlayerTokens.trackHeight.dp.toPx() }
        val trackY = (size.height - trackHeight) / 2f
        val radius = CornerRadius(trackHeight / 2f)
        // 未播放段
        drawRoundRect(
            color = PlayerTokens.trackRemaining,
            topLeft = Offset(0f, trackY),
            size = Size(size.width, trackHeight),
            cornerRadius = radius,
        )
        // 已播放段
        drawRoundRect(
            color = PlayerTokens.trackPlayed,
            topLeft = Offset(0f, trackY),
            size = Size(size.width * fraction, trackHeight),
            cornerRadius = radius,
        )
        // 拖动中的滑块（默认不可见 —— 见文件头说明）
        if (dragging) {
            val cx = size.width * fraction
            drawCircle(
                color = PlayerTokens.trackPlayed,
                radius = with(density) { PlayerTokens.thumbSize.dp.toPx() } / 2f,
                center = Offset(cx, trackY + trackHeight / 2f),
            )
        }
    }
}
