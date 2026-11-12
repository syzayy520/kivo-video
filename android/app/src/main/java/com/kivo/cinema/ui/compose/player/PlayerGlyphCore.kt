package com.kivo.cinema.ui.compose.player

import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke

/**
 * 播放页的**基础/传输类图标**（关闭、播放、暂停、上下集、快进快退、勾选…）。
 *
 * ## 与功能入口图标的分工
 * 基础图标 = "控件本身"（这一族每个只表示一个动作）；
 * 功能入口图标见 [PlayerGlyphFeature]（那一族表示"能打开哪个面板"）。
 * 两族形状语言必须一致（线框 + 少量实心），混在一排里才不脏。
 */
internal fun DrawScope.drawCoreGlyph(kind: String, color: Color, stroke: Stroke) {
    when (kind) {
        // ---- 关闭（X，实测 ink 17pt / 线宽 2pt）----
        // 杆 span 18 + 线宽 2.4 = ink 20.4/24；盒 20dp ⇒ ink 17dp、线 2dp。
        "close" -> {
            drawLine(color, Offset(3f, 3f), Offset(21f, 21f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(21f, 3f), Offset(3f, 21f), stroke.width, StrokeCap.Round)
        }

        // ---- 播放 / 暂停（实心，通用约定，不做线框）----
        "play" -> {
            val tri = Path().apply {
                moveTo(7.5f, 4.5f); lineTo(19.5f, 12f); lineTo(7.5f, 19.5f); close()
            }
            drawPath(tri, color)
        }

        "pause" -> {
            drawRoundRect(color, Offset(6.5f, 4.5f), Size(4f, 15f), CornerRadius(1.2f))
            drawRoundRect(color, Offset(13.5f, 4.5f), Size(4f, 15f), CornerRadius(1.2f))
        }

        // ---- 下一个（▷|）----
        "next" -> {
            val tri = Path().apply {
                moveTo(5f, 5f); lineTo(15f, 12f); lineTo(5f, 19f); close()
            }
            drawPath(tri, color)
            drawRoundRect(color, Offset(16.5f, 5f), Size(2.6f, 14f), CornerRadius(1.2f))
        }

        // ---- 快进 / 快退（双三角）----
        // 双三角是"快进/快退"的通用约定，与播放器界的普遍认知一致。
        "forward" -> drawFastTriangles(color, forward = true)
        "backward" -> drawFastTriangles(color, forward = false)

        // ---- 选中勾 ----
        "check" -> {
            drawLine(color, Offset(4.5f, 12.5f), Offset(9.5f, 17.5f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(9.5f, 17.5f), Offset(19.5f, 6.5f), stroke.width, StrokeCap.Round)
        }

        // ---- 亮度（太阳）----
        // 亮度 HUD 的图标：中心圆 + 8 道光芒
        "sun" -> {
            drawCircle(
                color = color,
                radius = 4.6f,
                center = Offset(12f, 12f),
                style = stroke,
            )
            var i = 0
            while (i < 8) {
                val angle = i * Math.PI / 4.0
                val cos = kotlin.math.cos(angle).toFloat()
                val sin = kotlin.math.sin(angle).toFloat()
                drawLine(
                    color = color,
                    start = Offset(12f + cos * 7.2f, 12f + sin * 7.2f),
                    end = Offset(12f + cos * 10.2f, 12f + sin * 10.2f),
                    strokeWidth = stroke.width,
                    cap = StrokeCap.Round,
                )
                i++
            }
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

        // ---- 开锁（锁定态的唯一控件，实测）----
        // 锁体在左下、锁梁从锁体右上角升起、右端悬空 = SF lock.open 形。
        // 实测 glyph 30×28.3pt、线宽 2.5pt ⇒ 24 画布下 stroke 2.0。
        "lock-open" -> {
            drawRoundRect(
                color = color,
                topLeft = Offset(0.5f, 9.5f),
                size = Size(16f, 13f),
                cornerRadius = CornerRadius(2.5f),
                style = stroke,
            )
            val shackle = Path().apply {
                moveTo(12.5f, 9.5f); lineTo(12.5f, 5.5f)
                arcTo(androidx.compose.ui.geometry.Rect(12.5f, 0.5f, 22.5f, 10.5f), 180f, 180f, false)
                lineTo(22.5f, 9.5f)
            }
            drawPath(shackle, color, style = stroke)
        }

        // ---- 锁定（锁形）----
        "lock" -> {
            drawRoundRect(
                color = color,
                topLeft = Offset(5f, 10.5f),
                size = Size(14f, 9.5f),
                cornerRadius = CornerRadius(2f),
                style = stroke,
            )
            val shackle = Path().apply {
                moveTo(8.5f, 10.5f); lineTo(8.5f, 8f)
                arcTo(androidx.compose.ui.geometry.Rect(8.5f, 4.5f, 15.5f, 11.5f), 0f, 180f, false)
                lineTo(15.5f, 10.5f)
            }
            drawPath(shackle, color, style = stroke)
        }

        else -> drawCircle(
            color = color,
            radius = 8.4f,
            center = Offset(12f, 12f),
            style = stroke,
        )
    }
}

/** 双三角快进/快退（在 24×24 的基准画布里画）。 */
private fun DrawScope.drawFastTriangles(color: Color, forward: Boolean) {
    if (forward) {
        val a = Path().apply {
            moveTo(4f, 5f); lineTo(13f, 12f); lineTo(4f, 19f); close()
        }
        drawPath(a, color)
        val b = Path().apply {
            moveTo(12f, 5f); lineTo(21f, 12f); lineTo(12f, 19f); close()
        }
        drawPath(b, color)
    } else {
        val a = Path().apply {
            moveTo(20f, 5f); lineTo(11f, 12f); lineTo(20f, 19f); close()
        }
        drawPath(a, color)
        val b = Path().apply {
            moveTo(12f, 5f); lineTo(3f, 12f); lineTo(12f, 19f); close()
        }
        drawPath(b, color)
    }
}
