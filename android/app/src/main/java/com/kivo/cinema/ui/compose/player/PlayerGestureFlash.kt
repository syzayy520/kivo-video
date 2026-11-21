package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.core.Animatable
import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.withTransform
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.motion.MotionGate

@Composable
internal fun PlayerGestureFlash(
    zone: DoubleTapZone?,
    modifier: Modifier = Modifier,
    eventId: Long = 0L,
    isPlaying: Boolean = false,
) {
    val reduceMotion = MotionGate.rememberAppReduceMotion()
    val elapsed = remember { Animatable(PlayerMotion.flashDurationMs.toFloat()) }
    LaunchedEffect(eventId, zone) {
        if (zone == null) {
            elapsed.snapTo(PlayerMotion.flashDurationMs.toFloat())
            return@LaunchedEffect
        }
        elapsed.snapTo(0f)
        elapsed.animateTo(
            PlayerMotion.flashDurationMs.toFloat(),
            tween(PlayerMotion.flashDurationMs, easing = LinearEasing),
        )
    }

    Canvas(modifier.fillMaxSize()) {
        val activeZone = zone ?: return@Canvas
        val time = elapsed.value
        val center = Offset(
            size.width * when (activeZone) {
                DoubleTapZone.BACKWARD -> 0.2f
                DoubleTapZone.TOGGLE_PLAY -> 0.5f
                DoubleTapZone.FORWARD -> 0.8f
            },
            size.height * 0.5f,
        )
        val fadeMs = if (reduceMotion) PlayerMotion.reduceMotionMs else PlayerMotion.flashEnterMs
        val opacity = flashOpacity(time, PlayerMotion.flashDurationMs, fadeMs, fadeMs)
        if (activeZone == DoubleTapZone.TOGGLE_PLAY) {
            if (opacity <= 0f) return@Canvas
            drawCircle(
                brush = Brush.radialGradient(
                    colors = listOf(Color.Black.copy(alpha = 0.36f * opacity), Color.Transparent),
                    center = center,
                    radius = 30.dp.toPx(),
                ),
                radius = 30.dp.toPx(),
                center = center,
            )
            val glyphSize = 36.dp.toPx()
            withTransform({
                translate(center.x - glyphSize / 2f, center.y - glyphSize / 2f)
                scale(glyphSize / 24f, glyphSize / 24f, pivot = Offset.Zero)
            }) {
                drawCoreGlyph(
                    kind = if (isPlaying) "play" else "pause",
                    color = Color.White.copy(alpha = opacity),
                    stroke = Stroke(1.9f),
                )
            }
            return@Canvas
        }

        val direction = if (activeZone == DoubleTapZone.FORWARD) 1f else -1f
        repeat(2) { index ->
            val localTime = time - index * PlayerMotion.flashRelayMs
            val alpha = if (reduceMotion) opacity else flashOpacity(
                localTime, PlayerMotion.flashArrowMs, PlayerMotion.flashEnterMs,
                PlayerMotion.flashArrowMs - PlayerMotion.flashEnterMs,
            )
            if (alpha <= 0f) return@repeat
            val travel = if (reduceMotion) 0f else
                (localTime / PlayerMotion.flashArrowMs).coerceIn(0f, 1f) * 14.dp.toPx()
            val x = center.x + direction * ((index - 0.5f) * 20.dp.toPx() + travel)
            val chevron = Path().apply {
                moveTo(x - direction * 5.dp.toPx(), center.y - 11.dp.toPx())
                lineTo(x + direction * 5.dp.toPx(), center.y)
                lineTo(x - direction * 5.dp.toPx(), center.y + 11.dp.toPx())
            }
            val stroke = Stroke(2.4.dp.toPx(), cap = StrokeCap.Round, join = StrokeJoin.Round)
            drawPath(chevron, Color.Black.copy(alpha = alpha * 0.28f), style = Stroke(7.dp.toPx()))
            drawPath(chevron, Color.White.copy(alpha = alpha * 0.09f), style = Stroke(6.dp.toPx()))
            drawPath(chevron, Color.White.copy(alpha = alpha * 0.96f), style = stroke)
        }
    }
}

private fun flashOpacity(time: Float, duration: Int, enter: Int, exit: Int): Float {
    if (time <= 0f || time >= duration) return 0f
    return minOf(time / enter, (duration - time) / exit, 1f).coerceIn(0f, 1f)
}
