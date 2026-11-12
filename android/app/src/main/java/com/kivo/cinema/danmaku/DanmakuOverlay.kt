package com.kivo.cinema.danmaku

import androidx.compose.foundation.Canvas
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameMillis
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.drawscope.drawIntoCanvas
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.platform.LocalDensity
import kotlin.math.min

/**
 * 弹幕滚动层（223 设计稿 D2 / v3.1 §四）：Compose Canvas overlay。
 *
 * 【v3.1 修掉的四处】
 * · 设置真生效：[DanmakuSettings] 的字号/不透明度/区域直接进绘制；
 * · mode 4/5（底部/顶部固定弹幕）不再被当滚动弹幕画 —— 居中钉住 4s；
 * · 文字宽度在**设好 textSize 之后**量（旧版用默认字号量宽，落点偏左）；
 * · 位置用连续媒体钟：positionMs 是 ~4Hz 轮询值，直接用它画会一顿一顿；
 *   显示值每帧向"轮询锚点 + 墙钟差值"渐进收敛（seek 才瞬移），暂停钉在真值上。
 *
 * 帧循环只在可见且有弹幕时组合（早退在 LaunchedEffect 之前），隐藏时零帧耗。
 */
@Composable
fun DanmakuOverlay(
    items: List<DanmakuItem>,
    positionMs: Long,
    isPlaying: Boolean,
    visible: Boolean,
    settings: DanmakuSettings,
    modifier: Modifier = Modifier,
) {
    if (!visible || items.isEmpty()) return

    var frameTime by remember { mutableStateOf(0L) }
    LaunchedEffect(Unit) {
        while (true) {
            withFrameMillis { frameTime = it }
        }
    }

    val density = LocalDensity.current.density
    val paint = remember { android.graphics.Paint().apply { isAntiAlias = true } }
    val clock = remember { DanmakuClock() }
    val baseTextSize = 14f * density * settings.fontScale
    val alpha = (255 * settings.opacity.coerceIn(0f, 1f)).toInt()

    Canvas(modifier = modifier) {
        // 连续媒体钟：轮询值只作锚点，不作显示值。
        // 旧版每次轮询（~4Hz）把墙钟锚点重置一次，轮询抖动直接变成位置跳变
        // ⇒ 肉眼可见的顿挫。这里显示值每帧向目标**渐进收敛**（slew），
        // 抖动被抹成速度微变；只有 seek（偏差 >1s）才瞬移。
        if (positionMs != clock.pollPosMs) {
            val expected = clock.pollPosMs +
                if (isPlaying) (frameTime - clock.pollWallMs).coerceAtLeast(0L) else 0L
            if (clock.shownMs < 0f || kotlin.math.abs(positionMs - expected) > 1000L) {
                clock.shownMs = positionMs.toFloat()
            }
            clock.pollPosMs = positionMs
            clock.pollWallMs = frameTime
        }
        val target = clock.pollPosMs +
            if (isPlaying) (frameTime - clock.pollWallMs).coerceAtLeast(0L) else 0L
        clock.shownMs += (target - clock.shownMs) * SLEW_PER_FRAME
        val nowSec = clock.shownMs / 1000f

        val canvasWidth = size.width
        val canvasHeight = size.height
        val trackHeight = baseTextSize * 1.6f

        // 滚动区域：全屏 / 上半 / 下半。固定弹幕（4/5）不受区域限制。
        val regionTop = if (settings.area == DanmakuArea.BOTTOM_HALF) canvasHeight / 2f else 0f
        val regionBottom = if (settings.area == DanmakuArea.TOP_HALF) canvasHeight / 2f else canvasHeight
        val maxTracks = min(((regionBottom - regionTop) / trackHeight).toInt(), 12).coerceAtLeast(1)
        val fixedSlots = 4

        for (item in items) {
            val age = nowSec - item.time
            paint.textSize = baseTextSize * (item.size / 25f)
            val textW = paint.measureText(item.text)

            val x: Float
            val y: Float
            when (item.mode) {
                4, 5 -> {
                    // 固定弹幕：居中钉住 4s，槽位按 hash 散开避免叠字。
                    if (age < 0 || age > FIXED_LIFE_SEC) continue
                    val slot = (item.hashCode().toUInt() % fixedSlots.toUInt()).toInt()
                    x = (canvasWidth - textW) / 2f
                    y = if (item.mode == 5) {
                        slot * trackHeight + paint.textSize
                    } else {
                        canvasHeight - slot * trackHeight - trackHeight * 0.2f
                    }
                }
                else -> {
                    if (age < 0 || age > settings.speedSec) continue
                    val progress = age / settings.speedSec
                    x = canvasWidth - progress * (canvasWidth + textW)
                    val trackIdx = (item.hashCode().toUInt() % maxTracks.toUInt()).toInt()
                    y = regionTop + trackIdx * trackHeight + paint.textSize
                }
            }

            drawIntoCanvas { canvas ->
                // 黑描边 + 彩色填充，不透明度对两者一起生效。
                paint.style = android.graphics.Paint.Style.STROKE
                paint.strokeWidth = 3f * density
                paint.color = android.graphics.Color.BLACK
                paint.alpha = alpha
                canvas.nativeCanvas.drawText(item.text, x, y, paint)
                paint.style = android.graphics.Paint.Style.FILL
                paint.color = android.graphics.Color.rgb(
                    (item.color shr 16) and 0xFF,
                    (item.color shr 8) and 0xFF,
                    item.color and 0xFF,
                )
                paint.alpha = alpha
                canvas.nativeCanvas.drawText(item.text, x, y, paint)
            }
        }
    }
}

/** 固定弹幕（mode 4/5）的停留时长，B 站默认 4s。 */
private const val FIXED_LIFE_SEC = 4f

/** 每帧收敛系数：60fps 下约 4 帧收敛，抹平轮询抖动又不拖尾。 */
private const val SLEW_PER_FRAME = 0.25f

/** 连续媒体钟：轮询锚点 + 显示值（显示值每帧向目标渐进收敛）。 */
private class DanmakuClock {
    var pollPosMs = -1L
    var pollWallMs = 0L
    var shownMs = -1f
}