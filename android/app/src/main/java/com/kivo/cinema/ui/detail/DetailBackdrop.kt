package com.kivo.cinema.ui.detail

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.State
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clipToBounds
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.TransformOrigin
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.ui.design.TmdbImage

/** Media stays behind the content; the curtain moves with the consumed list offset. */
@Composable
internal fun DetailBackdrop(
    details: TmdbDetail.Details?,
    error: String?,
    foreground: Color,
    field: State<Color>,
    scrollDp: () -> Float,
    pullDp: Float = 0f,
) {
    Box(Modifier.fillMaxSize().background(Color.Black).clipToBounds()) {
        val art = details?.heroArtworkUrl.orEmpty()
        if (art.isNotEmpty()) TmdbImage(art, details?.title.orEmpty(),
            Modifier.fillMaxWidth().height(585.dp).graphicsLayer {
                translationY = -scrollDp() * density * .05f
                val stretch = 1f + pullDp / 585f
                scaleX = stretch; scaleY = stretch
                transformOrigin = TransformOrigin(.5f, 0f)
            })
        Box(Modifier.fillMaxSize().drawBehind {
            val color = field.value
            val shift = (scrollDp() - pullDp) * density
            drawRect(Brush.verticalGradient(
                0f to color.copy(alpha = 0f),
                .25f to color.copy(alpha = .08f),
                .55f to color.copy(alpha = .45f),
                .8f to color.copy(alpha = .9f),
                1f to color,
                startY = 250.dp.toPx() - shift,
                endY = 640.dp.toPx() - shift))
        })
        if (details == null && error != null) Text(error, fontSize = 13.sp,
            color = foreground, textAlign = TextAlign.Center,
            modifier = Modifier.align(Alignment.Center).padding(horizontal = 24.dp))
    }
}
