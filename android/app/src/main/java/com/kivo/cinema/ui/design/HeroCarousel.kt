package com.kivo.cinema.ui.design

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.pager.HorizontalPager
import androidx.compose.foundation.pager.rememberPagerState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.tmdb.trailer.TrailerResult
import com.kivo.cinema.ui.design.home.HeroPage

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
    Box(Modifier.fillMaxWidth().height((HERO_HEIGHT_DP + pullDp).dp)) {
        HorizontalPager(state = pagerState, key = { "$generation:${candidates[it].isTv}:${candidates[it].id}" },
            modifier = Modifier.fillMaxSize()) { index ->
            HeroPage(candidates[index], index == pagerState.settledPage && !pagerState.isScrollInProgress && previewVisible,
                scrimColor, topPoster, logoFor, onAmbient, pullDp, trailerFor, proxyAddress, previewEnabled,
                scrollPx)
        }
        Row(Modifier.align(Alignment.TopCenter).offset(y = (DOTS_CENTER_DP - 3.65f + pullDp).dp),
            verticalAlignment = Alignment.CenterVertically) {
            candidates.forEachIndexed { index, _ ->
                val diameter = dotDiameter(index, candidates.size, pagerState.currentPage)
                Box(Modifier.width(DOT_SPACING_DP.dp).height(7.3.dp), contentAlignment = Alignment.Center) {
                    Box(Modifier.size(diameter.dp).clip(CircleShape)
                        .background(dotsColor.copy(alpha = if (index == pagerState.currentPage) 1f else .35f)))
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
