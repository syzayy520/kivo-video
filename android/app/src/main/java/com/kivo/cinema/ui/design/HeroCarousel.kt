package com.kivo.cinema.ui.design

import androidx.compose.animation.core.spring
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.pager.HorizontalPager
import androidx.compose.foundation.pager.PagerDefaults
import androidx.compose.foundation.pager.rememberPagerState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.lerp
import androidx.compose.ui.unit.dp
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.tmdb.trailer.TrailerResult
import com.kivo.cinema.ui.design.home.HeroPage
import kotlin.math.abs
import kotlin.math.floor

/** Paging, active artwork reporting, and the shared page indicator. */
@Composable
fun HeroCarousel(
    candidates: List<TmdbHome.Item>,
    onAmbient: (Color) -> Unit,
    dotsColor: Color,
    scrimColor: State<Color>,
    topPoster: (TmdbHome.Item) -> String?,
    logoFor: (TmdbHome.Item) -> String?,
    pullDp: Float = 0f,
    scrollPx: () -> Float = { 0f },
    previewVisible: Boolean = true,
    previewEnabled: Boolean = false,
    generation: Int = 0,
    trailerFor: (TmdbHome.Item) -> TrailerResult,
    proxyAddress: String?,
) {
    if (candidates.isEmpty()) return
    val pagerState = rememberPagerState(pageCount = { candidates.size })
    val mediaKeys = candidates.map { it.isTv to it.id }
    val palettes = remember(generation, mediaKeys) { mutableStateMapOf<Int, Color>() }
    val report by rememberUpdatedState(onAmbient)
    LaunchedEffect(pagerState, generation, mediaKeys) {
        snapshotFlow {
            val position = (pagerState.currentPage + pagerState.currentPageOffsetFraction)
                .coerceIn(0f, (candidates.size - 1).toFloat())
            val left = floor(position).toInt()
            val right = (left + 1).coerceAtMost(candidates.lastIndex)
            val a = palettes[left] ?: palettes[right]
            val b = palettes[right] ?: a
            if (a != null && b != null) lerp(a, b, position - left) else null
        }.collect { color -> color?.let { report(it) } }
    }
    Box(Modifier.fillMaxWidth().height((HERO_HEIGHT_DP + pullDp).dp)) {
        HorizontalPager(state = pagerState, key = { "$generation:${candidates[it].isTv}:${candidates[it].id}" },
            beyondViewportPageCount = 1,
            flingBehavior = PagerDefaults.flingBehavior(pagerState,
                snapAnimationSpec = spring(dampingRatio = 1f, stiffness = 240f)),
            modifier = Modifier.fillMaxSize()) { index ->
            HeroPage(candidates[index], index == pagerState.settledPage && !pagerState.isScrollInProgress && previewVisible,
                scrimColor, topPoster, logoFor, { palettes[index] = it }, pullDp, trailerFor, proxyAddress, previewEnabled,
                scrollPx, pageOffset = { pagerState.currentPage - index + pagerState.currentPageOffsetFraction })
        }
        Row(Modifier.align(Alignment.TopCenter).offset(y = (DOTS_CENTER_DP - 3.65f + pullDp).dp),
            verticalAlignment = Alignment.CenterVertically) {
            candidates.forEachIndexed { index, _ ->
                val position = pagerState.currentPage + pagerState.currentPageOffsetFraction
                val weight = (1f - abs(index - position)).coerceIn(0f, 1f)
                val left = floor(position).toInt().coerceIn(0, candidates.lastIndex)
                val right = (left + 1).coerceAtMost(candidates.lastIndex)
                val fraction = (position - left).coerceIn(0f, 1f)
                val diameter = dotDiameter(index, candidates.size, left) * (1f - fraction) +
                    dotDiameter(index, candidates.size, right) * fraction
                Box(Modifier.width(DOT_SPACING_DP.dp).height(7.3.dp), contentAlignment = Alignment.Center) {
                    Box(Modifier.size(diameter.dp).clip(CircleShape)
                        .background(dotsColor.copy(alpha = .35f + .65f * weight)))
                }
            }
        }
    }
}

const val HERO_HEIGHT_DP = 585
const val LOGO_CENTER_DP = 402f
const val LOGO_MAX_H = 115f
const val META_TOP_DP = 472f
const val BADGE_TOP_DP = 522f
const val DOTS_CENTER_DP = 574.5f
const val DOT_SPACING_DP = 17.7f

internal fun dotDiameter(index: Int, count: Int, active: Int): Float {
    if (index == active) return 7.3f
    // Shrink the remote end; near the final page the leading end recedes instead.
    val edgeDistance = if (active >= count - 3) index else count - 1 - index
    return when (edgeDistance) { 0 -> 3.3f; 1 -> 6f; else -> 7.3f }
}
