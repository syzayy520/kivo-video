package com.kivo.cinema.ui.compose.player

import android.graphics.Bitmap
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.scaleIn
import androidx.compose.animation.scaleOut
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
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp
import kotlin.math.roundToInt

/**
 * 进度条轨道（v3 §1.2）。
 *
 * 实测：x[110,740]、**厚 6pt**、圆角端、**完全没有滑块**；
 * 三段＝已播 #FFFFFF / 缓冲 ≈#9B9B9B 实色 / 未播＝黑 45% 罩视频。
 * v2 的 3.5pt 细轨 + 拖动时 8pt 圆点都被推翻 —— 参考没有圆点，
 * 6pt 已经足够看清，加圆点反而多一个参考里不存在的零件。
 *
 * 【平滑度：按帧插值，不按轮询】内核位置 100ms 采样一次（省电的常规做法），
 * 直接用采样值 ⇒ 进度条是 10fps 的跳变。这里用 [withFrameNanos] 按帧插值
 * ⇒ **60fps 平滑移动**，轮询频率不变。时间文字仍用采样值（只显示整秒）。
 *
 * 【缓冲段从哪来】mpv `demuxer-cache-time`＝已缓冲但尚未播放的秒数，
 * 由 1Hz 的采样换算成 fraction 传进来 —— 不是 UI 自己估的进度。
 */
