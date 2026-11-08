package com.kivo.cinema.ui.design

import androidx.compose.foundation.background
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.LocalOverscrollConfiguration
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.RoundedCornerShape
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
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.tmdb.trailer.TrailerResult
import com.kivo.cinema.ui.design.home.NavigationBackdropState
import com.kivo.cinema.ui.design.home.navigationSource
import com.kivo.cinema.ui.design.home.rememberHeroPullZoom
import com.kivo.cinema.stats.WatchStore

/** Discovery content and one continuous scroll-driven color field. */
@Composable
@OptIn(ExperimentalFoundationApi::class)
fun DiscoverPage(
    backdrop: NavigationBackdropState? = null,
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
    val chromeDark = com.kivo.cinema.ui.design.home.HomeAtmosphere.lightForeground(scrimColor.value)
    val chromeFg = if (chromeDark) Color.White else DesignTokens.textMain
    val chromeSub = if (chromeDark) Color.White.copy(alpha = .72f) else DesignTokens.textSub

    Box(Modifier.fillMaxSize()) {
      Box(Modifier.fillMaxSize().navigationSource(backdrop).drawBehind { drawRect(scrimColor.value) })
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
                else HeroCarousel(heroCandidates, onAmbient = { heroFieldColor = it },
                    dotsColor = chromeFg, scrimColor = scrimColor,
                    topPoster = topPoster, logoFor = logoFor, pullDp = pullDp,
                    scrollPx = heroScrollPx,
                    previewVisible = listState.firstVisibleItemIndex == 0 &&
                        listState.firstVisibleItemScrollOffset < 350 * density.density,
                    generation = generation, previewEnabled = previewEnabled, trailerFor = trailerFor, proxyAddress = proxyAddress)
            }
            // 继续观看
            if (continueItems.isNotEmpty()) {
                item(key = "continue") {
                    Column(Modifier.navigationSource(backdrop)) {
                        Spacer(Modifier.height(13.dp))
                        HomeSectionHeader("继续观看", chromeFg, onShowWatching)
                        Spacer(Modifier.height(12.dp))
                        LazyRow(
                            horizontalArrangement = Arrangement.spacedBy(10.dp),
                            contentPadding = PaddingValues(horizontal = 13.dp),
                        ) {
                            itemsIndexed(continueItems) { _, (item, prog) ->
                                ContinueCard(
                                    title = item.title,
                                    meta = "${relDays(item.addedSec)} · ${sourceOf(item)}",
                                    seenText = "看到 " + fmtSeen(prog.positionMs),
                                    progress = if (prog.durationMs > 0)
                                        prog.positionMs.toFloat() / prog.durationMs else 0f,
                                    posterBlock = { posterOf(item)?.invoke() },
                                    onClick = { onOpenItem(item) },
                                    titleColor = chromeFg,
                                    metaColor = chromeSub,
                                )
                            }
                        }
                    }
                }
            }
            // 海报节（正在热映 / 正在播出）+ 宽卡节（本周趋势）
            listOf(
                "正在热映" to nowPlaying,
                "正在播出" to onTheAir,
            ).forEach { (title, list) ->
                if (list.isNotEmpty()) {
                    item(key = title) { Box(Modifier.navigationSource(backdrop)) { PosterSection(title, list, chromeFg, onOpenTmdbItem) } }
                }
            }
            if (trending.isNotEmpty()) {
                item(key = "本周趋势") { Box(Modifier.navigationSource(backdrop)) { WideSection(trending, chromeFg, onOpenTmdbItem) } }
            }
            if (variety.isNotEmpty()) {
                item(key = "近一月·热门综艺") { Box(Modifier.navigationSource(backdrop)) { VarietySection(variety, chromeFg, onOpenTmdbItem) } }
            }
            if (todayAnime.isNotEmpty()) {
                item(key = "今日动漫") { Box(Modifier.navigationSource(backdrop)) { PosterSection("今日动漫", todayAnime, chromeFg, onOpenTmdbItem) } }
            }
            // 错误态（home.error.network.* 五类原文 + 重试——M04-S04，不可合并）
            if (tmdbError != null && heroCandidates.isNotEmpty()) {
                item(key = "error") { Box(Modifier.navigationSource(backdrop)) { TmdbErrorCard(tmdbError, onRetryTmdb) } }
            }
        }
      }
    }

    // 导航玻璃/状态栏随 chrome 组翻转（0138 暗→0139 亮）
    LaunchedEffect(chromeDark) { onAmbientDark(chromeDark) }
}

