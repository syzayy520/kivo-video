package com.kivo.cinema

import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.produceState
import com.kivo.cinema.tmdb.TmdbGenre
import com.kivo.cinema.tmdb.TmdbGenres
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.tmdb.TmdbList
import com.kivo.cinema.tmdb.TmdbPeople
import com.kivo.cinema.tmdb.TmdbPerson
import com.kivo.cinema.tmdb.TmdbProvider
import com.kivo.cinema.tmdb.TmdbProviders
import com.kivo.cinema.tmdb.TmdbStudio
import com.kivo.cinema.tmdb.TmdbStudios
import com.kivo.cinema.ui.design.AppearancePrefs
import com.kivo.cinema.ui.design.DiscoverPage
import com.kivo.cinema.ui.design.home.v6.DiscoverPageV6
import com.kivo.cinema.ui.design.home.v6.list.HomeListActivity
import com.kivo.cinema.ui.design.home.v6.list.NasCategoryActivity
import com.kivo.cinema.ui.detail.MovieDetailActivity
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/** 首屏新增请求预算：只补前 4 个平台的拼贴海报、前 6 个类型的底图（失败项保持空，不造假）。 */
private const val PROVIDER_TILE_COUNT = 4
private const val GENRE_BACKDROP_COUNT = 6
/** v6 分类图卡：19 个类型全部配互不重复的剧照（串行 19 次请求，12h 缓存；缺图才退回纯色底）。 */
private const val GENRE_BACKDROP_COUNT_V6 = Int.MAX_VALUE

/**
 * 发现 Tab 的数据接线：按「首页新版（预览）」开关选 v6 或原版，并只加载该版用得到的 TMDB 分区。
 * 从 MainActivityShell 迁出（Shell 已到 300 行门限；「发现页接线」是独立变化原因）。
 */
@Composable
internal fun DiscoverTab(
    activity: MainActivity,
    state: HomeShellState,
    previewEnabled: Boolean,
    onShowWatching: () -> Unit,
) {
    if (AppearancePrefs.homeV6) DiscoverTabV6(activity, state, previewEnabled, onShowWatching)
    else DiscoverTabLegacy(activity, state, previewEnabled, onShowWatching)
}

@Composable
private fun DiscoverTabV6(activity: MainActivity, state: HomeShellState, previewEnabled: Boolean, onShowWatching: () -> Unit) {
    val tmdbClient = activity.tmdb
    val genres by produceState(emptyList<TmdbGenre>(), tmdbClient) {
        value = withContext(Dispatchers.IO) {
            TmdbGenres.fillDistinctBackdrops(tmdbClient, TmdbGenres.movie(tmdbClient), max = GENRE_BACKDROP_COUNT_V6)
        }
    }
    val topRatedTv by produceState(emptyList<TmdbHome.Item>(), tmdbClient) {
        value = withContext(Dispatchers.IO) { activity.tmdbHome.topRatedTv().items }
    }
    val topRatedMovie by rememberTmdbList(activity, TmdbList.TOP_RATED_MOVIE)
    val chineseMovie by rememberTmdbList(activity, TmdbList.CHINESE_MOVIE)
    val koreanTv by rememberTmdbList(activity, TmdbList.KOREAN_TV)
    val japaneseTv by rememberTmdbList(activity, TmdbList.JAPANESE_TV)
    DiscoverPageV6(
        previewEnabled = previewEnabled,
        continueItems = state.homeEntries,
        nasSections = state.nasSections,
        heroCandidates = state.heroCandidates,
        nowPlaying = state.nowPlaying,
        onTheAir = state.onTheAir,
        todayAnime = state.todayAnime,
        topRatedTv = topRatedTv,
        topRatedMovie = topRatedMovie,
        chineseMovie = chineseMovie,
        koreanTv = koreanTv,
        japaneseTv = japaneseTv,
        genres = genres,
        topPoster = { activity.tmdbHome.topVotedPosterUrl(it) },
        logoFor = { activity.tmdbHome.logoUrlFor(it) },
        trailerFor = { activity.tmdbHome.trailerFor(it) },
        proxyAddress = activity.tmdb.proxyAddr(),
        generation = state.homeGeneration,
        posterOf = { activity.entryPoster(it) },
        onOpenItem = { activity.openHomeEntry(it) },
        onOpenNasItem = { activity.openNasEntry(it) },
        onOpenTmdbItem = { activity.startActivity(MovieDetailActivity.intent(activity, it)) },
        tmdbError = state.tmdbError,
        onRetryTmdb = { activity.refreshHomeFeed() },
        onAmbientDark = { state.ambientDark = it },
        onShowWatching = onShowWatching,
        onOpenList = { kind, title -> activity.startActivity(HomeListActivity.intent(activity, kind, title)) },
        onOpenNasCategory = { activity.startActivity(NasCategoryActivity.intent(activity, it)) },
    )
}

