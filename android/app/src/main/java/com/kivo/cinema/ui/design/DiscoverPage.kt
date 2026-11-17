package com.kivo.cinema.ui.design

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
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.State
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.stats.WatchStore
import com.kivo.cinema.tmdb.TmdbGenre
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.tmdb.TmdbPerson
import com.kivo.cinema.tmdb.TmdbProvider
import com.kivo.cinema.tmdb.TmdbStudio
import com.kivo.cinema.tmdb.trailer.TrailerResult
import com.kivo.cinema.ui.design.home.rememberHeroPullZoom
import com.kivo.cinema.ui.design.home.sections.ContinueRow
import com.kivo.cinema.ui.design.home.sections.GenreCards
import com.kivo.cinema.ui.design.home.sections.GenrePosterWall
import com.kivo.cinema.ui.design.home.sections.HomeSectionHeader
import com.kivo.cinema.ui.design.home.sections.PeopleRow
import com.kivo.cinema.ui.design.home.sections.PlatformCards
import com.kivo.cinema.ui.design.home.sections.PosterGrid
import com.kivo.cinema.ui.design.home.sections.PosterSection
import com.kivo.cinema.ui.design.home.sections.StudioCards
import com.kivo.cinema.ui.design.home.sections.TopRatedSection
import com.kivo.cinema.ui.design.home.sections.TmdbErrorCard
import com.kivo.cinema.ui.design.home.sections.VarietySection
import com.kivo.cinema.ui.design.home.sections.WideSection

/**
 * 发现页编排：只负责模块顺序、滚动驱动（hero 拉伸/环境色）与分区数据接线。
 *
 * ✅ 各分区的视图实现已迁到 `home/sections/`（PosterSection / WideSection /
 * VarietySection / ContinueRow / HomeHeader + B3 的 6 个新模块），本文件不含卡片排版。
 *
 * 分区顺序（对齐竞品实拍 0330→0335）：
 * hero → 继续观看 → 正在热映 → 正在播出 → 本周趋势 → 近一月·热门综艺 → 今日动漫
 * → 热门人物 → 播出平台 → 分类浏览（+ 海报网格）→ 高分剧集 → 电影公司 → 错误态。
 *
 * 6 个新模块的数据都由 Shell 传入，**默认空列表**：空 ⇒ 整段不渲染（连标题都不出现），
 * 因此 Shell 不接线时页面与 B2 完全一致（零回归）。
 *
 * @param genrePosters 旧版 `TmdbHome.Item` 形态的海报集，仅作兼容保留：当 [genrePosterUrls]
 *   为空而它非空时才渲染（B3R 之后 Shell 只传 URL 形态）。
 * @param genrePosterUrls 分类浏览下方的 5 列海报墙（w342）。传空 ⇒ 只渲染分类卡，
 *   不出现空网格、不发任何图片请求。
 * @param onOpenPerson/onOpenProvider/onOpenGenre/onOpenStudio ◐ 人物/平台/类型/公司详情
 *   尚无对应页面（B5 接），默认空实现；保留参数避免届时再改调用点。
 */
