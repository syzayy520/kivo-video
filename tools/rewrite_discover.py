#!/usr/bin/env python3
"""237：发现页重写为 TMDB 节序（hero 轮播 + 继续观看 + 热映/播出/趋势 + 本地影片）。"""
import io

P = r'app/src/main/java/com/kivo/cinema/ui/design/Pages.kt'
s = io.open(P, encoding='utf-8').read()

start = s.find('/** 发现首页')
end = s.find('/** 资源库页')
assert start > 0 and end > start, (start, end)

new_discover = r'''/**
 * 发现首页（grid_01/grid_02/浏览页帧实测 + 用户指定 IA）：
 * hero 轮播（TMDB trending，标题叠印+类型·日期+简介+评分行+圆点，自动 5s per M04-S01）
 * → 继续观看（本地进度数据）→ 正在热映 → 正在播出 → 本周趋势（全部 TMDB）。
 * 无 key/无网 → TMDB 节整体隐藏（不显空壳）；本地节独立存在。
 */
@Composable
fun DiscoverPage(
    items: List<MediaItem>,
    continueItems: List<Pair<MediaItem, com.kivo.cinema.stats.WatchStore.Progress>>,
    heroCandidates: List<com.kivo.cinema.tmdb.TmdbHome.Item>,
    nowPlaying: List<com.kivo.cinema.tmdb.TmdbHome.Item>,
    onTheAir: List<com.kivo.cinema.tmdb.TmdbHome.Item>,
    trending: List<com.kivo.cinema.tmdb.TmdbHome.Item>,
    onOpenItem: (MediaItem) -> Unit,
    onOpenLibrary: () -> Unit,
    onOpenTmdbItem: (com.kivo.cinema.tmdb.TmdbHome.Item) -> Unit,
) {
    val pad = DesignTokens.screenPad.dp
    LazyVerticalGrid(
        columns = GridCells.Fixed(1),
        modifier = Modifier.fillMaxSize(),
        contentPadding = androidx.compose.foundation.layout.PaddingValues(bottom = 10.dp),
    ) {
        // hero 轮播（TMDB）
        if (heroCandidates.isNotEmpty()) {
            item(span = { androidx.compose.foundation.lazy.grid.GridItemSpan(maxLineSpan) }) {
                HeroCarousel(heroCandidates)
            }
        }
        // 继续观看（本地）
        if (continueItems.isNotEmpty()) {
            item(span = { androidx.compose.foundation.lazy.grid.GridItemSpan(maxLineSpan) }) {
                Column {
                    Spacer(Modifier.height(10.dp))
                    SectionHeader("继续观看", onMore = onOpenLibrary)
                    Spacer(Modifier.height(10.dp))
                    LazyRow(horizontalArrangement = Arrangement.spacedBy(pad),
                        contentPadding = androidx.compose.foundation.layout.PaddingValues(horizontal = pad)) {
                        rowItems(continueItems) { (item, prog) ->
                            ContinueCard(
                                title = item.title,
                                meta = relDays(item.addedSec),
                                seenText = "看到 " + fmtSeen(prog.positionMs),
                                progress = if (prog.durationMs > 0)
                                    prog.positionMs.toFloat() / prog.durationMs else 0f,
                                posterBlock = { posterOf(item)?.invoke() },
                                onClick = { onOpenItem(item) },
                            )
                        }
                    }
                }
            }
        }
        // TMDB 三节（帧序：正在热映 → 正在播出 → 本周趋势）
        listOf("正在热映" to nowPlaying, "正在播出" to onTheAir, "本周趋势" to trending)
            .forEach { (title, list) ->
                if (list.isNotEmpty()) {
                    item(span = { androidx.compose.foundation.lazy.grid.GridItemSpan(maxLineSpan) }) {
                        Column {
                            Spacer(Modifier.height(10.dp))
                            SectionHeader(title, onMore = null)
                            Spacer(Modifier.height(10.dp))
                            LazyRow(horizontalArrangement = Arrangement.spacedBy(pad),
                                contentPadding = androidx.compose.foundation.layout.PaddingValues(horizontal = pad)) {
                                rowItems(list) { m ->
                                    TmdbPosterCard(m) { onOpenTmdbItem(m) }
                                }
                            }
                        }
                    }
                }
            }
        // 本地影片节
        if (items.isNotEmpty()) {
            item(span = { androidx.compose.foundation.lazy.grid.GridItemSpan(maxLineSpan) }) {
                Column {
                    Spacer(Modifier.height(10.dp))
                    SectionHeader("本地影片", onMore = onOpenLibrary)
                    Spacer(Modifier.height(10.dp))
                    LazyRow(horizontalArrangement = Arrangement.spacedBy(pad),
                        contentPadding = androidx.compose.foundation.layout.PaddingValues(horizontal = pad)) {
                        rowItems(items.take(10)) { item ->
                            DesignPosterCard(
                                title = item.title,
                                date = formatDate(item.addedSec),
                                posterBlock = { posterOf(item)?.invoke() },
                                onClick = { onOpenItem(item) },
                            )
                        }
                    }
                }
            }
        }
    }
}

/** 相对时间（grid_01 帧："3 天前 · 来源"语义）。 */
internal fun relDays(addedSec: Long): String {
    val days = (System.currentTimeMillis() / 1000 - addedSec) / 86400
    return when {
        days <= 0L -> "今天"
        days == 1L -> "昨天"
        days < 30 -> "" + days + " 天前"
        else -> formatDate(addedSec)
    }
}

/** hero 轮播（grid_01 帧结构 + M04-S01 动效参数：0.4s spring(0.5,0.85)，自动 5s，swipe 打断）。 */
@Composable
internal fun HeroCarousel(candidates: List<com.kivo.cinema.tmdb.TmdbHome.Item>) {
    val pagerState = remember { androidx.compose.foundation.pager.PagerState { candidates.size } }
    LaunchedEffect(candidates.size) {
        if (candidates.size > 1) {
            while (true) {
                kotlinx.coroutines.delay(5000)
                val next = (pagerState.currentPage + 1) % candidates.size
                pagerState.animateScrollToPage(
                    next,
                    animationSpec = androidx.compose.animation.core.spring(0.5f, 0.85f))
            }
        }
    }
    Column {
        androidx.compose.foundation.pager.HorizontalPager(
            state = pagerState,
            modifier = Modifier.fillMaxWidth(),
        ) { idx ->
            val m = candidates[idx]
            Box(
                Modifier
                    .fillMaxWidth()
                    .aspectRatio(16f / 10f)
            ) {
                TmdbImage(m.backdropUrl, m.title, Modifier.fillMaxSize())
                Box(
                    Modifier
                        .fillMaxSize()
                        .background(androidx.compose.ui.graphics.Brush.verticalGradient(
                            listOf(Color.Transparent,
                                Color.Black.copy(alpha = 0.45f),
                                Color.Transparent)))
                )
                Text(m.title, fontSize = 30.sp, fontWeight = FontWeight.Bold,
                    color = Color.White, maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                    modifier = Modifier
                        .align(Alignment.BottomStart)
                        .padding(horizontal = pad, vertical = 10.dp))
            }
        }
        Spacer(Modifier.height(10.dp))
        val cur = candidates.getOrNull(pagerState.currentPage) ?: candidates.firstOrNull()
        if (cur != null) {
            Text(cur.date, fontSize = DesignTokens.fontHeroMeta.sp,
                color = DesignTokens.textMain,
                modifier = Modifier.fillMaxWidth(),
                textAlign = androidx.compose.ui.text.style.TextAlign.Center)
            Spacer(Modifier.height(6.dp))
            if (cur.overview.isNotBlank()) {
                Text(cur.overview, fontSize = DesignTokens.fontHeroSynopsis.sp,
                    color = DesignTokens.textMain, maxLines = 2,
                    overflow = TextOverflow.Ellipsis, lineHeight = 21.sp,
                    modifier = Modifier.fillMaxWidth().padding(horizontal = 16.dp),
                    textAlign = androidx.compose.ui.text.style.TextAlign.Center)
                Spacer(Modifier.height(6.dp))
            }
            RatingBadges(listOf("TMDB" to cur.rating),
                modifier = Modifier.fillMaxWidth(), alignCenter = true)
            Spacer(Modifier.height(10.dp))
        }
        // 圆点指示器（活动黑大，帧实测 ~11 点）
        Row(horizontalArrangement = Arrangement.Center,
            modifier = Modifier.fillMaxWidth()) {
            candidates.forEachIndexed { i, _ ->
                Box(
                    Modifier
                        .padding(horizontal = 4.dp)
                        .size(if (i == pagerState.currentPage) 9.dp else 6.dp)
                        .clip(CircleShape)
                        .background(if (i == pagerState.currentPage)
                            DesignTokens.textMain else Color(0xFFB9B9BE))
                )
            }
        }
    }
}

/** TMDB 海报卡（同 DesignPosterCard 规格：90pt 2:3 居中标题/日期）。 */
@Composable
internal fun TmdbPosterCard(
    m: com.kivo.cinema.tmdb.TmdbHome.Item,
    onClick: () -> Unit) {
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        modifier = Modifier
            .width(DesignTokens.posterWidth.dp)
            .clickable { onClick() }
    ) {
        TmdbImage(m.posterUrl, m.title,
            Modifier
                .size(DesignTokens.posterWidth.dp, (DesignTokens.posterWidth * 1.5f).dp)
                .clip(androidx.compose.foundation.shape.RoundedCornerShape(
                    DesignTokens.radiusPoster.dp)))
        Spacer(Modifier.height(6.dp))
        Text(m.title, fontSize = DesignTokens.fontPosterTitle.sp,
            fontWeight = FontWeight.Bold, color = DesignTokens.textMain,
            maxLines = 1, overflow = TextOverflow.Ellipsis)
        Text(m.date, fontSize = DesignTokens.fontPosterDate.sp,
            color = DesignTokens.textSub)
    }
}

'''

s = s[:start] + new_discover + s[end:]
io.open(P, 'w', encoding='utf-8').write(s)
print('discover rewritten, len =', len(s))
