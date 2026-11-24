package com.kivo.cinema

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.ui.Modifier
import com.kivo.cinema.search.BrowseRepository
import com.kivo.cinema.search.CategoryArtworkRepository
import com.kivo.cinema.search.EmbyPlayableIndex
import com.kivo.cinema.search.HotSearchRepository
import com.kivo.cinema.search.LocalPlayableIndex
import com.kivo.cinema.search.RecentSearchStore
import com.kivo.cinema.search.SearchEntry
import com.kivo.cinema.search.SearchRepository
import com.kivo.cinema.tmdb.TmdbSearch
import com.kivo.cinema.ui.design.CapsuleNavBar
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.detail.MovieDetailActivity
import com.kivo.cinema.ui.search.SearchOverlay
import com.kivo.cinema.ui.shell.ShellTab

/**
 * 搜索覆盖层的宿主接线（与 MainActivityHistory 同构）：装配仓储、详情入口、底栏 slot。
 * 底栏放大镜 = 本覆盖层（取代跳旧 AggregateActivity）；切 Tab = 关闭覆盖层并切页。
 */
@Composable
internal fun MainActivity.SearchOverlayScaffold(
    onBack: () -> Unit,
    onSelectTab: (Int) -> Unit,
) {
    val app = applicationContext
    val scope = rememberCoroutineScope()
    val local = remember { LocalPlayableIndex(app) }
    val emby = remember { EmbyPlayableIndex(app) }
    val tmdbSearch = remember { TmdbSearch(tmdbHome) }
    val search = remember { SearchRepository(app, tmdbSearch, local, emby, scope) }
    val browse = remember { BrowseRepository(tmdbHome, local, emby, { items }, scope) }
    val recent = remember { RecentSearchStore(app) }
    val hot = remember { HotSearchRepository(tmdbSearch) }
    val artwork = remember { CategoryArtworkRepository(tmdbHome) }

    // Emby 渠道：进搜索即建/复用标题池（10 分钟缓存）；池就绪后重搜当前词、重筛「只看可播」。
    // NAS 索引每次进搜索失效重读：后台重扫后不许再出旧数据。
    androidx.compose.runtime.LaunchedEffect(Unit) {
        local.invalidate()
        emby.refreshIfStale(scope)
    }
    androidx.compose.runtime.LaunchedEffect(emby.version.value) {
        if (!emby.ready) return@LaunchedEffect
        if (search.query.isNotBlank()) search.refresh()
        if (browse.filter.onlyPlayable) browse.refresh()
    }

    Box(
        Modifier.fillMaxSize()
            .background(DesignTokens.pageBackground)
            .windowInsetsPadding(WindowInsets.statusBars),
    ) {
        SearchOverlay(
            recent = recent,
            search = search,
            hot = hot,
            artwork = artwork,
            browse = browse,
            onOpenEntry = { openSearchEntry(it) },
            onClose = onBack,
            navSlot = {
                CapsuleNavBar(
                    tabs = ShellTab.displayOrder.map { it.label to "" },
                    selected = ShellTab.displayOrder.indexOfFirst { it.page == ShellTab.Discover.page }
                        .coerceAtLeast(0),
                    onSelect = { onSelectTab(ShellTab.displayOrder[it].page) },
                    onSearch = onBack,
                    onDark = false,
                )
            },
        )
    }
}

/** 结果打开口径与全 App 一致：NAS → 详情（选集/播放），本机 → 统一详情，TMDB → 详情。 */
private fun MainActivity.openSearchEntry(entry: SearchEntry) {
    entry.homeEntry?.let { openNasEntry(it); return }
    entry.emby?.let { startActivity(it.source.detailIntent(this, it.item)); return }
    entry.mediaItem?.let { openPlayer(it); return }
    entry.tmdb?.let { startActivity(MovieDetailActivity.intent(this, it)) }
}