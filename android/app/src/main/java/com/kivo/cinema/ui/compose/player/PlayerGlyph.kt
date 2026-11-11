package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.Canvas
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.PathOperation
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.scale

/**
 * 播放页图标族（player-ui-v2.md 边界规则 P2）。
 *
 * **为什么自己画而不是用 Material 图标**：
 * REX 的图标全是**线框**（stroke），而 Material 的 `Icons.Filled.*` 是**实心**。
 * 两种形态混在一排里会显得脏 —— 这是"照抄结构但不照抄 iOS 惯例"的一个具体落点：
 * 结构照搬 REX，形态按 Android 的统一性重画。
 *
 * 画法沿用项目里 `ui/design/home/NavGlyph.kt` 的既有风格（Canvas + Path），
 * 不引入任何图标库依赖。
 *
 * 坐标系：以 24×24 为基准画布，外部按目标尺寸缩放。
 */
@Composable
internal fun PlayerGlyph(
    kind: String,
    color: Color,
    modifier: Modifier = Modifier,
    strokeWidth: Float = 1.9f,
) {
    Canvas(modifier) {
        scale(size.width / 24f, size.height / 24f, pivot = Offset.Zero) {
            val stroke = Stroke(strokeWidth, cap = StrokeCap.Round)

            when (kind) {
                // ---- 选中勾 ----
                "check" -> {
                    drawLine(color, Offset(4.5f, 12.5f), Offset(9.5f, 17.5f), strokeWidth, StrokeCap.Round)
                    drawLine(color, Offset(9.5f, 17.5f), Offset(19.5f, 6.5f), strokeWidth, StrokeCap.Round)
                }

                // ---- 关闭（X）----
                "close" -> {
                    drawLine(color, Offset(5f, 5f), Offset(19f, 19f), strokeWidth, StrokeCap.Round)
                    drawLine(color, Offset(19f, 5f), Offset(5f, 19f), strokeWidth, StrokeCap.Round)
                }

                // ---- 播放 / 暂停（实心，这是通用约定，不做线框）----
                "play" -> {
                    val tri = Path().apply {
                        moveTo(7.5f, 4.5f); lineTo(19.5f, 12f); lineTo(7.5f, 19.5f); close()
                    }
                    drawPath(tri, color)
                }

                "pause" -> {
                    drawRoundRect(
                        color, Offset(6.5f, 4.5f), Size(4f, 15f),
                        androidx.compose.ui.geometry.CornerRadius(1.2f),
                    )
                    drawRoundRect(
                        color, Offset(13.5f, 4.5f), Size(4f, 15f),
                        androidx.compose.ui.geometry.CornerRadius(1.2f),
                    )
                }

                // ---- 下一个（▷|）----
                "next" -> {
                    val tri = Path().apply {
                        moveTo(5f, 5f); lineTo(15f, 12f); lineTo(5f, 19f); close()
                    }
                    drawPath(tri, color)
                    drawRoundRect(color, Offset(16.5f, 5f), Size(2.6f, 14f), androidx.compose.ui.geometry.CornerRadius(1.2f))
                }

                // ---- 画质比例（方框 + 上下箭头，REX 的第一个图标）----
                "aspect" -> {
                    drawRoundRect(
                        color, Offset(3.5f, 5.5f), Size(17f, 13f),
                        androidx.compose.ui.geometry.CornerRadius(2f), style = stroke,
                    )
                    // 上下两条横线（表示比例）
                    drawLine(color, Offset(7.5f, 5.5f), Offset(7.5f, 18.5f), 1.5f, StrokeCap.Round)
                }

                // ---- 音频（声波）----
                "audio" -> {
                    drawRoundRect(
                        color, Offset(3.5f, 9.5f), Size(4f, 5f),
                        androidx.compose.ui.geometry.CornerRadius(1.5f),
                    )
                    drawArc(
                        color = color,
                        startAngle = -60f,
                        sweepAngle = 120f,
                        useCenter = false,
                        topLeft = Offset(6f, 5.5f),
                        size = Size(7f, 13f),
                        style = Stroke(1.7f, cap = StrokeCap.Round),
                    )
                    drawArc(
                        color = color,
                        startAngle = -60f,
                        sweepAngle = 120f,
                        useCenter = false,
                        topLeft = Offset(8.5f, 2.5f),
                        size = Size(9f, 19f),
                        style = Stroke(1.7f, cap = StrokeCap.Round),
                    )
                }

                // ---- 字幕（对话气泡 + 两条线）----
                "subtitle" -> {
                    drawRoundRect(
                        color, Offset(2.5f, 5f), Size(19f, 12f),
                        androidx.compose.ui.geometry.CornerRadius(2.5f), style = stroke,
                    )
                    drawLine(color, Offset(6f, 9.5f), Offset(18f, 9.5f), 1.7f, StrokeCap.Round)
                    drawLine(color, Offset(6f, 13f), Offset(13f, 13f), 1.7f, StrokeCap.Round)
                }

                // ---- 投屏（屏幕 + 投射角）----
                "cast" -> {
                    drawRoundRect(
                        color, Offset(3f, 4.5f), Size(18f, 12f),
                        androidx.compose.ui.geometry.CornerRadius(2f), style = stroke,
                    )
                    drawLine(color, Offset(8f, 20f), Offset(16f, 20f), 1.8f, StrokeCap.Round)
                    drawArc(
                        color = color,
                        startAngle = 0f,
                        sweepAngle = 180f,
                        useCenter = false,
                        topLeft = Offset(9.5f, 14f),
                        size = Size(5f, 5f),
                        style = Stroke(1.6f, cap = StrokeCap.Round),
                    )
                }

                // ---- 弹幕（圆圈 + 弹字，REX 的"弹"圈）----
                "danmaku" -> {
                    drawCircle(color, 9f, Offset(12f, 12f), style = stroke)
                    drawLine(color, Offset(12f, 7.5f), Offset(12f, 16.5f), 1.7f, StrokeCap.Round)
                    drawLine(color, Offset(7.5f, 12f), Offset(16.5f, 12f), 1.7f, StrokeCap.Round)
                }

                // ---- 锁定（锁形，锁定态用）----
                "lock" -> {
                    drawRoundRect(
                        color, Offset(5f, 10.5f), Size(14f, 9.5f),
                        androidx.compose.ui.geometry.CornerRadius(2f), style = stroke,
                    )
                    val shackle = Path().apply {
                        moveTo(8.5f, 10.5f); lineTo(8.5f, 8f)
                        arcTo(Rect(8.5f, 4.5f, 15.5f, 11.5f), 0f, 180f, false)
                        lineTo(15.5f, 10.5f)
                    }
                    drawPath(shackle, color, style = stroke)
                }

                // ---- 音量（喇叭 + 声波）----
                "volume" -> {
                    val spk = Path().apply {
                        moveTo(4f, 9.5f); lineTo(8f, 9.5f); lineTo(12.5f, 5.5f)
                        lineTo(12.5f, 18.5f); lineTo(8f, 14.5f); lineTo(4f, 14.5f); close()
                    }
                    drawPath(spk, color)
                    drawArc(
                        color = color,
                        startAngle = -60f,
                        sweepAngle = 120f,
                        useCenter = false,
                        topLeft = Offset(11f, 7f),
                        size = Size(6f, 10f),
                        style = Stroke(1.7f, cap = StrokeCap.Round),
                    )
                }

                // ---- 齿轮（设置类入口）----
                "gear" -> {
                    val ring = Path()
                    for (i in 0 until 48) {
                        val angle = i * Math.PI / 24
                        val radius = if (i % 6 in 1..3) 10.5f else 8f
                        val x = 12f + kotlin.math.cos(angle).toFloat() * radius
                        val y = 12f + kotlin.math.sin(angle).toFloat() * radius
                        if (i == 0) ring.moveTo(x, y) else ring.lineTo(x, y)
                    }
                    ring.close()
                    val hole = Path().apply { addOval(Rect(8.6f, 8.6f, 15.4f, 15.4f)) }
                    drawPath(Path.combine(PathOperation.Difference, ring, hole), color)
                }

                else -> {
                    drawCircle(color, 8f, Offset(12f, 12f), style = stroke)
                }
            }
        }
    }
}
