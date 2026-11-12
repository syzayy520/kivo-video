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
 * 播放页的**chrome / 控件符号**（v3 新增的第三族）。
 *
 * 【为什么另起一族而不是塞进已有两族】
 * · [drawCoreGlyph] 是"传输动作"（播放/暂停/快进/勾）；
 * · [drawFeatureGlyph] 是"功能入口"（投屏/弹幕/字幕/音轨…），已 272 行；
 * · 这一族是**界面零件**：主页、电池、chevron、−/+、相机、重置。
 * 三者变化的原因不同（动作语义 / 面板入口 / 系统 chrome），所以按变化原因拆开。
 *
 * 形状语言与前两族一致：24×24 基准画布、线框 + 少量实心、圆头线帽。
 */
internal fun DrawScope.drawChromeGlyph(kind: String, color: Color, stroke: Stroke, fill: Float) {
    when (kind) {
        // ---- 主页（REX 0160 实测 ink 21×18.3pt / 线宽 1.3pt / 门实心）----
        "home" -> {
            val roof = Path().apply {
                moveTo(1.2f, 11.8f); lineTo(12f, 2.8f); lineTo(22.8f, 11.8f)
            }
            drawPath(roof, color, style = stroke)
            val body = Path().apply {
                moveTo(4.0f, 10.6f); lineTo(4.0f, 21.2f)
                lineTo(20.0f, 21.2f); lineTo(20.0f, 10.6f)
            }
            drawPath(body, color, style = stroke)
            drawRoundRect(
                color = color,
                topLeft = Offset(10.0f, 15.2f),
                size = Size(4.0f, 6.0f),
                cornerRadius = CornerRadius(0.8f),
            )
        }

        // ---- 电池（自绘状态行右端；fill＝系统电量 0..1）----
        "battery" -> {
            drawRoundRect(
                color = color.copy(alpha = 0.5f),
                topLeft = Offset(2.0f, 7.0f),
                size = Size(17.5f, 10.0f),
                cornerRadius = CornerRadius(2.6f),
                style = Stroke(1.2f, cap = StrokeCap.Round),
            )
            drawRoundRect(
                color = color.copy(alpha = 0.5f),
                topLeft = Offset(20.4f, 10.2f),
                size = Size(1.6f, 3.6f),
                cornerRadius = CornerRadius(0.8f),
            )
            val w = 14.5f * fill.coerceIn(0f, 1f)
            if (w > 0.5f) {
                drawRoundRect(
                    color = color,
                    topLeft = Offset(3.5f, 8.5f),
                    size = Size(w, 7.0f),
                    cornerRadius = CornerRadius(1.4f),
                )
            }
        }

        // ---- chevron（钻入 / 展开）----
        "chevron-right" -> {
            drawLine(color, Offset(9f, 5f), Offset(16f, 12f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(16f, 12f), Offset(9f, 19f), stroke.width, StrokeCap.Round)
        }

        "chevron-down" -> {
            drawLine(color, Offset(5f, 9f), Offset(12f, 16f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(12f, 16f), Offset(19f, 9f), stroke.width, StrokeCap.Round)
        }

        // ---- 微调 pill 的 −/+ ----
        "plus" -> {
            drawLine(color, Offset(12f, 5f), Offset(12f, 19f), stroke.width, StrokeCap.Round)
            drawLine(color, Offset(5f, 12f), Offset(19f, 12f), stroke.width, StrokeCap.Round)
        }

        "minus" -> drawLine(color, Offset(5f, 12f), Offset(19f, 12f), stroke.width, StrokeCap.Round)

        // ---- 相机（拖动预览浮窗左侧的"截当前帧"圆钮）----
        "camera" -> {
            val bump = Path().apply {
                moveTo(8.4f, 7.0f); lineTo(9.8f, 4.6f)
                lineTo(14.2f, 4.6f); lineTo(15.6f, 7.0f)
            }
            drawPath(bump, color, style = stroke)
            drawRoundRect(
                color = color,
                topLeft = Offset(2.6f, 7.0f),
                size = Size(18.8f, 12.4f),
                cornerRadius = CornerRadius(3.0f),
                style = stroke,
            )
            drawCircle(color, radius = 3.8f, center = Offset(12f, 13.2f), style = stroke)
        }

        // ---- 资源卡的两枚 meta 图标（0169：胶片盒 / 码率菱形叠）----
        "film" -> {
            drawRoundRect(
                color = color,
                topLeft = Offset(3.0f, 6.0f),
                size = Size(18.0f, 12.0f),
                cornerRadius = CornerRadius(1.6f),
                style = stroke,
            )
            var x = 6.0f
            while (x < 19.0f) {
                drawLine(color, Offset(x, 6.4f), Offset(x, 8.4f), stroke.width * 0.8f, StrokeCap.Round)
                drawLine(color, Offset(x, 15.6f), Offset(x, 17.6f), stroke.width * 0.8f, StrokeCap.Round)
                x += 4.0f
            }
        }

        "bitrate" -> {
            val a = Path().apply {
                moveTo(8.0f, 5.0f); lineTo(13.0f, 12.0f); lineTo(8.0f, 19.0f); lineTo(3.0f, 12.0f); close()
            }
            drawPath(a, color, style = stroke)
            val b = Path().apply {
                moveTo(14.0f, 7.0f); lineTo(18.0f, 12.0f); lineTo(14.0f, 17.0f)
            }
            drawPath(b, color, style = stroke)
        }

        // ---- 重置（环形箭头）----
        "reset" -> {
            drawArc(
                color = color,
                startAngle = -60f,
                sweepAngle = 290f,
                useCenter = false,
                topLeft = Offset(4.6f, 4.6f),
                size = Size(14.8f, 14.8f),
                style = stroke,
            )
            val head = Path().apply {
                moveTo(15.4f, 3.4f); lineTo(19.6f, 5.2f); lineTo(16.2f, 8.4f); close()
            }
            drawPath(head, color)
        }

        else -> drawCircle(color, radius = 8.4f, center = Offset(12f, 12f), style = stroke)
    }
}

/** 属于 chrome 一族的 key（走 [drawChromeGlyph]）。 */
internal val CHROME_GLYPHS = setOf(
    "home", "battery", "chevron-right", "chevron-down", "plus", "minus", "camera", "reset",
    "film", "bitrate",
)