@Composable
@OptIn(ExperimentalFoundationApi::class)
fun DiscoverPage(
    previewEnabled: Boolean = false,
    continueItems: List<Pair<MediaItem, WatchStore.Progress>>,
    heroCandidates: List<TmdbHome.Item>,
    nowPlaying: List<TmdbHome.Item>,
    onTheAir: List<TmdbHome.Item>,
    trending: List<TmdbHome.Item>,
    variety: List<TmdbHome.Item>,
    todayAnime: List<TmdbHome.Item>,
    topPoster: (TmdbHome.Item) -> String?,
    logoFor: (TmdbHome.Item) -> String?,
    trailerFor: (TmdbHome.Item) -> TrailerResult,
    proxyAddress: String?,
    generation: Int,
    posterOf: @Composable (MediaItem) -> (@Composable () -> Unit)?,
    onOpenItem: (MediaItem) -> Unit,
    onOpenTmdbItem: (TmdbHome.Item) -> Unit,
    tmdbError: String? = null,
    onRetryTmdb: () -> Unit = {},
    onAmbientDark: (Boolean) -> Unit = {},
    onShowWatching: () -> Unit = {},
    people: List<TmdbPerson> = emptyList(),
    providers: List<TmdbProvider> = emptyList(),
    genres: List<TmdbGenre> = emptyList(),
    genrePosters: List<TmdbHome.Item> = emptyList(),
    /** 分类海报墙的纯 URL 形态（w342）；无 id/title 可填，故不伪造 [TmdbHome.Item]（T3 消费）。 */
    genrePosterUrls: List<String> = emptyList(),
    topRatedTv: List<TmdbHome.Item> = emptyList(),
    studios: List<TmdbStudio> = emptyList(),
    onOpenPerson: (TmdbPerson) -> Unit = {},
    onOpenProvider: (TmdbProvider) -> Unit = {},
    onOpenGenre: (TmdbGenre) -> Unit = {},
    onOpenStudio: (TmdbStudio) -> Unit = {},
) {
    val listState = rememberLazyListState()
    val density = androidx.compose.ui.platform.LocalDensity.current
    val pull = rememberHeroPullZoom(
        atTop = { listState.firstVisibleItemIndex == 0 && listState.firstVisibleItemScrollOffset == 0 },
        maxPullPx = with(density) { 180.dp.toPx() })
    val pullDp = pull.distance.value / density.density
    val heroScrollPx = { if (listState.firstVisibleItemIndex == 0)
        listState.firstVisibleItemScrollOffset.toFloat() else 0f }

    var heroFieldColor by remember { mutableStateOf(DesignTokens.pageBackground) }
    val scrimColor: State<Color> = remember {
        derivedStateOf {
            val scrollDp = if (listState.firstVisibleItemIndex == 0)
                listState.firstVisibleItemScrollOffset / density.density else 390f
            com.kivo.cinema.ui.design.home.HomeAtmosphere.background(heroFieldColor, scrollDp)
        }
    }
    // 页面底 = **hero 海报吸色 → 向下过渡到白**（REX 的真实做法，HomeAtmosphere）。
    // 不要换成固定六页渐变：那样会丢掉「首屏跟着当前海报走」的氛围（2026-11-17 真机对比确认）。
    // hero 区仍按海报吸色决定 chrome 明暗与图内遮罩。
    val chromeDark = com.kivo.cinema.ui.design.home.HomeAtmosphere.lightForeground(scrimColor.value)
    val chromeFg = if (chromeDark) Color.White else DesignTokens.textMain
    val chromeSub = if (chromeDark) Color.White.copy(alpha = .72f) else DesignTokens.textSub

    // 317 · 适配成中立 HeroCard（HeroPage 不再依赖 TmdbHome.Item）。
    // DISC-08 ✅：hero 整卡可点 → 按 id 反查回 TmdbHome.Item 再走详情。
    val heroCards = remember(heroCandidates) {
        heroCandidates.map { it ->
            com.kivo.cinema.ui.design.home.HeroCard(
                id = it.id.toString(),
                title = it.title,
                heroUrl = topPoster(it) ?: it.posterHeroUrl,
                metaLine = it.metaLine,
                overview = it.overview,
                rating = it.rating.toDouble(),
                ratingSource = "TMDB",
                trailerKey = it.id,
                isSeries = it.isTv,
            )
        }
    }

    Box(Modifier.fillMaxSize()) {
      // 页面底 = scrimColor（hero 海报吸色 → 随滚动渐变到 pageBackground）。
      // ✅ REX 0454 实测：首屏整页 = 海报吸色 #532C14，与 hero 底部渐变无缝；
      // 画固定 pageBackground 会在 hero 结束处出现「吸色深灰 → 浅灰白」硬断层
      // （2026-11-18 真机采样：y=545dp #3D3E48 → y=600dp #F2F2F7）。
      Box(Modifier.fillMaxSize().drawBehind { drawRect(scrimColor.value) })
      CompositionLocalProvider(LocalOverscrollConfiguration provides null) {
        LazyColumn(
            state = listState,
            modifier = Modifier.fillMaxSize().nestedScroll(pull.connection),
            contentPadding = PaddingValues(bottom = 100.dp),
        ) {
            // Keep the first slot stable while network data arrives: inserting it later
            // would preserve the next item's anchor and open the page below the hero.
            item(key = "hero") {
                if (heroCandidates.isEmpty()) Box(
                    Modifier.fillMaxWidth().height(HERO_HEIGHT_DP.dp),
                    contentAlignment = Alignment.Center,
                ) {
                    if (tmdbError == null) Text("正在加载推荐影片…", color = chromeSub, fontSize = 14.sp)
                    else TmdbErrorCard(tmdbError, onRetryTmdb)
                }
                else HeroCarousel(
                    cards = heroCards,
                    onAmbient = { heroFieldColor = it },
                    dotsColor = chromeFg, scrimColor = scrimColor,
                    pullDp = pullDp,
                    scrollPx = heroScrollPx,
                    previewVisible = listState.firstVisibleItemIndex == 0 &&
                        listState.firstVisibleItemScrollOffset < 350 * density.density,
                    generation = generation, previewEnabled = previewEnabled,
                    trailerFor = trailerFor, proxyAddress = proxyAddress,
                    trailerItemFor = { card ->
                        heroCandidates.firstOrNull { it.id.toString() == card.id }
                    },
                    logoLoader = { card ->
                        heroCandidates.firstOrNull { it.id.toString() == card.id }?.let { logoFor(it) }
                    },
                    onOpen = { card ->
                        heroCandidates.firstOrNull { it.id.toString() == card.id }?.let(onOpenTmdbItem)
                    })
            }
            if (continueItems.isNotEmpty()) {
                item(key = "continue") {
                    ContinueRow(continueItems, chromeFg, chromeSub, posterOf, onOpenItem, onShowWatching)
                }
            }
            // 海报节（正在热映 / 正在播出）+ 宽卡节（本周趋势）
            listOf(
                "正在热映" to nowPlaying,
                "正在播出" to onTheAir,
            ).forEach { (title, list) ->
                if (list.isNotEmpty()) {
                    item(key = title) { Box { PosterSection(title, list, chromeFg, onOpenTmdbItem) } }
                }
            }
            if (trending.isNotEmpty()) {
                item(key = "本周趋势") { Box { WideSection(trending, chromeFg, onOpenTmdbItem) } }
            }
            if (variety.isNotEmpty()) {
                item(key = "近一月·热门综艺") { Box { VarietySection(variety, chromeFg, onOpenTmdbItem) } }
            }
            if (todayAnime.isNotEmpty()) {
                item(key = "今日动漫") { Box { PosterSection("今日动漫", todayAnime, chromeFg, onOpenTmdbItem) } }
            }
            // B3 新模块（顺序 = IMG_0330→0335）：空数据整段不渲染。
            if (people.isNotEmpty()) {
                homeSection("热门人物", "热门人物", chromeFg) { PeopleRow(people, chromeFg, onOpenPerson) }
            }
            if (providers.isNotEmpty()) {
                homeSection("播出平台", "播出平台", chromeFg) {
                    PlatformCards(providers, chromeFg, onOpenProvider)
                }
            }
            if (genres.isNotEmpty()) {
                homeSection("分类浏览", "分类浏览", chromeFg) {
                    GenreCards(genres, chromeFg, onOpenGenre)
                    // B3R：URL 形态优先（Shell 已接线）；旧 Item 形态仅作兼容回退。
                    if (genrePosterUrls.isNotEmpty()) {
                        Spacer(Modifier.height(12.dp))
                        // onOpen 空实现：页面层暂无「按 url 打开详情」，B5 接详情后替换。
                        GenrePosterWall(genrePosterUrls, chromeFg, onOpen = {})
                    } else if (genrePosters.isNotEmpty()) {
                        Spacer(Modifier.height(12.dp))
                        PosterGrid(genrePosters, fg = chromeFg, onOpen = onOpenTmdbItem)
                    }
                }
            }
            if (topRatedTv.isNotEmpty()) {
                item(key = "高分剧集") { TopRatedSection(topRatedTv, chromeFg, onOpenTmdbItem) }
            }
            if (studios.isNotEmpty()) {
                homeSection("电影公司", "电影公司", chromeFg) {
                    StudioCards(studios, chromeFg, onOpenStudio)
                }
            }
            // 错误态（home.error.network.* 五类原文 + 重试——M04-S04，不可合并）
            if (tmdbError != null && heroCandidates.isNotEmpty()) {
                item(key = "error") { Box { TmdbErrorCard(tmdbError, onRetryTmdb) } }
            }
        }
      }
    }

    // 导航玻璃/状态栏随 chrome 组翻转（0138 暗→0139 亮）
    LaunchedEffect(chromeDark) { onAmbientDark(chromeDark) }
}

