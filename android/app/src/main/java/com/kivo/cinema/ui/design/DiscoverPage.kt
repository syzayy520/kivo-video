package com.kivo.cinema.ui.design

import android.graphics.Bitmap
import androidx.compose.foundation.background
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
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.stats.WatchStore

/**
 * 发现首页（239 §2/§3-v3；IMG_0138/0139/0140 三滚动态 + IMG_0136 全链帧实测）：
 *
 *   hero(585, 羽化) → 继续观看 → 正在热映 → 正在播出 → 本周趋势(宽卡)
 *   → 近一月·热门综艺(大卡) → 今日动漫。本地影片节已删（对照帧无此节）。
 *
 * 背景层（239 §3.2-v3，IMG_0138/0139/0140 三态拟合）：
 *   hero 海报的**重度模糊版**（48px 双线性放大 + 盒模糊，位图自带底部 25% α 渐隐），
 *   视口定位、视差 0.7（盒子顶 = −0.7×S），整体 α 随滚动淡出：
 *   分段线性 (180dp,1)(300dp,0.38)(500dp,0)。
 *   下滑过程 = 清晰海报(羽化) 上滚露出模糊背景，背景再整体淡入页底——三段皆渐变，无硬线。
 *
 * 明暗（239 §3.3-v3）：chrome 组（节标题/卡标签/圆点/导航）= 背景在其屏幕行的有效亮度
 *   <0.45 → 白（0138 白→0139 黑随滚动翻转）；hero 组（meta/简介/徽章/logo）= S=0 行
 *   亮度 <0.55 一次定死整页不变（0138/0139 meta 都白）。
 */
private const val AMBIENT_FADE_A = 180    // dp：整体 α 开始衰减
private const val AMBIENT_FADE_B = 300    // dp：α≈0.75
private const val AMBIENT_FADE_C = 430    // dp：α≈0.37（0140 实测）
private const val AMBIENT_FADE_D = 620    // dp：α=0
private const val CHROME_FLIP_DP = 110f   // chrome 组翻白→黑的滚动点（0138 白/0139 黑夹逼）
private const val FIELD_END_DP = 1100f    // 下缘场结束（内容坐标；底部 29% α 渐隐，无硬线）

/** 下缘场的混白比例 t(S)：0138(S=0) 全暗 (28,10,9)、0139(S≈182) 淡紫 (129,119,123)、
 *  0140(S≈301) 近白 (202,197,204) 三态夹逼的分段线性。 */
private fun fieldMix(scrollDp: Float): Float = when {
    scrollDp <= 182f -> scrollDp / 182f * 0.49f
    scrollDp <= 301f -> 0.49f + (scrollDp - 182f) / 119f * 0.32f
    scrollDp <= 502f -> 0.81f + (scrollDp - 301f) / 201f * 0.04f
    scrollDp <= 900f -> 0.85f + (scrollDp - 502f) / 398f * 0.15f
    else -> 1f
}

private fun scrollFade(scrollDp: Float): Float = when {
    scrollDp <= AMBIENT_FADE_A -> 1f
    scrollDp <= AMBIENT_FADE_B -> 1f - (scrollDp - AMBIENT_FADE_A) /
        (AMBIENT_FADE_B - AMBIENT_FADE_A) * 0.25f
    scrollDp <= AMBIENT_FADE_C -> 0.75f - (scrollDp - AMBIENT_FADE_B) /
        (AMBIENT_FADE_C - AMBIENT_FADE_B) * 0.38f
    scrollDp <= AMBIENT_FADE_D -> 0.37f * (AMBIENT_FADE_D - scrollDp) /
        (AMBIENT_FADE_D - AMBIENT_FADE_C)
    else -> 0f
}