/** 某个 TMDB 榜单第 1 页（IO 线程；无网 / 无 key ⇒ 空列表 ⇒ 该行整段不显示，不造假）。 */
@Composable
private fun rememberTmdbList(activity: MainActivity, kind: TmdbList) =
    produceState(emptyList<TmdbHome.Item>(), activity.tmdb, kind) {
        value = withContext(Dispatchers.IO) { activity.tmdbHome.list(kind).items }
    }

/** 原版发现页接线（自 MainActivityShell 原样迁出，行为不变）。 */
@Composable
private fun DiscoverTabLegacy(activity: MainActivity, state: HomeShellState, previewEnabled: Boolean, onShowWatching: () -> Unit) {
    // B3：6 个新模块的数据（无 key/无网 → 空列表 → 模块整段隐藏，不造假）。
    // TmdbHome.get 自带 12h 列表缓存，回页只是重建快照。
    val tmdbClient = activity.tmdb
    val tmdbHome = activity.tmdbHome
    val people by produceState(emptyList<TmdbPerson>(), tmdbClient) {
        value = withContext(Dispatchers.IO) { TmdbPeople.popular(tmdbClient) }
    }
    val providers by produceState(emptyList<TmdbProvider>(), tmdbClient) {
        // watch_region=CN 实测 0 条（TMDB 无大陆平台数据）；取 US 段（实测 296 条，
        // Netflix/Disney+ 等国际平台齐平竞品展示口径）。◐ 区域口径是取舍，不是事实断言。
        value = withContext(Dispatchers.IO) {
            TmdbProviders.movie(tmdbClient, region = "US").mapIndexed { i, p ->
                if (i < PROVIDER_TILE_COUNT) p.copy(titles = TmdbProviders.titles(tmdbClient, p.id)) else p
            }
        }
    }
    val genres by produceState(emptyList<TmdbGenre>(), tmdbClient) {
        value = withContext(Dispatchers.IO) {
            TmdbGenres.fillBackdrops(tmdbClient, TmdbGenres.movie(tmdbClient), max = GENRE_BACKDROP_COUNT)
        }
    }
    // 分类海报墙：第一个类型的人气前 10 张；genres 未就绪时为空、不发请求（见 DiscoverPage genrePosterUrls）。
    val genrePosterUrls by produceState(emptyList<String>(), tmdbClient, genres) {
        value = withContext(Dispatchers.IO) {
            genres.firstOrNull()?.let { TmdbGenres.posters(tmdbClient, it.id) }.orEmpty()
        }
    }
    val topRatedTv by produceState(emptyList<TmdbHome.Item>(), tmdbClient) {
        value = withContext(Dispatchers.IO) { tmdbHome.topRatedTv().items }
    }
    val studios by produceState(emptyList<TmdbStudio>(), tmdbClient) {
        value = withContext(Dispatchers.IO) { TmdbStudios.featured(tmdbClient) }
    }
    DiscoverPage(
        previewEnabled = previewEnabled, continueItems = state.homeEntries,
        nasSections = state.nasSections,
        heroCandidates = state.heroCandidates,
        nowPlaying = state.nowPlaying,
        onTheAir = state.onTheAir,
        trending = state.trending,
        variety = state.variety,
        todayAnime = state.todayAnime,
        topPoster = { activity.tmdbHome.topVotedPosterUrl(it) },
        logoFor = { activity.tmdbHome.logoUrlFor(it) },
        trailerFor = { activity.tmdbHome.trailerFor(it) },
        proxyAddress = activity.tmdb.proxyAddr(),
        generation = state.homeGeneration,
        posterOf = { activity.entryPoster(it) },
        onOpenItem = { activity.openHomeEntry(it) },
        onOpenNasItem = { activity.openNasEntry(it) },
        onOpenTmdbItem = { activity.startActivity(MovieDetailActivity.intent(activity, it)) },
        tmdbError = state.tmdbError,
        onRetryTmdb = { activity.refreshHomeFeed() },
        onAmbientDark = { state.ambientDark = it },
        onShowWatching = onShowWatching,
        people = people,
        providers = providers,
        genres = genres,
        genrePosterUrls = genrePosterUrls,
        topRatedTv = topRatedTv,
        studios = studios,
    )
}
