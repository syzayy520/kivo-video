package com.kivo.cinema.ui.search

import androidx.compose.foundation.Canvas
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.scale
import androidx.compose.ui.unit.dp

/** 本页线框字形（稿面 24 viewBox、stroke 1.9、round cap/join）。 */
internal enum class SearchIcon { Back, Close, Mic, ChevronDown, Filter, Play }

@Composable
internal fun SearchGlyph(icon: SearchIcon, color: Color, modifier: Modifier = Modifier) {
    Canvas(modifier) {
        scale(size.width / 24f, size.height / 24f, pivot = Offset.Zero) {
            val stroke = Stroke(1.9f, cap = StrokeCap.Round, join = StrokeJoin.Round)
            when (icon) {
                SearchIcon.Back -> {
                    val p = Path().apply {
                        moveTo(14.5f, 6f); lineTo(8.5f, 12f); lineTo(14.5f, 18f)
                    }
                    drawPath(p, color, style = stroke)
                }
                SearchIcon.Close -> {
                    drawLine(color, Offset(7f, 7f), Offset(17f, 17f), 1.9f, StrokeCap.Round)
                    drawLine(color, Offset(17f, 7f), Offset(7f, 17f), 1.9f, StrokeCap.Round)
                }
                SearchIcon.Mic -> {
                    drawRoundRect(color, Offset(9f, 3f), androidx.compose.ui.geometry.Size(6f, 11f),
                        androidx.compose.ui.geometry.CornerRadius(3f), style = stroke)
                    val arc = Path().apply {
                        moveTo(5.5f, 11f)
                        cubicTo(5.5f, 15f, 8.4f, 17.5f, 12f, 17.5f)
                        cubicTo(15.6f, 17.5f, 18.5f, 15f, 18.5f, 11f)
                    }
                    drawPath(arc, color, style = stroke)
                    drawLine(color, Offset(12f, 17.5f), Offset(12f, 21f), 1.9f, StrokeCap.Round)
                }
                SearchIcon.ChevronDown -> {
                    val p = Path().apply {
                        moveTo(7f, 10f); lineTo(12f, 15f); lineTo(17f, 10f)
                    }
                    drawPath(p, color, style = stroke)
                }
                SearchIcon.Filter -> {
                    drawLine(color, Offset(4f, 7f), Offset(14f, 7f), 1.9f, StrokeCap.Round)
                    drawLine(color, Offset(18f, 7f), Offset(20f, 7f), 1.9f, StrokeCap.Round)
                    drawCircle(color, 2f, Offset(16f, 7f), style = stroke)
                    drawLine(color, Offset(4f, 17f), Offset(6f, 17f), 1.9f, StrokeCap.Round)
                    drawLine(color, Offset(10f, 17f), Offset(20f, 17f), 1.9f, StrokeCap.Round)
                    drawCircle(color, 2f, Offset(8f, 17f), style = stroke)
                }
                SearchIcon.Play -> {
                    val p = Path().apply {
                        moveTo(8f, 5f); lineTo(19f, 12f); lineTo(8f, 19f); close()
                    }
                    drawPath(p, color)
                }
            }
        }
    }
}

/** 搜索小图标（与 NavGlyph("search") 同形，供本页公开调用，避免跨包 internal 语义错位）。 */
@Composable
internal fun SearchMagnifier(color: Color, modifier: Modifier = Modifier) {
    Canvas(modifier) {
        scale(size.width / 24f, size.height / 24f, pivot = Offset.Zero) {
            val stroke = Stroke(2.1.dp.toPx() / (size.width / 24f), cap = StrokeCap.Round)
            drawCircle(color, 6.5f, Offset(11f, 11f), style = stroke)
            drawLine(color, Offset(16f, 16f), Offset(20f, 20f), 2.1f, StrokeCap.Round)
        }
    }
}