/**
 * 带标题的分区块：13dp / 标题 / 16dp 与旧 section 完全同距，保证新旧模块节奏一致。
 * body 在 `item` 内部绘制（不可有多个 distinct key），因此分类浏览的「卡行 + 网格」共享一个 item。
 */
private fun LazyListScope.homeSection(
    key: String,
    title: String,
    fg: Color,
    body: @Composable () -> Unit,
) {
    item(key = key) {
        Column {
            Spacer(Modifier.height(13.dp))
            HomeSectionHeader(title, fg)
            Spacer(Modifier.height(16.dp))
            body()
        }
    }
}

/**
 * 继续卡次行的来源名（E10「3 天前 · iLoveHD」格式）。
 *
 * ✅ http(s) 条目取 `URI.host`（拿不到时「在线」）；`file://` / `/storage` / `/sdcard`
 * 以及其余一律「本地」。◐ 真实平台名需要来源元数据（B5 接 SourceRepository 反查），
 * 在那之前不允许把「本地」写成假的服务名。
 */
internal fun sourceOf(item: MediaItem): String {
    val path = item.path
    if (path.startsWith("http://", ignoreCase = true) || path.startsWith("https://", ignoreCase = true)) {
        return runCatching { java.net.URI(path).host }.getOrNull()?.takeIf { it.isNotBlank() } ?: "在线"
    }
    return "本地"
}

/** TMDB 错误态卡见 `home/sections/TmdbErrorCard.kt`（已等值迁出，保持五类原文）。 */