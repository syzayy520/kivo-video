package com.kivo.cinema.ui.design.home

import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import com.kivo.cinema.tmdb.TmdbHome
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * hero 候选 → [HeroCard] 列表（新旧两版首页共用，从 DiscoverPage 原样抽出，行为不变）。
 *
 * D13：首帧直接用列表自带海报（零网络、不可能空），竖版顶部海报在 IO 线程补齐后按 id 覆盖。
 * 不用 produceState：它的 initialValue 只在首次组合求值，heroCandidates 从空变非空时不会重算，
 * 真机上表现为 hero 整块高度 0（"继续观看"顶到屏幕最上）。
 */
@Composable
internal fun rememberHeroCards(
    heroCandidates: List<TmdbHome.Item>,
    topPoster: (TmdbHome.Item) -> String?,
): List<HeroCard> {
    var heroImages by remember { mutableStateOf<Map<Int, String>>(emptyMap()) }
    LaunchedEffect(heroCandidates) {
        heroImages = withContext(Dispatchers.IO) {
            heroCandidates.mapNotNull { item -> topPoster(item)?.let { item.id to it } }.toMap()
        }
    }
    return remember(heroCandidates, heroImages) {
        heroCandidates.map { item -> heroCardOf(item, heroImages[item.id] ?: item.posterHeroUrl) }
    }
}