/** 本地条目来源名（E10「3 天前 · iLoveHD」格式；MediaStore 条目 = 本地）。 */
internal fun sourceOf(item: MediaItem): String = "本地"

/** Home poster row measured against the 390pt source viewport. */
@Composable
private fun PosterSection(
    title: String,
    list: List<TmdbHome.Item>,
    fg: Color,
    onOpenTmdbItem: (TmdbHome.Item) -> Unit,
) {
    Column {
        Spacer(Modifier.height(13.dp))
        HomeSectionHeader(title, fg)
        Spacer(Modifier.height(16.dp))
        LazyRow(
            horizontalArrangement = Arrangement.spacedBy(12.dp),
            contentPadding = PaddingValues(horizontal = 13.dp),
        ) {
            items(list) { m ->
                TmdbPosterCard(m, fg) { onOpenTmdbItem(m) }
            }
        }
    }
}

private fun chromeSubFor(fg: Color): Color =
    if (fg == Color.White) Color.White.copy(alpha = 0.72f) else DesignTokens.textSub

/** 本周趋势：宽卡 161×90.6 (16:9) 横滑 2.2 列露头（E8），标签「2026 · 恐怖 / 科幻」。 */
@Composable
private fun WideSection(
    list: List<TmdbHome.Item>,
    fg: Color,
    onOpenTmdbItem: (TmdbHome.Item) -> Unit,
) {
    Column {
        Spacer(Modifier.height(13.dp))
        HomeSectionHeader("本周趋势", fg)
        Spacer(Modifier.height(16.dp))
        LazyRow(
            horizontalArrangement = Arrangement.spacedBy(10.dp),
            contentPadding = PaddingValues(horizontal = 13.dp),
        ) {
            items(list) { m ->
                Column(
                    Modifier
                        .width(161.dp)
                        .clickable { onOpenTmdbItem(m) },
                ) {
                    Box(
                        Modifier
                            .size(161.dp, 91.dp)
                            .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
                    ) { TmdbImage(m.backdropUrl, m.title, Modifier.fillMaxSize()) }
                    Spacer(Modifier.height(6.dp))
                    Text(m.title, fontSize = 15.sp, fontWeight = FontWeight.Bold,
                        color = fg, maxLines = 1, overflow = TextOverflow.Ellipsis)
                    Text(m.tagLine(), fontSize = 13.sp, color = chromeSubFor(fg),
                        maxLines = 1, overflow = TextOverflow.Ellipsis)
                }
            }
        }
    }
}

/**
 * 近一月·热门综艺大卡（hero_32 帧实测：全宽玻璃卡 = 顶部特写图（月桂+节题叠印）
 * + 下部 16:9 缩略横排）。详情页 v2（237 §8 勘误 #4：v1 默认模块流）。
 */
@Composable
private fun VarietySection(
    list: List<TmdbHome.Item>,
    fg: Color,
    onOpenTmdbItem: (TmdbHome.Item) -> Unit,
) {
    Column {
        Spacer(Modifier.height(13.dp))
        GlassSurface(Modifier.padding(horizontal = 13.dp)) {
            Column {
                Box(
                    Modifier
                        .fillMaxWidth()
                        .height(150.dp)
                ) {
                    TmdbImage(list.first().backdropUrl, list.first().title,
                        Modifier.fillMaxSize())
                    Box(
                        Modifier
                            .fillMaxSize()
                            .background(Brush.verticalGradient(
                                0.3f to Color.Transparent,
                                1f to Color.Black.copy(alpha = 0.55f))))
                    Column(
                        Modifier.align(Alignment.BottomCenter).padding(bottom = 10.dp),
                        horizontalAlignment = Alignment.CenterHorizontally,
                    ) {
                        Text("近一月 · 热门综艺", fontSize = 20.sp, fontWeight = FontWeight.Bold,
                            color = Color.White)
                        Text("The most popular variety shows in the past month",
                            fontSize = 11.sp, color = Color.White.copy(alpha = 0.8f))
                    }
                }
                Spacer(Modifier.height(8.dp))
                LazyRow(
                    horizontalArrangement = Arrangement.spacedBy(10.dp),
                    contentPadding = PaddingValues(horizontal = 13.dp),
                ) {
                    items(list.take(10)) { m ->
                        Column(Modifier.width(103.dp).clickable { onOpenTmdbItem(m) }) {
                            Box(
                                Modifier
                                    .size(103.dp, 58.dp)
                                    .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
                            ) { TmdbImage(m.backdropUrl, m.title, Modifier.fillMaxSize()) }
                            Spacer(Modifier.height(4.dp))
                            Text(m.title, fontSize = 12.sp, color = DesignTokens.textMain,
                                maxLines = 1, overflow = TextOverflow.Ellipsis)
                            Text(m.tagLine(), fontSize = 11.sp, color = DesignTokens.textSub,
                                maxLines = 1, overflow = TextOverflow.Ellipsis)
                        }
                    }
                }
                Spacer(Modifier.height(10.dp))
            }
        }
    }
}

