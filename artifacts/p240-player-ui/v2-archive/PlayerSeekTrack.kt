package com.kivo.cinema.ui.compose.player

import android.graphics.Bitmap
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.gestures.detectHorizontalDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableLongStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp
import kotlin.math.roundToInt

/**
 * 进度条轨道（player-ui-v2.md §3.2）。
 *
 * ## 形态：细轨 + **默认不显示滑块**
 * 参考用的是 3.5pt 细轨、**完全没有滑块**。
 * 我们比参考多一步：**拖动时才出现一个 8pt 小圆点** ——
 * 参考的用户不知道往哪按；圆点只在拖动期间出现，不干扰观看。
 *
 * ## 平滑度：进度条按**帧**走，不是按轮询走
 * 内核位置每 100ms 采样一次（轮询是省电的常规做法），
 * 若进度条直接用采样值，它就是 **10fps 的跳变** —— 看起来"一格一格"。
 * 这里用 [withFrameNanos] 按帧把位置**插值**出来 ⇒ 进度条 **60fps 平滑移动**，
 * 而轮询频率不变（省电不受影响）。
 * 时间文字仍用采样值 —— 它只显示整秒，一秒才变一次，视觉上无跳变。
 */
@Composable
internal fun PlayerSeekTrack(
    positionMs: Long,
    durationMs: Long,
    thumbnailBitmap: Bitmap?,
    previewMs: Long,
    onDragTarget: (Long?) -> Unit,
    onScrub: (Long) -> Unit,
    onSeek: (Long) -> Unit,
    modifier: Modifier = Modifier,
) {
    var dragging by remember { mutableStateOf(false) }
    var dragFraction by remember { mutableFloatStateOf(0f) }

    // 拖动目标时刻（REX：拖动中左端实时显示目标时间）。
    // 用回调把目标**抛给父级**显示 —— 父级才知道该显示在哪（左端时间位）。
    var lastTarget by remember { mutableLongStateOf(0L) }

    // 采样时刻：位置**每次变化**时记一次（比帧循环晚 ~1 帧，误差可忽略）。
    // 有了它就能按帧插值，而不是"跟着 100ms 的采样一跳一跳"。
    var anchorMs by remember { mutableLongStateOf(System.currentTimeMillis()) }
    LaunchedEffect(positionMs) { anchorMs = System.currentTimeMillis() }

    // 帧循环里读最新值：用 rememberUpdatedState 避免每 100ms 重建协程。
    val posState = rememberUpdatedState(positionMs)
    val anchorState = rememberUpdatedState(anchorMs)
    val durState = rememberUpdatedState(durationMs)
    var smoothFraction by remember { mutableFloatStateOf(0f) }

    LaunchedEffect(Unit) {
        while (true) {
            withFrameNanos {
                val d = durState.value
                if (d > 0 && !dragging) {
                    val elapsed = (System.currentTimeMillis() - anchorState.value).coerceAtLeast(0L)
                    val ms = posState.value + elapsed
                    smoothFraction = (ms.toFloat() / d).coerceIn(0f, 1f)
                }
            }
        }
    }

    val density = LocalDensity.current
    BoxWithConstraints(
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
                        dragging = true
                        if (durationMs > 0) {
                            val f = (offset.x / size.width).coerceIn(0f, 1f)
                            dragFraction = f
                            val ms = (f * durationMs).roundToInt().toLong()
                            lastTarget = ms
                            onDragTarget(ms)
                            onScrub(ms)
                        }
                    },
                    onHorizontalDrag = { change, _ ->
                        if (durationMs > 0) {
                            val f = (change.position.x / size.width).coerceIn(0f, 1f)
                            dragFraction = f
                            val ms = (f * durationMs).roundToInt().toLong()
                            lastTarget = ms
                            onDragTarget(ms)
                            onScrub(ms)
                        }
                    },
                    onDragEnd = {
                        dragging = false
                        onDragTarget(null)
                        if (durationMs > 0) {
                            onSeek(lastTarget)
                        }
                    },
                    onDragCancel = {
                        dragging = false
                        onDragTarget(null)
                    },
                )
            },
    ) {
        // 预览浮窗（拖动中）：向上挂在进度条上方，不占布局高度
        val previewing = previewMs >= 0
        if (dragging || previewing) {
            val shownFraction = if (dragging) dragFraction else {
                if (durationMs > 0) (previewMs.toFloat() / durationMs).coerceIn(0f, 1f) else 0f
            }
            PlayerSeekPreview(
                bitmap = thumbnailBitmap,
                fraction = shownFraction,
                containerWidth = maxWidth,
            )
        }

        Canvas(
            modifier = Modifier
                .fillMaxWidth()
                .height(24.dp),
        ) {
            val trackHeight = with(density) { PlayerTokens.trackHeight.dp.toPx() }
            val trackY = (size.height - trackHeight) / 2f
            val radius = CornerRadius(trackHeight / 2f)
            // 未播放段（**拖动中整体变亮** —— 参考：拖动中轨道变白，
            // 让用户看清"松手后会落到哪一段"）
            drawRoundRect(
                color = if (dragging) PlayerTokens.trackDragging else PlayerTokens.trackRemaining,
                topLeft = Offset(0f, trackY),
                size = Size(size.width, trackHeight),
                cornerRadius = radius,
            )
            // 已播放段（在 draw 里读状态 ⇒ 只重绘 Canvas，不重组整棵树）
            val f = if (dragging) dragFraction else smoothFraction
            drawRoundRect(
                color = PlayerTokens.trackPlayed,
                topLeft = Offset(0f, trackY),
                size = Size(size.width * f, trackHeight),
                cornerRadius = radius,
            )
            // 拖动中的滑块（默认不可见）
            if (dragging) {
                drawCircle(
                    color = PlayerTokens.trackPlayed,
                    radius = with(density) { PlayerTokens.thumbSize.dp.toPx() } / 2f,
                    center = Offset(size.width * f, trackY + trackHeight / 2f),
                )
            }
        }
    }
}
