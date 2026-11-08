package com.kivo.cinema.ui.detail.actions

import androidx.compose.foundation.Canvas
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.scale

internal enum class ActionGlyph { Bookmark, Watched, External, Reload, Close, Tick, Plus, Chevron }

/** Consistent 24-unit strokes; decorative glyphs use the row's accessibility label. */
@Composable
internal fun DetailActionGlyph(glyph: ActionGlyph, color: Color, modifier: Modifier) {
    Canvas(modifier) {
        scale(size.width / 24f, size.height / 24f, pivot = Offset.Zero) {
            val stroke = Stroke(1.7f, cap = StrokeCap.Round, join = StrokeJoin.Round)
            fun path(block: Path.() -> Unit) = drawPath(Path().apply(block), color, style = stroke)
            when (glyph) {
                ActionGlyph.Bookmark -> path {
                    moveTo(7f, 3.5f); lineTo(17f, 3.5f); quadraticTo(18f, 3.5f, 18f, 4.5f)
                    lineTo(18f, 21f); lineTo(12f, 17f); lineTo(6f, 21f)
                    lineTo(6f, 4.5f); quadraticTo(6f, 3.5f, 7f, 3.5f); close()
                }
                ActionGlyph.Watched -> {
                    drawCircle(color, 9f, Offset(12f, 12f), style = stroke)
                    path { moveTo(7.8f, 12f); lineTo(10.6f, 14.8f); lineTo(16.3f, 9.1f) }
                }
                ActionGlyph.External -> {
                    path {
                        moveTo(10f, 5f); lineTo(5f, 5f); lineTo(5f, 20f)
                        lineTo(19f, 20f); lineTo(19f, 14f)
                    }
                    path { moveTo(13f, 4f); lineTo(20f, 4f); lineTo(20f, 11f) }
                    path { moveTo(20f, 4f); lineTo(11f, 13f) }
                }
                ActionGlyph.Reload -> {
                    drawArc(color, -45f, 295f, false, Offset(4f, 4f), Size(16f, 16f), style = stroke)
                    path { moveTo(19f, 3.5f); lineTo(19f, 9f); lineTo(13.5f, 9f) }
                }
                ActionGlyph.Close -> {
                    path { moveTo(5f, 5f); lineTo(19f, 19f) }
                    path { moveTo(19f, 5f); lineTo(5f, 19f) }
                }
                ActionGlyph.Tick -> path {
                    moveTo(4f, 12f); lineTo(10f, 18f); lineTo(20f, 6f)
                }
                ActionGlyph.Plus -> {
                    path { moveTo(12f, 5f); lineTo(12f, 19f) }
                    path { moveTo(5f, 12f); lineTo(19f, 12f) }
                }
                ActionGlyph.Chevron -> path {
                    moveTo(9f, 5f); lineTo(16f, 12f); lineTo(9f, 19f)
                }
            }
        }
    }
}