/** TMDB 错误态卡（home.error.network.* 五类原文文案 + 重试，M04-S04）。 */
@Composable
private fun TmdbErrorCard(kind: String, onRetry: () -> Unit) {
    val kindText = when (kind) {
        "timeout" -> "连接超时，请稍后重试"
        "offline" -> "网络未连接，请检查网络后重试"
        "secure" -> "无法建立安全连接，请检查服务器设置"
        "server" -> "无法连接服务器，请稍后重试"
        else -> "网络连接已中断，请重试"
    }
    Spacer(Modifier.height(20.dp))
    GlassSurface(Modifier.fillMaxWidth().padding(horizontal = 13.dp)) {
        Column(
            Modifier.fillMaxWidth().padding(16.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            Text("无法加载 TMDB 内容", fontSize = 15.sp,
                fontWeight = FontWeight.Bold, color = DesignTokens.textMain)
            Spacer(Modifier.height(4.dp))
            Text(kindText, fontSize = 13.sp, color = DesignTokens.textSub)
            Spacer(Modifier.height(10.dp))
            Text("重试", fontSize = 14.sp, fontWeight = FontWeight.Bold,
                color = DesignTokens.accentGreen,
                modifier = Modifier
                    .clip(RoundedCornerShape(8.dp))
                    .clickable { onRetry() }
                    .padding(horizontal = 16.dp, vertical = 6.dp))
        }
    }
}

/** Home-only 104×156 poster with compact provider labels. */
@Composable
internal fun TmdbPosterCard(
    m: TmdbHome.Item,
    fg: Color = DesignTokens.textMain,
    onClick: () -> Unit,
) {
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        modifier = Modifier
            .width(104.dp)
            .clickable { onClick() },
    ) {
        TmdbImage(m.posterUrl, m.title,
            Modifier
                .size(104.dp, 156.dp)
                .clip(RoundedCornerShape(DesignTokens.radiusPoster.dp)))
        Spacer(Modifier.height(6.dp))
        Text(m.title, fontSize = 12.sp,
            fontWeight = FontWeight.Bold, color = fg,
            maxLines = 1, overflow = TextOverflow.Ellipsis)
        Text(m.year(), fontSize = 11.sp,
            color = chromeSubFor(fg))
    }
}

/** Compact section typography measured in the source's 390pt viewport. */
@Composable
private fun HomeSectionHeader(title: String, fg: Color, onMore: (() -> Unit)? = null) {
    Row(Modifier.fillMaxWidth().height(22.dp).padding(horizontal = 13.dp),
        verticalAlignment = Alignment.CenterVertically) {
        Text(title, fontSize = 16.sp, lineHeight = 22.sp, fontWeight = FontWeight.Bold,
            color = fg, style = androidx.compose.ui.text.TextStyle(
                platformStyle = androidx.compose.ui.text.PlatformTextStyle(includeFontPadding = false)))
        Spacer(Modifier.weight(1f))
        if (onMore != null) Box(
            Modifier.size(28.dp, 22.dp).semantics { contentDescription = "查看全部$title" }
                .clickable(onClick = onMore), contentAlignment = Alignment.CenterEnd,
        ) {
            Canvas(Modifier.size(8.dp, 13.dp)) {
                val inset = 1.dp.toPx()
                drawPath(Path().apply {
                    moveTo(inset, inset)
                    lineTo(size.width - inset, size.height / 2)
                    lineTo(inset, size.height - inset)
                }, fg, style = Stroke(1.7.dp.toPx(), cap = StrokeCap.Round, join = StrokeJoin.Round))
            }
        }
    }
}
