package com.kivo.cinema.ui.compose.player.seek

import androidx.compose.foundation.Canvas
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.compose.player.PlayerTokens
import kotlin.math.abs
import kotlin.math.min

@Composable
internal fun PlayerSeekRail(
    fraction: Float,
    bufferedFraction: Float,
    originFraction: Float,
    feedbackVisible: Boolean,
    press: Float,
    modifier: Modifier = Modifier,
) {
    Canvas(modifier) {
        val trackHeight = PlayerTokens.trackHeight.dp.toPx()
        val inset = 7.dp.toPx()
        val width = (size.width - inset * 2).coerceAtLeast(1f)
        val y = (size.height - trackHeight) / 2f
        val radius = CornerRadius(trackHeight / 2)
        val current = fraction.coerceIn(0f, 1f)
        val buffered = bufferedFraction.coerceIn(0f, 1f)
        val origin = originFraction.coerceIn(0f, 1f)
        drawRoundRect(PlayerTokens.trackRemaining, Offset(inset, y), Size(width, trackHeight), radius)
        if (buffered > 0f) {
            drawRoundRect(PlayerTokens.trackBuffered, Offset(inset, y), Size(width * buffered, trackHeight), radius)
        }
        if (current > 0f) {
            drawRoundRect(PlayerTokens.trackPlayed.copy(alpha = .78f), Offset(inset, y), Size(width * current, trackHeight), radius)
        }
        if (feedbackVisible) {
            drawRoundRect(
                Color(0xFFF0F5DF), Offset(inset + width * min(origin, current), y),
                Size(width * abs(current - origin), trackHeight), radius,
            )
            val tickX = inset + width * origin
            drawLine(Color.White.copy(alpha = .45f), Offset(tickX, size.height / 2 - 6.dp.toPx()),
                Offset(tickX, size.height / 2 + 6.dp.toPx()), 1.dp.toPx())
        }
        val center = Offset(inset + width * current, size.height / 2)
        val knob = (4f + press * 2f).dp.toPx()
        if (press > 0f) drawCircle(Color.White.copy(alpha = press * .08f), knob + 4.dp.toPx(), center)
        drawCircle(Color.Black.copy(alpha = .18f), knob + 1.dp.toPx(), center + Offset(0f, 1.dp.toPx()))
        drawCircle(Color(0xFFF0F5E8), knob, center)
    }
}
