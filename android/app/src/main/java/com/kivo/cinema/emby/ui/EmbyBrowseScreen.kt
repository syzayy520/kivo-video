package com.kivo.cinema.emby.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.emby.EmbyCatalog
import com.kivo.cinema.emby.EmbyHeroItem
import com.kivo.cinema.emby.EmbyItem
import com.kivo.cinema.emby.EmbySessionModel
import com.kivo.cinema.tmdb.trailer.TrailerResult
import com.kivo.cinema.ui.design.HeroCarousel
import com.kivo.cinema.ui.design.home.HeroCard
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.withContext

/**
 * 317 · **服务器首页**（对齐 Rex：大海报 hero + 继续观看 + 分类横排）。
 *
 * ## 与改造前的差别
 * 改造前是一列「分类名 + 横排海报」，进来第一眼看到的是"18 个媒体库"。
 * 现在顶部是一张铺屏大海报（可左右滑），下面是「继续观看」，再下面才是分类横排 ——
 * 信息密度没丢（分类行全保留），但第一屏有"这是什么剧、好不好看"的判断依据。
 *
 * ## 为什么 hero 要浮起返回栏
 * Rex 的返回键是**压在图上**的半透明圆钮：海报要顶到状态栏才够"大"。
 * 用 `Box` 叠层（LazyColumn 在下、返回栏在上）而不是 `Column` 串行 ——
 * 串行会把海报往下挤一截，那就不叫大海报了。
 *
 * ## 数据
 * hero 候选与「继续观看」都来自 [EmbySessionModel]：前者是"最近入库"（跨库），
 * 后者是 Emby 原生 `Items/Resume`（自带位置）。两者都是**尽力而为**：拉不到就少一块，
 * 不影响下面的分类行（服务器慢不该让整页白屏）。
 */
@Composable
fun EmbyBrowseScreen(
    model: EmbySessionModel, onBack: () -> Unit,
    onOpenView: (EmbyCatalog.View) -> Unit, onItem: (EmbyItem) -> Unit,
    /** 317：当前服务器源 id（列表中高亮用）。 */
    currentSourceId: String? = null,
    /** 317：可切换的服务器（id to 名称），来自资源库的源列表。 */
    sources: List<Pair<String, String>> = emptyList(),
    /** 317：切服（宿主重启本页并带新 sourceId，不用退回资源库）。 */
    onSwitchSource: (String) -> Unit = {},
) {
    EmbyBrowseBody(model, onBack, onOpenView, onItem, currentSourceId, sources, onSwitchSource)
}

