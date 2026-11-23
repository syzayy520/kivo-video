package com.kivo.cinema.ui.design.home.v6

import android.widget.Toast
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.LocalOverscrollConfiguration
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyListScope
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.State
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawBehind
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.asPaddingValues
import androidx.compose.foundation.layout.statusBars
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.library.NasCategory
import com.kivo.cinema.library.NasLibrarySource
import com.kivo.cinema.tmdb.TmdbGenre
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.tmdb.TmdbList
import com.kivo.cinema.tmdb.trailer.TrailerResult
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.HERO_HEIGHT_DP
import com.kivo.cinema.ui.design.HeroCarousel
import com.kivo.cinema.ui.design.home.HomeAtmosphere
import com.kivo.cinema.ui.design.home.rememberHeroCards
import com.kivo.cinema.ui.design.home.rememberHeroPullZoom
import com.kivo.cinema.ui.design.home.sections.TmdbErrorCard
import com.kivo.cinema.ui.design.home.v6.sections.V6ContinueRow
import com.kivo.cinema.ui.design.home.v6.sections.V6GenreCards
import com.kivo.cinema.ui.design.home.v6.sections.V6LibraryRow
import com.kivo.cinema.ui.design.home.v6.sections.V6PosterRow
import com.kivo.cinema.ui.design.home.v6.sections.V6RankedRow
import com.kivo.cinema.ui.design.home.v6.sections.V6WideRow
import com.kivo.cinema.ui.design.home.v6.sections.monthDay

/**
 * 首页 v6 编排：只负责区块顺序、滚动驱动（hero 拉伸 / 取色底 / 舞台明暗）与数据接线，不含卡片排版。
 *
 * 顺序：Hero → 继续观看 → 片库 → 院线新片(S) → 热门剧集(16:9) → 高分电影(排行) → 近期热门韩剧(16:9)
 *       → 热门华语电影(S) → 近期热门日剧(16:9) → 今日动漫(S) → 高分剧集(排行) → 分类浏览。
 * 节奏规则：相邻两行不用同一种卡形（S 海报 / 16:9 / 排行交替）。
 * 取色底与明暗翻转沿用旧版已验收的 [HomeAtmosphere]（一次性阈值翻转，导航玻璃由 [onAmbientDark] 驱动）。
 * 家族树：docs/design/ui/home-v6/family-tree.md
 */
