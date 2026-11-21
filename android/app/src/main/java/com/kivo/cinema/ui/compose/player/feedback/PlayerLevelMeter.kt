package com.kivo.cinema.ui.compose.player.feedback

import androidx.compose.animation.core.Animatable
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.RoundRect
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.clipPath
import androidx.compose.ui.graphics.drawscope.scale
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.compose.player.PlayerHudKind
import com.kivo.cinema.ui.compose.player.PlayerMotion
import com.kivo.cinema.ui.motion.MotionGate
import kotlin.math.cos
import kotlin.math.sin

@Composable
internal fun PlayerLevelMeter(
    fraction: Float,
    originFraction: Float,
    dragging: Boolean,
    modifier: Modifier = Modifier,
) {
    val level = fraction.coerceIn(0f, 1f)
    val origin = originFraction.coerceIn(0f, 1f)
    val boundary = when (level) { 0f -> -1; 1f -> 1; else -> 0 }
    val reduceMotion = MotionGate.rememberAppReduceMotion()
    var previousBoundary by remember { mutableIntStateOf(boundary) }
    val highlight = remember { Animatable(0f) }
    LaunchedEffect(boundary) {
        val entered = boundary != 0 && boundary != previousBoundary
        previousBoundary = boundary
        highlight.snapTo(0f)
        if (dragging && entered) {
            highlight.animateTo(1f, tween(PlayerMotion.hudBoundaryEnterMs))
            highlight.animateTo(
                0f,
                tween(if (reduceMotion) PlayerMotion.reduceMotionMs else PlayerMotion.hudBoundaryExitMs),
            )
        }
    }

    Canvas(modifier) {
        val slotWidth = 8.dp.toPx()
        val left = (size.width - slotWidth) / 2f
        val top = 4.dp.toPx()
        val height = (size.height - top * 2f).coerceAtLeast(0f)
        val radius = CornerRadius(slotWidth / 2f)
        val slot = Path().apply {
            addRoundRect(RoundRect(left, top, left + slotWidth, top + height, radius))
        }
        drawPath(slot, Color.White.copy(alpha = 0.16f))
        clipPath(slot) {
            drawRect(
                brush = Brush.verticalGradient(
                    listOf(Color.White, Color(0xFFCED5E1)), startY = top, endY = top + height,
                ),
                topLeft = Offset(left, top + height * (1f - level)),
                size = Size(slotWidth, height * level),
                alpha = 0.82f + highlight.value * 0.16f,
            )
        }
        val originY = top + height * (1f - origin)
        val tickColor = Color.White.copy(alpha = 0.38f)
        drawLine(
            tickColor, Offset(left - 5.dp.toPx(), originY), Offset(left - 1.dp.toPx(), originY),
            strokeWidth = 1.dp.toPx(), cap = StrokeCap.Round,
        )
        drawLine(
            tickColor, Offset(left + slotWidth + 1.dp.toPx(), originY),
            Offset(left + slotWidth + 5.dp.toPx(), originY),
            strokeWidth = 1.dp.toPx(), cap = StrokeCap.Round,
        )
        if (highlight.value > 0f) {
            val inset = 3.dp.toPx()
            drawRoundRect(
                color = Color.White.copy(alpha = highlight.value * 0.22f),
                topLeft = Offset(left - inset, top - inset),
                size = Size(slotWidth + inset * 2f, height + inset * 2f),
                cornerRadius = CornerRadius(radius.x + inset),
                style = Stroke(1.dp.toPx()),
            )
        }
    }
}

@Composable
internal fun PlayerLevelIcon(kind: PlayerHudKind, fraction: Float, modifier: Modifier = Modifier) {
    val level = fraction.coerceIn(0f, 1f)
    Canvas(modifier) {
        scale(size.width / 24f, size.height / 24f, pivot = Offset.Zero) {
            val white = Color.White.copy(alpha = 0.94f)
            val stroke = Stroke(1.7f, cap = StrokeCap.Round)
            if (kind == PlayerHudKind.BRIGHTNESS) {
                drawCircle(white, 3.5f, Offset(12f, 12f), style = stroke)
                drawCircle(white.copy(alpha = level * 0.3f), 2.5f, Offset(12f, 12f))
                repeat(8) { index ->
                    val angle = index * Math.PI / 4.0
                    val direction = Offset(cos(angle).toFloat(), sin(angle).toFloat())
                    drawLine(
                        white.copy(alpha = 0.5f + level * 0.44f),
                        Offset(12f, 12f) + direction * 6.5f,
                        Offset(12f, 12f) + direction * (8f + level * 2f),
                        strokeWidth = stroke.width, cap = StrokeCap.Round,
                    )
                }
            } else if (kind == PlayerHudKind.VOLUME) {
                val speaker = Path().apply {
                    moveTo(3f, 9f); lineTo(7f, 9f); lineTo(12f, 5f)
                    lineTo(12f, 19f); lineTo(7f, 15f); lineTo(3f, 15f); close()
                }
                drawPath(speaker, white, style = stroke)
                if (level == 0f) {
                    drawLine(white, Offset(17f, 9f), Offset(22f, 15f), stroke.width, StrokeCap.Round)
                    drawLine(white, Offset(22f, 9f), Offset(17f, 15f), stroke.width, StrokeCap.Round)
                } else {
                    drawArc(white, -55f, 110f, false, Offset(11f, 8f), Size(9f, 8f), style = stroke)
                    if (level >= 0.5f) {
                        drawArc(white, -55f, 110f, false, Offset(10f, 5f), Size(13f, 14f), style = stroke)
                    }
                }
            }
        }
    }
}