@Composable
private fun EmbyBrowseBody(
    model: EmbySessionModel, onBack: () -> Unit,
    onOpenView: (EmbyCatalog.View) -> Unit, onItem: (EmbyItem) -> Unit,
    currentSourceId: String?,
    sources: List<Pair<String, String>>,
    onSwitchSource: (String) -> Unit,
) {
    val context = androidx.compose.ui.platform.LocalContext.current
    var heroes by remember(model) { mutableStateOf<List<EmbyHeroItem>?>(null) }
    var resumed by remember(model) { mutableStateOf<List<EmbyHeroItem>>(emptyList()) }
    LaunchedEffect(model) {
        // 335：hero 与「继续观看」是两个独立网络请求，原来串行（实测间隔 ~3.7s）
        // ⇒ 首屏被第二个请求拖住。并行发起；hero（首屏主角）一拿到就先显示，
        // 「继续观看」（下方横排）随后补上，都不用等对方。
        val h = async(Dispatchers.IO) { model.heroItems(context) }
        val r = async(Dispatchers.IO) { model.resumeItems() }
        heroes = h.await()
        resumed = r.await()
    }
    // 317 · **完全照抄首页（DiscoverPage）的吸色写法**。
    //
    // 之前我用自己的理解做了两版（固定暗色 / 整页刷成海报色），都错：
    // · 固定暗色 ⇒ 每部剧背景一样，海报像"贴上去"的；
    // · 整页海报色 ⇒ 下方内容区也被染色，与浅色卡片割裂。
    //
    // 首页的正确模型是三层：
    //   heroFieldColor  海报提取的主色（HeroPage 的 onAmbient 回传）
    //   scrimColor      = HomeAtmosphere.background(海报色, 滚动距离) —— **随滚动从海报色渐变回页面底色**
    //   文字色          = HomeAtmosphere.lightForeground(scrimColor) 判深浅
    // 这样页面仍是浅色，只有 hero 及其接下来的一段是海报色，往下自然回到 pageBackground。
    var heroFieldColor by remember { mutableStateOf(com.kivo.cinema.ui.design.DesignTokens.pageBackground) }
    val density = androidx.compose.ui.platform.LocalDensity.current
    val listState = rememberLazyListState()
    // 317 · **完全复用首页的吸色模型**（HomeAtmosphere）。
    //
    // 走过一次弯路：我把它改成"取海报色压暗、整页固定深色"，结果是整页一块死色、
    // 滚动也不变（用户当场指出）。首页那套才是对的：
    //   softened = ambient 向 pageBackground 混 28%（柔化，不带海报的深阴影）
    //   再按滚动距离 lerp 回 pageBackground（滚下去就回到页面底色）
    // ⇒ 顶部是海报氛围色、往下自然过渡回浅色，滚动时连续变化。
    val scrimColor: State<Color> = remember {
        derivedStateOf {
            val scrollDp = if (listState.firstVisibleItemIndex == 0)
                listState.firstVisibleItemScrollOffset / density.density else 390f
            com.kivo.cinema.ui.design.home.HomeAtmosphere.background(heroFieldColor, scrollDp, wash = .10f)
        }
    }
    val chromeDark = com.kivo.cinema.ui.design.home.HomeAtmosphere.lightForeground(scrimColor.value)
    val chromeFg = if (chromeDark) Color.White else com.kivo.cinema.ui.design.DesignTokens.textMain
    val chromeSub = if (chromeDark) Color.White.copy(alpha = .72f)
        else com.kivo.cinema.ui.design.DesignTokens.textSub
    // 语义色跟着 chrome 走（卡片占位、加载态、副标题都用它）
    val scheme = androidx.compose.material3.MaterialTheme.colorScheme.copy(
        onBackground = chromeFg,
        onSurface = chromeFg,
        onSurfaceVariant = chromeSub,
        background = scrimColor.value,
    )
    val cards = remember(heroes) {
        heroes.orEmpty().map { h ->
            HeroCard(
                id = h.id,
                title = h.name,
                heroUrl = h.heroUrl,
                metaLine = h.metaLine(),
                overview = h.overview,
                // TMDB 优先（详情页同口径）；没匹配到才回退 Emby 的 CommunityRating
                rating = h.tmdbRating ?: h.rating,
                ratingSource = if (h.tmdbRating != null) "TMDB" else "Emby",
                logoUrl = null,          // Emby 没有片名艺术字 ⇒ HeroPage 回落文字标题
                isSeries = h.isSeries,
            )
        }
    }

    androidx.compose.material3.MaterialTheme(colorScheme = scheme) {
    Box(Modifier.fillMaxSize().drawBehind { drawRect(scrimColor.value) }) {
        LazyColumn(
            state = listState,
            modifier = Modifier.fillMaxSize(),
            contentPadding = PaddingValues(bottom = 32.dp),
        ) {
            item(key = "hero") {
                if (cards.isEmpty()) {
                    // 数据没到 / 服务器没图：给一块与 hero 等高的空位，避免下方内容上跳。
                    Box(Modifier.fillMaxWidth().height(240.dp))
                } else {
                    HeroCarousel(
                        cards = cards,
                        onAmbient = { heroFieldColor = it },
                        dotsColor = Color.White,
                        scrimColor = scrimColor,
                        pullDp = 0f,
                        scrollPx = { -listState.firstVisibleItemScrollOffset.toFloat() },
                        previewVisible = false,
                        previewEnabled = false,      // Emby 侧不做预告片
                        generation = 0,
                        trailerFor = { TrailerResult() },
                        proxyAddress = null,
                        trailerItemFor = { null },   // 无 TMDB 依赖
                        logoLoader = null,
                        // 点海报进详情：走既有 onItem，与海报墙同一条路径
                        onClick = { card ->
                            heroes.orEmpty().firstOrNull { it.id == card.id }?.let {
                                onItem(EmbyItem(it.id, it.name, it.year, it.heroUrl, it.isSeries))
                            }
                        },
                    )
                }
            }
            if (resumed.isNotEmpty()) {
                item(key = "resume") {
                    EmbyResumeRow(
                        items = resumed,
                        onOpen = { h -> onItem(EmbyItem(h.id, h.name, h.year, h.heroUrl, h.isSeries)) },
                    )
                }
            }
            if (!model.viewsLoaded) {
                item(key = "views-loading") {
                    Box(Modifier.fillMaxWidth().height(160.dp), contentAlignment = Alignment.Center) {
                        CircularProgressIndicator(Modifier.size(26.dp), strokeWidth = 2.dp)
                    }
                }
            } else if (model.views.isEmpty()) {
                item(key = "views-empty") {
                    Text(
                        "服务器没有返回媒体库分类，请检查账号权限。",
                        fontSize = 14.sp, color = scheme.onSurfaceVariant,
                        modifier = Modifier.padding(24.dp),
                    )
                }
            } else {
                items(model.views, key = { it.id }) { view ->
                    EmbyCategoryRow(model, view, onOpenView, onItem)
                }
            }
        }

        // 悬浮顶部栏（返回 + 服务器名 + 切服），压在 hero 上不占布局高度
        EmbyBrowseTopBar(
            serverName = model.serverName,
            currentSourceId = currentSourceId,
            sources = sources,
            onSwitchSource = onSwitchSource,
            onBack = onBack,
        )
    }
    }
}