@Composable
internal fun PlayerSeekTrack(
    positionMs: Long,
    durationMs: Long,
    bufferedFraction: Float,
    thumbnailBitmap: Bitmap?,
    thumbUnavailable: String,
    previewMs: Long,
    scrubPreview: Boolean,
    onDragTarget: (Long?) -> Unit,
    onScrub: (Long) -> Unit,
    onSeek: (Long) -> Unit,
    /**
     * 拖动**未落到 seek**时也要走一次（取消 / 片长为 0）。
     * 拖动期间弹幕让位（scrubbing），只有 onSeek 会复位它 ——
     * 取消时若不复位，弹幕会永久消失（272 修）。
     */
    onScrubEnd: () -> Unit,
    onCapture: () -> Unit,
    modifier: Modifier = Modifier,
) {
    var dragging by remember { mutableStateOf(false) }
    var dragFraction by remember { mutableFloatStateOf(0f) }
    var lastTarget by remember { mutableLongStateOf(0L) }

    // 选中态（v3.1）：按下或拖动 ⇒ 轨道 6→10pt；松手回 6。
    var pressed by remember { mutableStateOf(false) }
    val thickness by animateFloatAsState(
        targetValue = if (pressed || dragging) PlayerTokens.trackActiveHeight else PlayerTokens.trackHeight,
        animationSpec = tween(PlayerMotion.trackMs, easing = PlayerMotion.easing),
        label = "trackThickness",
    )

    var anchorMs by remember { mutableLongStateOf(System.currentTimeMillis()) }
    LaunchedEffect(positionMs) { anchorMs = System.currentTimeMillis() }

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
                    smoothFraction = ((posState.value + elapsed).toFloat() / d).coerceIn(0f, 1f)
                }
            }
        }
    }

    val density = LocalDensity.current

    // 拖动中的统一处理：算 fraction + 抛目标时刻 + 要缩略图。
    fun updateDrag(x: Float, width: Int) {
        val d = durState.value
        if (d <= 0 || width <= 0) return
        val f = (x / width).coerceIn(0f, 1f)
        dragFraction = f
        val ms = (f * d).roundToInt().toLong()
        lastTarget = ms
        onDragTarget(ms)
        onScrub(ms)
    }

    BoxWithConstraints(
        modifier = modifier
            .fillMaxWidth()
            .height(PlayerTokens.trackHitHeight.dp)
            // key 用 Unit 而非 durationMs：SMB 大文件的时长会在拖中途被内核
            // 修正，key 变化会重建 pointerInput ⇒ 进行中的拖动被 cancel
            // （真机：浮窗消失、标签跳回拖前位置）。时长经 durState 读最新值。
            .pointerInput(Unit) {
                detectTapGestures(
                    onPress = {
                        pressed = true
                        tryAwaitRelease()
                        pressed = false
                    },
                    onTap = { offset ->
                        val d = durState.value
                        if (d > 0) {
                            val f = (offset.x / size.width).coerceIn(0f, 1f)
                            onSeek((f * d).roundToInt().toLong())
                        }
                    },
                )
            }
            .pointerInput(Unit) {
                detectHorizontalDragGestures(
                    onDragStart = { offset -> dragging = true; updateDrag(offset.x, size.width) },
                    onHorizontalDrag = { change, _ -> updateDrag(change.position.x, size.width) },
                    onDragEnd = {
                        dragging = false
                        onDragTarget(null)
                        if (durState.value > 0) onSeek(lastTarget) else onScrubEnd()
                    },
                    onDragCancel = {
                        dragging = false
                        onDragTarget(null)
                        // 取消也要复位：拖动期间弹幕让位，拖动没落 seek 就没人复位它。
                        onScrubEnd()
                    },
                )
            },
    ) {
        AnimatedVisibility(
            visible = (dragging || previewMs >= 0) && scrubPreview,
            enter = fadeIn(tween(PlayerMotion.previewEnterMs, easing = PlayerMotion.easing)) +
                scaleIn(tween(PlayerMotion.previewEnterMs, easing = PlayerMotion.easing), initialScale = 0.92f),
            exit = fadeOut(tween(PlayerMotion.previewExitMs)) +
                scaleOut(tween(PlayerMotion.previewExitMs), targetScale = 0.92f),
        ) {
            val shown = if (dragging) {
                dragFraction
            } else if (durationMs > 0) {
                (previewMs.toFloat() / durationMs).coerceIn(0f, 1f)
            } else {
                0f
            }
            PlayerSeekPreview(
                bitmap = thumbnailBitmap,
                unavailableReason = thumbUnavailable,
                fraction = shown,
                targetMs = if (dragging) lastTarget else previewMs,
                containerWidth = maxWidth,
                onCapture = onCapture,
            )
        }

        Canvas(
            modifier = Modifier
                .fillMaxWidth()
                .height(PlayerTokens.trackHitHeight.dp),
        ) {
            val h = with(density) { thickness.dp.toPx() }
            val y = (size.height - h) / 2f
            val radius = CornerRadius(h / 2f)
            val f = if (dragging) dragFraction else smoothFraction
            // 玻璃质感＝半透明填充 + 1px 提亮描边（v3.2 实测），不是实色色块。
            val rim = with(density) { 1.dp.toPx() }

            // 未播段（拖动中整体提亮，让用户看清"松手会落到哪一段"）
            drawRoundRect(
                color = if (dragging) PlayerTokens.trackDragging else PlayerTokens.trackRemaining,
                topLeft = Offset(0f, y),
                size = Size(size.width, h),
                cornerRadius = radius,
            )
            drawRoundRect(
                color = if (dragging) PlayerTokens.trackDraggingRim else PlayerTokens.trackRemainingRim,
                topLeft = Offset(0f, y),
                size = Size(size.width, h),
                cornerRadius = radius,
                style = Stroke(rim),
            )
            // 缓冲段（已缓冲未播放）：半透明白 + 更亮的边，画在未播之上、已播之下
            val bf = bufferedFraction.coerceIn(f, 1f)
            if (bf > f) {
                drawRoundRect(
                    color = PlayerTokens.trackBuffered,
                    topLeft = Offset(size.width * f, y),
                    size = Size(size.width * (bf - f), h),
                    cornerRadius = radius,
                )
                drawRoundRect(
                    color = PlayerTokens.trackBufferedRim,
                    topLeft = Offset(size.width * f, y),
                    size = Size(size.width * (bf - f), h),
                    cornerRadius = radius,
                    style = Stroke(rim),
                )
            }
            // 已播段（在 draw 里读状态 ⇒ 只重绘 Canvas，不重组整棵树）
            if (f > 0f) {
                drawRoundRect(
                    color = PlayerTokens.trackPlayed,
                    topLeft = Offset(0f, y),
                    size = Size(size.width * f, h),
                    cornerRadius = radius,
                )
            }
        }
    }
}