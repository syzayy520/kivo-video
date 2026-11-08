package com.kivo.cinema.ui.design.home

import androidx.compose.foundation.Canvas
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.PathOperation
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.scale
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.geometry.Rect

/** Monochrome symbols independent of the system emoji font. */
@Composable
internal fun NavGlyph(kind: String, color: Color, modifier: Modifier) {
    Canvas(modifier) {
        scale(size.width / 28f, size.height / 28f, pivot = Offset.Zero) {
            val stroke = Stroke(2.3f, cap = StrokeCap.Round)
            when (kind) {
                "search" -> {
                    drawCircle(color, 8.5f, Offset(11.5f, 11.5f), style = stroke)
                    drawLine(color, Offset(18f, 18f), Offset(25f, 25f), 3f, StrokeCap.Round)
                }
                "我的" -> {
                    val body = Path().apply { addOval(Rect(1.5f, 1.5f, 26.5f, 26.5f)) }
                    val holes = Path().apply {
                        addOval(Rect(9.8f, 5.8f, 18.2f, 14.2f))
                        addOval(Rect(7f, 16f, 21f, 24f))
                    }
                    drawPath(Path.combine(PathOperation.Difference, body, holes), color)
                }
                "资源库" -> {
                    val lid = Path().apply {
                        moveTo(5f, 4f); lineTo(23f, 4f); lineTo(27f, 17f); lineTo(1f, 17f); close()
                    }
                    drawPath(lid, color)
                    drawRoundRect(color, Offset(1f, 18f), Size(26f, 8f), androidx.compose.ui.geometry.CornerRadius(4f))
                    drawCircle(Color.White.copy(alpha=.75f), 1.3f, Offset(22f, 22f))
                }
                "设置" -> {
                    val gear = Path()
                    for (i in 0 until 48) {
                        val angle = i * Math.PI / 24
                        val radius = if (i % 6 in 1..3) 13f else 10.3f
                        val x = 14f + kotlin.math.cos(angle).toFloat() * radius
                        val y = 14f + kotlin.math.sin(angle).toFloat() * radius
                        if (i == 0) gear.moveTo(x, y) else gear.lineTo(x, y)
                    }
                    gear.close()
                    val hole = Path().apply { addOval(Rect(9.5f, 9.5f, 18.5f, 18.5f)) }
                    drawPath(Path.combine(PathOperation.Difference, gear, hole), color)
                }
                "在看" -> {
                    drawRoundRect(color, Offset(2f, 5f), Size(24f, 19f), androidx.compose.ui.geometry.CornerRadius(4f), style = stroke)
                    val play = Path().apply {
                        moveTo(11f, 9f); lineTo(19f, 14.5f); lineTo(11f, 20f); close()
                    }
                    drawPath(play, color)
                }
                else -> {
                    drawCircle(color, 12f, Offset(14f, 14f), style = stroke)
                    val needle = Path().apply {
                        moveTo(19f, 7f); lineTo(16f, 16f); lineTo(7f, 20f); lineTo(11f, 11f); close()
                    }
                    drawPath(needle, color)
                }
            }
        }
    }
}