@Composable
@OptIn(ExperimentalFoundationApi::class)
fun DiscoverPageV6(
    previewEnabled: Boolean,
    continueItems: List<HomeEntry>,
    nasSections: List<NasLibrarySource.Section>,
    heroCandidates: List<TmdbHome.Item>,
    nowPlaying: List<TmdbHome.Item>,
    onTheAir: List<TmdbHome.Item>,
    todayAnime: List<TmdbHome.Item>,
    topRatedTv: List<TmdbHome.Item>,
    topRatedMovie: List<TmdbHome.Item>,
    chineseMovie: List<TmdbHome.Item>,
    koreanTv: List<TmdbHome.Item>,
    japaneseTv: List<TmdbHome.Item>,
    genres: List<TmdbGenre>,
    topPoster: (TmdbHome.Item) -> String?,
    logoFor: (TmdbHome.Item) -> String?,
    trailerFor: (TmdbHome.Item) -> TrailerResult,
    proxyAddress: String?,
    generation: Int,
    posterOf: @Composable (HomeEntry) -> (@Composable () -> Unit)?,
    onOpenItem: (HomeEntry) -> Unit,
    onOpenNasItem: (HomeEntry) -> Unit,
    onOpenTmdbItem: (TmdbHome.Item) -> Unit,
    tmdbError: String?,
    onRetryTmdb: () -> Unit,
    onAmbientDark: (Boolean) -> Unit,
    onShowWatching: () -> Unit,
    /** 「查看全部」：打开某个 TMDB 榜单的分页网格页。 */
    onOpenList: (TmdbList, String) -> Unit,
    /** NAS 分类「查看全部」。 */
    onOpenNasCategory: (NasCategory) -> Unit,
) {
    val context = LocalContext.current
    val density = LocalDensity.current
    val listState = rememberLazyListState()
    val pull = rememberHeroPullZoom(
        atTop = { listState.firstVisibleItemIndex == 0 && listState.firstVisibleItemScrollOffset == 0 },
        maxPullPx = with(density) { 180.dp.toPx() })
    val heroScrollPx = { if (listState.firstVisibleItemIndex == 0) listState.firstVisibleItemScrollOffset.toFloat() else 0f }

    var heroFieldColor by remember { mutableStateOf(DesignTokens.pageBackground) }
    val scrimColor: State<Color> = remember {
        derivedStateOf {
            val scrollDp = if (listState.firstVisibleItemIndex == 0)
                listState.firstVisibleItemScrollOffset / density.density else 390f
            HomeAtmosphere.background(heroFieldColor, scrollDp)
        }
    }
    val dark = HomeAtmosphere.lightForeground(scrimColor.value)
    val fg = if (dark) Color.White else DesignTokens.textMain
    val sub = if (dark) Color.White.copy(alpha = .72f) else DesignTokens.textSub

    val heroCards = rememberHeroCards(heroCandidates, topPoster)
    val genreSoon: (TmdbGenre) -> Unit = {
        Toast.makeText(context, "「${it.name}」分类页还在开发中", Toast.LENGTH_SHORT).show()
    }

    Box(Modifier.fillMaxSize()) {
        Box(Modifier.fillMaxSize().drawBehind { drawRect(scrimColor.value) })
        CompositionLocalProvider(LocalOverscrollConfiguration provides null) {
            LazyColumn(
                state = listState,
                modifier = Modifier.fillMaxSize().nestedScroll(pull.connection),
                contentPadding = PaddingValues(bottom = 112.dp),
            ) {
                item(key = "hero") {
                    if (heroCandidates.isEmpty()) Box(
                        Modifier.fillMaxWidth().height(HERO_HEIGHT_DP.dp), contentAlignment = Alignment.Center,
                    ) {
                        if (tmdbError == null) Text("正在加载推荐影片…", color = sub, fontSize = 14.sp)
                        else TmdbErrorCard(tmdbError, onRetryTmdb)
                    }
                    else HeroCarousel(
                        cards = heroCards,
                        onAmbient = { heroFieldColor = it },
                        dotsColor = fg, scrimColor = scrimColor,
                        pullDp = pull.distance.value / density.density,
                        scrollPx = heroScrollPx,
                        previewVisible = listState.firstVisibleItemIndex == 0 &&
                            listState.firstVisibleItemScrollOffset < 350 * density.density,
                        generation = generation, previewEnabled = previewEnabled,
                        trailerFor = trailerFor, proxyAddress = proxyAddress,
                        trailerItemFor = { card -> heroCandidates.firstOrNull { it.id.toString() == card.id } },
                        logoLoader = { card -> heroCandidates.firstOrNull { it.id.toString() == card.id }?.let(logoFor) },
                        onOpen = { card -> heroCandidates.firstOrNull { it.id.toString() == card.id }?.let(onOpenTmdbItem) },
                    )
                }
                v6Section("continue", first = true) {
                    V6ContinueRow(continueItems, fg, sub, posterOf, onOpenItem, onShowWatching)
                }
                nasSections.forEach { s ->
                    v6Section("nas-${s.category.key}") {
                        V6LibraryRow(s.category.label, s.entries, fg, sub, posterOf, onOpenNasItem) { onOpenNasCategory(s.category) }
                    }
                }
                val more = { kind: TmdbList, title: String -> { onOpenList(kind, title) } }
                if (nowPlaying.isNotEmpty()) v6Section("now") {
                    V6PosterRow("院线新片", nowPlaying, fg, sub, { monthDay(it.date) }, onOpenTmdbItem, more(TmdbList.NOW_PLAYING, "院线新片"))
                }
                if (onTheAir.isNotEmpty()) v6Section("tv") {
                    V6WideRow("热门剧集", onTheAir, fg, sub, onOpenTmdbItem, more(TmdbList.ON_THE_AIR, "热门剧集"))
                }
                if (topRatedMovie.isNotEmpty()) v6Section("top-movie") {
                    V6RankedRow("高分电影", topRatedMovie, fg, sub, onOpenTmdbItem, more(TmdbList.TOP_RATED_MOVIE, "高分电影"))
                }
                if (koreanTv.isNotEmpty()) v6Section("ko") {
                    V6WideRow("近期热门韩剧", koreanTv, fg, sub, onOpenTmdbItem, more(TmdbList.KOREAN_TV, "近期热门韩剧"))
                }
                if (chineseMovie.isNotEmpty()) v6Section("zh") {
                    V6PosterRow("热门华语电影", chineseMovie, fg, sub, { it.year() }, onOpenTmdbItem, more(TmdbList.CHINESE_MOVIE, "热门华语电影"))
                }
                if (japaneseTv.isNotEmpty()) v6Section("ja") {
                    V6WideRow("近期热门日剧", japaneseTv, fg, sub, onOpenTmdbItem, more(TmdbList.JAPANESE_TV, "近期热门日剧"))
                }
                if (todayAnime.isNotEmpty()) v6Section("anime") {
                    V6PosterRow("今日动漫", todayAnime, fg, sub, { it.year() }, onOpenTmdbItem, more(TmdbList.TODAY_ANIME, "今日动漫"))
                }
                if (topRatedTv.isNotEmpty()) v6Section("top") {
                    V6RankedRow("高分剧集", topRatedTv, fg, sub, onOpenTmdbItem, more(TmdbList.TOP_RATED_TV, "高分剧集"))
                }
                if (genres.isNotEmpty()) v6Section("genres") { V6GenreCards(genres, fg, sub, genreSoon) }
                if (tmdbError != null && heroCandidates.isNotEmpty()) {
                    item(key = "error") { Box { TmdbErrorCard(tmdbError, onRetryTmdb) } }
                }
            }
        }
        // 状态栏遮罩：亮场时淡入（参考作同款），内容滑到顶部不再与时间 / 电量叠字。
        val veil = DesignTokens.pageBackground
        Box(
            Modifier.fillMaxWidth()
                .height(WindowInsets.statusBars.asPaddingValues().calculateTopPadding() + 22.dp)
                .graphicsLayer { alpha = if (dark) 0f else 1f }
                .background(Brush.verticalGradient(listOf(veil.copy(alpha = .96f), veil.copy(alpha = .78f), Color.Transparent))),
        )
    }
    LaunchedEffect(dark) { onAmbientDark(dark) }
}

/** 区块外壳：统一的区块间距（继续观看紧贴 hero，只留 18dp）。 */
private fun LazyListScope.v6Section(key: String, first: Boolean = false, body: @Composable () -> Unit) {
    item(key = key) {
        Column {
            Spacer(Modifier.height(if (first) 18.dp else V6Metrics.sectionGap))
            body()
        }
    }
}