@Composable
fun DiscoverPage(
    continueItems: List<Pair<MediaItem, WatchStore.Progress>>,
    heroCandidates: List<TmdbHome.Item>,
    nowPlaying: List<TmdbHome.Item>,
    onTheAir: List<TmdbHome.Item>,
    trending: List<TmdbHome.Item>,
    variety: List<TmdbHome.Item>,
    todayAnime: List<TmdbHome.Item>,
    topPoster: (TmdbHome.Item) -> String?,
    logoFor: (TmdbHome.Item) -> String?,
    posterOf: @Composable (MediaItem) -> (@Composable () -> Unit)?,
    onOpenItem: (MediaItem) -> Unit,
    onOpenTmdbItem: (TmdbHome.Item) -> Unit,
    tmdbError: String? = null,
    onRetryTmdb: () -> Unit = {},
    onAmbientDark: (Boolean) -> Unit = {},
) {
    val listState = rememberLazyListState()
    val density = androidx.compose.ui.platform.LocalDensity.current

    var heroFieldColor by remember { mutableStateOf(DesignTokens.bgCream) }
    var heroFgLight by remember { mutableStateOf(true) }

    // chrome 组（节标题/圆点/导航）：暗海报且滚动未过翻转点 → 白，否则黑
    // （0138 S=0 白 / 0139 S=182 黑夹逼；亮海报恒黑——grid_01 狗狗圣诞实证）
    val chromeDark by remember {
        derivedStateOf {
            heroFgLight &&
                (listState.firstVisibleItemIndex == 0 &&
                    listState.firstVisibleItemScrollOffset < CHROME_FLIP_DP * density.density)
        }
    }

    val chromeFg = if (chromeDark) Color.White else DesignTokens.textMain
    val chromeSub = if (chromeDark) Color.White.copy(alpha = 0.72f) else DesignTokens.textSub
    val dotsFg = chromeFg

    // 环境遮罩/下缘场的当前色：底缘色随滚动向页底过渡（t(S)），三态夹逼的分段线性
    val scrimColor: State<Color> = remember {
        derivedStateOf {
            val sPx = if (listState.firstVisibleItemIndex == 0)
                listState.firstVisibleItemScrollOffset.toFloat() else 1e6f
            val t = fieldMix(sPx / density.density)
            Color(
                heroFieldColor.red + (DesignTokens.pageBackground.red - heroFieldColor.red) * t,
                heroFieldColor.green + (DesignTokens.pageBackground.green - heroFieldColor.green) * t,
                heroFieldColor.blue + (DesignTokens.pageBackground.blue - heroFieldColor.blue) * t)
        }
    }
    val scrimAlpha: State<Float> = remember {
        derivedStateOf {
            val sPx = if (listState.firstVisibleItemIndex == 0)
                listState.firstVisibleItemScrollOffset.toFloat() else 1e6f
            scrollFade(sPx / density.density)
        }
    }

    Box(Modifier.fillMaxSize()) {
        LazyColumn(
            state = listState,
            modifier = Modifier
                .fillMaxSize()
                .drawBehind {
                    // 下缘场：内容 [585, 1100dp] 平色（与海报外层遮罩同色同源，
                    // 在 585 处无缝相接），底部 29% α 渐隐入场底——无硬线
                    val sPx = if (listState.firstVisibleItemIndex == 0)
                        listState.firstVisibleItemScrollOffset.toFloat() else 1e6f
                    val sDp = sPx / density.density
                    val fade = scrollFade(sDp)
                    if (fade <= 0f) return@drawBehind
                    val fieldTop = HERO_HEIGHT_DP.dp.toPx() - sPx
                    val fieldH = (FIELD_END_DP - HERO_HEIGHT_DP).dp.toPx()
                    if (fieldTop < size.height && fieldH > 0f) {
                        val c = scrimColor.value
                        val flatEnd = 0.71f   // (950-585)/515
                        drawRect(
                            brush = Brush.verticalGradient(
                                0f to c, flatEnd to c, 1f to c.copy(alpha = 0f),
                                startY = fieldTop,
                                endY = fieldTop + fieldH),
                            topLeft = androidx.compose.ui.geometry.Offset(0f, fieldTop),
                            size = androidx.compose.ui.geometry.Size(size.width, fieldH),
                            alpha = fade,
                        )
                    }
                },
            contentPadding = PaddingValues(bottom = 100.dp),
        ) {
            // hero
            if (heroCandidates.isNotEmpty()) {
                item(key = "hero") {
                    HeroCarousel(heroCandidates, onAmbient = { fgLight, field ->
                        heroFgLight = fgLight
                        heroFieldColor = field
                    }, dotsColor = dotsFg,
                        scrimColor = scrimColor, scrimAlpha = scrimAlpha,
                        topPoster = topPoster, logoFor = logoFor)
                }
            }
            // 继续观看
            if (continueItems.isNotEmpty()) {
                item(key = "continue") {
                    Column {
                        Spacer(Modifier.height(13.dp))
                        SectionHeader("继续观看", onMore = null, color = chromeFg)
                        Spacer(Modifier.height(19.dp))
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
                    item(key = title) { PosterSection(title, list, chromeFg, onOpenTmdbItem) }
                }
            }
            if (trending.isNotEmpty()) {
                item(key = "本周趋势") { WideSection(trending, chromeFg, onOpenTmdbItem) }
            }
            if (variety.isNotEmpty()) {
                item(key = "近一月·热门综艺") { VarietySection(variety, chromeFg, onOpenTmdbItem) }
            }
            if (todayAnime.isNotEmpty()) {
                item(key = "今日动漫") { PosterSection("今日动漫", todayAnime, chromeFg, onOpenTmdbItem) }
            }
            // 错误态（home.error.network.* 五类原文 + 重试——M04-S04，不可合并）
            if (tmdbError != null && nowPlaying.isEmpty() && trending.isEmpty()) {
                item(key = "error") { TmdbErrorCard(tmdbError, onRetryTmdb) }
            }
        }
    }

    // 导航玻璃/状态栏随 chrome 组翻转（0138 暗→0139 亮）
    LaunchedEffect(chromeDark) { onAmbientDark(chromeDark) }
}

/** 本地条目来源名（E10「3 天前 · iLoveHD」格式；MediaStore 条目 = 本地）。 */
internal fun sourceOf(item: MediaItem): String = "本地"

/** 海报节：22pt 节标题 + 92×138 2:3 海报横滑（E9：标题 15 + 年份/类型 13 灰）。 */
@Composable
private fun PosterSection(
    title: String,
    list: List<TmdbHome.Item>,
    fg: Color,
    onOpenTmdbItem: (TmdbHome.Item) -> Unit,
) {
    Column {
        Spacer(Modifier.height(13.dp))
        SectionHeader(title, onMore = null, color = fg)
        Spacer(Modifier.height(16.dp))
        LazyRow(
            horizontalArrangement = Arrangement.spacedBy(10.dp),
            contentPadding = PaddingValues(horizontal = 13.dp),
        ) {
            items(list) { m ->
                TmdbPosterCard(m) { onOpenTmdbItem(m) }
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
        SectionHeader("本周趋势", onMore = null, color = fg)
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

/** TMDB 海报卡（92×138 2:3 圆角 10 + 标题 15 + 年份 13 灰，E9/236 既有规格）。 */
@Composable
internal fun TmdbPosterCard(
    m: TmdbHome.Item,
    onClick: () -> Unit,
) {
    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        modifier = Modifier
            .width(DesignTokens.posterWidth.dp)
            .clickable { onClick() },
    ) {
        TmdbImage(m.posterUrl, m.title,
            Modifier
                .size(DesignTokens.posterWidth.dp, (DesignTokens.posterWidth * 1.5f).dp)
                .clip(RoundedCornerShape(DesignTokens.radiusPoster.dp)))
        Spacer(Modifier.height(6.dp))
        Text(m.title, fontSize = DesignTokens.fontPosterTitle.sp,
            fontWeight = FontWeight.Bold, color = DesignTokens.textMain,
            maxLines = 1, overflow = TextOverflow.Ellipsis)
        Text(m.year(), fontSize = DesignTokens.fontPosterDate.sp,
            color = DesignTokens.textSub)
    }
}
