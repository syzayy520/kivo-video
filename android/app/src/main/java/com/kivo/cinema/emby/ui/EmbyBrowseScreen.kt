package com.kivo.cinema.emby.ui

import androidx.compose.foundation.background
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
) {
    EmbyBrowseBody(model, onBack, onOpenView, onItem)
}

@Composable
private fun EmbyBrowseBody(
    model: EmbySessionModel, onBack: () -> Unit,
    onOpenView: (EmbyCatalog.View) -> Unit, onItem: (EmbyItem) -> Unit,
) {
    var heroes by remember(model) { mutableStateOf<List<EmbyHeroItem>?>(null) }
    var resumed by remember(model) { mutableStateOf<List<EmbyHeroItem>>(emptyList()) }
    LaunchedEffect(model) {
        heroes = withContext(Dispatchers.IO) { model.heroItems() }
        resumed = withContext(Dispatchers.IO) { model.resumeItems() }
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
                rating = h.rating,
                ratingSource = "Emby",
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
                    ResumeRow(
                        model = model,
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
                    ViewRow(model, view, onOpenView, onItem)
                }
            }
        }

        // 悬浮返回栏：压在 hero 上（不占布局高度，海报才能顶到状态栏）
        Row(
            Modifier.fillMaxWidth().statusBarsPadding().padding(horizontal = 12.dp, vertical = 6.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Surface(shape = RoundedCornerShape(50), color = Color.Black.copy(alpha = .34f)) {
                TextButton(onClick = onBack) { Text("‹  返回", fontSize = 15.sp, color = Color.White) }
            }
            // 317 · 服务器名放中间：放在「继续观看」下方会让人以为那是内容分区的标题
            // （用户反馈"很奇怪"）；这里是导航层，位置语义正确。
            Text(
                model.serverName, fontSize = 13.sp, color = Color.White.copy(alpha = .82f),
                maxLines = 1, overflow = androidx.compose.ui.text.style.TextOverflow.Ellipsis,
                modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
                textAlign = androidx.compose.ui.text.style.TextAlign.Center,
            )
            Surface(shape = RoundedCornerShape(50), color = Color.Black.copy(alpha = .34f)) {
                TextButton(onClick = onBack) { Text("切换服务器", fontSize = 13.sp, color = Color.White) }
            }
        }
    }
    }
}

/** 「继续观看」横排：缩略图 + 片名 + 看到哪里。 */
@Composable
private fun ResumeRow(
    model: EmbySessionModel,
    items: List<EmbyHeroItem>,
    onOpen: (EmbyHeroItem) -> Unit,
) {
    Column(Modifier.fillMaxWidth().padding(top = 16.dp)) {
        Text(
            "继续观看", fontSize = 18.sp, fontWeight = FontWeight.Bold,
            modifier = Modifier.padding(horizontal = 24.dp, vertical = 6.dp),
        )
        LazyRow(
            contentPadding = PaddingValues(horizontal = 24.dp),
            horizontalArrangement = Arrangement.spacedBy(14.dp),
        ) {
            items(items, key = { it.id }) { h ->
                Column(Modifier.width(208.dp)) {
                    Box(
                        Modifier.fillMaxWidth().aspectRatio(16f / 9f)
                            .clip(RoundedCornerShape(10.dp))
                            .background(MaterialTheme.colorScheme.primary.copy(alpha = .06f)),
                    ) {
                        EmbyHeroThumb(model, h.heroUrl, h.name)
                        // 进度条压在缩略图底部（Rex 的样式）
                        val frac = if (h.runtimeMs > 0) (h.resumeMs.toFloat() / h.runtimeMs).coerceIn(0f, 1f) else 0f
                        if (frac > 0f) {
                            Box(
                                Modifier.align(Alignment.BottomStart).fillMaxWidth(frac)
                                    .height(3.dp).background(MaterialTheme.colorScheme.primary),
                            )
                        }
                    }
                    Text(
                        h.name, fontSize = 13.sp, maxLines = 1,
                        modifier = Modifier.padding(top = 7.dp),
                    )
                    Text(
                        "看到 " + com.kivo.cinema.ui.compose.player.formatPlayerTime(h.resumeMs),
                        fontSize = 11.sp, color = MaterialTheme.colorScheme.onSurfaceVariant,
                        modifier = Modifier.padding(top = 2.dp),
                    )
                }
            }
        }
    }
}

@Composable
private fun ViewRow(
    model: EmbySessionModel, view: EmbyCatalog.View,
    onOpenView: (EmbyCatalog.View) -> Unit, onItem: (EmbyItem) -> Unit,
) {
    var items by remember(view.id) { mutableStateOf<List<EmbyItem>?>(null) }
    LaunchedEffect(view.id) { items = withContext(Dispatchers.IO) { model.rowPage(view) } }
    // 317 · **空分类整行不渲染**（连标题一起）。
    // 之前只把内容换成 Unit，标题「播放列表」还立在那里 ⇒ 一个空标题挂在页面上，
    // 用户一眼就看出不对。加载中（null）仍保留标题 + 进度圈，避免闪一下又回来。
    if (items?.isEmpty() == true) return
    Column(Modifier.fillMaxWidth().padding(vertical = 12.dp)) {
        Row(Modifier.fillMaxWidth().padding(horizontal = 24.dp), verticalAlignment = Alignment.CenterVertically) {
            Text(view.name, fontSize = 20.sp, fontWeight = FontWeight.Bold, modifier = Modifier.weight(1f))
            TextButton(onClick = { onOpenView(view) }) { Text("›", fontSize = 20.sp) }
        }
        Spacer(Modifier.height(10.dp))
        val list = items
        when {
            list == null -> Box(Modifier.fillMaxWidth().height(210.dp), contentAlignment = Alignment.Center) {
                CircularProgressIndicator(Modifier.size(22.dp), strokeWidth = 2.dp)
            }
            else -> LazyRow(contentPadding = PaddingValues(horizontal = 24.dp),
                horizontalArrangement = Arrangement.spacedBy(16.dp)) {
                items(list, key = { it.id }) { item ->
                    Box(Modifier.width(128.dp)) { EmbyPosterCard(model, item) { onItem(item) } }
                }
            }
        }
    }
}

/** 横排缩略图（Emby 图要 token ⇒ 走 model 的带鉴权加载器，不直接用通用 loader）。 */
@Composable
private fun EmbyHeroThumb(model: EmbySessionModel, url: String, name: String) {
    var bmp by remember(url) { mutableStateOf<android.graphics.Bitmap?>(null) }
    LaunchedEffect(url) { bmp = withContext(Dispatchers.IO) { model.image(url) } }
    val frame = bmp
    if (frame != null) {
        androidx.compose.foundation.Image(
            bitmap = frame.asImageBitmap(),
            contentDescription = name,
            contentScale = ContentScale.Crop,
            modifier = Modifier.fillMaxSize(),
        )
    }
}
