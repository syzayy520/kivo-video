package com.kivo.cinema.ui.detail

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clipToBounds
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.TransformOrigin
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.graphics.luminance
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.ui.design.TmdbImage

/** Media stays behind the content; the curtain moves with the consumed list offset. */
@Composable
internal fun DetailBackdrop(
    details: TmdbDetail.Details?,
    field: State<Color>,
    scrollDp: () -> Float,
    pullDp: Float = 0f,
) {
    val art = details?.heroArtworkUrl.orEmpty()
    var failed by remember(art) { mutableStateOf(false) }
    val missing = art.isBlank() || failed
    Box(Modifier.fillMaxSize().background(when {
        details == null -> com.kivo.cinema.ui.design.DesignTokens.pageBackground
        missing -> field.value
        else -> Color.Black
    }).clipToBounds()) {
        if (!missing) TmdbImage(art, details?.title.orEmpty(),
            Modifier.fillMaxWidth().height(585.dp).graphicsLayer {
                // Measured reference: about +10.7% at 312pt, centered on media.
                val upward = 1f + scrollDp().coerceIn(0f, 350f) / (585f * 5f)
                val stretch = upward * (1f + pullDp / 585f)
                scaleX = stretch; scaleY = stretch
                transformOrigin = TransformOrigin(.5f, if (pullDp > 0f) 0f else .5f)
            }, onLoaded = { failed = !it })
        if (details != null && missing) Text(
            if (failed) "封面暂不可用" else "暂无封面",
            color = (if (field.value.luminance() > .45f) Color.Black else Color.White).copy(alpha = .6f),
            fontSize = 14.sp, textAlign = TextAlign.Center,
            modifier = Modifier.fillMaxWidth().padding(top = 200.dp),
        )
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

    }
}
