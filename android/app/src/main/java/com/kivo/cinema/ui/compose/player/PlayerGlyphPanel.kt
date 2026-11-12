package com.kivo.cinema.ui.compose.player

import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import kotlin.math.cos
import kotlin.math.sin

/**
 * 面板**行首图标**族（v3 §1.4 形态 1/4 的行 icon，iPhone 现屏实测形状）。
 *
 * 与顶栏入口图标分族：顶栏是"功能入口"（[drawFeatureGlyph]），
 * 这里是"行内语义"（齿轮=设置、文档=外挂、放大镜=搜索、托盘=导入、
 * 圈叉=关闭、三滑杆=延迟微调）。形状语言同族：24×24 基准、线框为主。
 */
internal val PANEL_GLYPHS = setOf("gear", "doc", "search", "import", "xcircle", "sliders")

internal fun DrawScope.drawPanelGlyph(kind: String, color: Color, stroke: Stroke) {
    when (kind) {
        "gear" -> {
            drawCircle(color, radius = 4.6f, center = Offset(12f, 12f), style = stroke)
            drawCircle(color, radius = 1.6f, center = Offset(12f, 12f), style = stroke)
            for (i in 0 until 8) {
                val a = Math.toRadians(i * 45.0)
                val dx = cos(a).toFloat()
                val dy = sin(a).toFloat()
                drawLine(
                    color,
                    Offset(12f + dx * 6.4f, 12f + dy * 6.4f),
                    Offset(12f + dx * 9.2f, 12f + dy * 9.2f),
                    strokeWidth = stroke.width,
                    cap = stroke.cap,
                )
            }
        }
        "doc" -> {
            val p = Path().apply {
                moveTo(7f, 3f)
                lineTo(14f, 3f)
                lineTo(18f, 7f)
                lineTo(18f, 21f)
                lineTo(7f, 21f)
                close()
                moveTo(14f, 3f)
                lineTo(14f, 7f)
                lineTo(18f, 7f)
            }
            drawPath(p, color, style = stroke)
            drawLine(color, Offset(10f, 12f), Offset(15f, 12f), strokeWidth = stroke.width, cap = stroke.cap)
            drawLine(color, Offset(10f, 16f), Offset(15f, 16f), strokeWidth = stroke.width, cap = stroke.cap)
        }
        "search" -> {
            drawCircle(color, radius = 5.5f, center = Offset(10.5f, 10.5f), style = stroke)
            drawLine(color, Offset(14.6f, 14.6f), Offset(20f, 20f), strokeWidth = stroke.width, cap = stroke.cap)
        }
        "import" -> {
            val p = Path().apply {
                moveTo(4f, 15f)
                lineTo(4f, 20f)
                lineTo(20f, 20f)
                lineTo(20f, 15f)
                moveTo(12f, 4f)
                lineTo(12f, 14f)
                moveTo(8f, 10.5f)
                lineTo(12f, 14.5f)
                lineTo(16f, 10.5f)
            }
            drawPath(p, color, style = stroke)
        }
        "xcircle" -> {
            drawCircle(color, radius = 9f, center = Offset(12f, 12f), style = stroke)
            drawLine(color, Offset(9f, 9f), Offset(15f, 15f), strokeWidth = stroke.width, cap = stroke.cap)
            drawLine(color, Offset(15f, 9f), Offset(9f, 15f), strokeWidth = stroke.width, cap = stroke.cap)
        }
        "sliders" -> {
            listOf(7f to 15f, 12f to 9f, 17f to 14f).forEach { (y, kx) ->
                drawLine(color, Offset(4f, y), Offset(20f, y), strokeWidth = stroke.width, cap = stroke.cap)
                drawLine(color, Offset(kx, y - 2.4f), Offset(kx, y + 2.4f), strokeWidth = stroke.width, cap = stroke.cap)
            }
        }
    }
}