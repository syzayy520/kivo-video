package com.kivo.cinema.ui.design

import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.lazy.items as rowItems
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.foundation.pager.rememberPagerState
import androidx.compose.runtime.remember
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.setValue
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.MediaItem

/**
 * 四个页面（236 设计稿 §三）：发现 / 资源库 / 设置 / 我的。
 * 数值全部帧实测；内容图给色、chrome 退后。
 * 发现页数据 = 本地媒体库（TMDB 元数据未接——hero 的评分行 v1 留空位，明示）。
 */
private val pad = DesignTokens.screenPad.dp

/**
 * 发现首页（grid_01/grid_02/rex_browse_a 帧实测 + 用户指定 IA）：
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
    posterOf: @Composable (MediaItem) -> (@Composable () -> Unit)?,
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

/**
 * hero 轮播 v2（hero_8/grid_01 帧实测吃透版）：
 * ① 全出血 高=53%屏高（黑鸽帧 y0→427pt / 844pt 屏）
 * ② 标题 ~55sp 白/黑自适应叠印在 62% 高度处居中（黑鸽=白字、狗狗圣诞=黑字）
 * ③ 类型·日期 17pt / 简介 15pt 两行 / 评分徽章行——全部在图内、同自适应色
 * ④ 自适应 = backdrop 下半区平均亮度采样（<0.55 → 白字）
 * ⑤ 圆点在 hero 块外（活动点=前景色）
 * ⑥ 轮播 0.4s spring(0.5,0.85)，自动 5s，swipe 打断（M04-S01 CSV）
 */
@Composable
internal fun HeroCarousel(candidates: List<com.kivo.cinema.tmdb.TmdbHome.Item>) {
    val pagerState = androidx.compose.foundation.pager.rememberPagerState(
        pageCount = { candidates.size })
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
    val conf = androidx.compose.ui.platform.LocalConfiguration.current
    val heroH = (conf.screenHeightDp * 0.53f).dp
    // 自适应前景色状态（按当前页 backdrop 亮度）
    var fgLight by remember { mutableStateOf(true) }
    LaunchedEffect(pagerState.currentPage, candidates.size) {
        val m = candidates.getOrNull(pagerState.currentPage)
        if (m == null) return@LaunchedEffect
        val bmp = kotlinx.coroutines.withContext(kotlinx.coroutines.Dispatchers.IO) {
            com.kivo.cinema.ui.design.TmdbImageCache.get(m.backdropUrl)
                ?: com.kivo.cinema.tmdb.TmdbClient.staticBitmap(m.backdropUrl)
        }
        bmp?.let { fgLight = luminanceBelow(it) < 0.55f }
    }
    val fg = if (fgLight) Color.White else DesignTokens.textMain
    val fgSub = if (fgLight) Color.White.copy(alpha = 0.85f) else DesignTokens.textMain

    Column {
        androidx.compose.foundation.pager.HorizontalPager(
            state = pagerState,
            modifier = Modifier.fillMaxWidth(),
        ) { idx ->
            val m = candidates[idx]
            Box(
                Modifier
                    .fillMaxWidth()
                    .height(heroH)
            ) {
                TmdbImage(m.backdropUrl, m.title, Modifier.fillMaxSize())
                // 内容层：标题大字（62% 高度）→ 类型·日期 → 简介 → 评分行（全部图内自适应色）
                Column(
                    Modifier
                        .fillMaxSize()
                        .padding(horizontal = 16.dp),
                    horizontalAlignment = Alignment.CenterHorizontally,
                ) {
                    Spacer(Modifier.weight(0.62f))
                    Text(m.title, fontSize = 52.sp, fontWeight = FontWeight.Bold,
                        color = fg, maxLines = 1, overflow = TextOverflow.Ellipsis,
                        style = androidx.compose.ui.text.TextStyle(
                            shadow = androidx.compose.ui.graphics.Shadow(
                                color = Color.Black.copy(alpha = 0.35f),
                                blurRadius = 12f)),
                        textAlign = androidx.compose.ui.text.style.TextAlign.Center,
                        modifier = Modifier.fillMaxWidth())
                    Spacer(Modifier.height(10.dp))
                    Text(m.date, fontSize = DesignTokens.fontHeroMeta.sp,
                        color = fgSub, textAlign = androidx.compose.ui.text.style.TextAlign.Center,
                        modifier = Modifier.fillMaxWidth())
                    Spacer(Modifier.height(6.dp))
                    if (m.overview.isNotBlank()) {
                        Text(m.overview, fontSize = DesignTokens.fontHeroSynopsis.sp,
                            color = fgSub, maxLines = 2,
                            overflow = TextOverflow.Ellipsis, lineHeight = 21.sp,
                            textAlign = androidx.compose.ui.text.style.TextAlign.Center,
                            modifier = Modifier.fillMaxWidth())
                        Spacer(Modifier.height(6.dp))
                    }
                    RatingBadges(listOf("TMDB" to m.rating),
                        modifier = Modifier.fillMaxWidth(), alignCenter = true,
                        adaptiveLight = fgLight)
                }
            }
        }
        Spacer(Modifier.height(10.dp))
        // 圆点（hero 块外；色随前景）
        Row(horizontalArrangement = Arrangement.Center,
            modifier = Modifier.fillMaxWidth()) {
            candidates.forEachIndexed { i, _ ->
                Box(
                    Modifier
                        .padding(horizontal = 4.dp)
                        .size(if (i == pagerState.currentPage) 9.dp else 6.dp)
                        .clip(CircleShape)
                        .background(if (i == pagerState.currentPage) fg
                            else if (fgLight) Color(0xFFB9B9BE) else Color(0xFF5A5A5E))
                )
            }
        }
    }
}

/** backdrop 下半区平均亮度（0-1）——文字黑白自适应依据。 */
internal fun luminanceBelow(bmp: android.graphics.Bitmap): Float {
    val y0 = bmp.height / 2
    var sum = 0f; var n = 0
    var y = y0
    while (y < bmp.height) {
        var x = 0
        while (x < bmp.width) {
            val c = bmp.getPixel(x, y)
            val l = (0.299f * (c shr 16 and 0xFF) +
                     0.587f * (c shr 8 and 0xFF) +
                     0.114f * (c and 0xFF)) / 255f
            sum += l; n++
            x += 24
        }
        y += 24
    }
    return if (n == 0) 0.5f else sum / n
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

/** 资源库页（云服务矩阵语义 + 我们的源集合；玻璃卡矩阵）。 */
@Composable
fun SourcesPage(
    onOpenSmb: () -> Unit,
    onOpenEmby: () -> Unit,
    onOpenLibrary: () -> Unit,
    onOpenPrivate: () -> Unit,
    videoCount: Int,
) {
    Column(Modifier.fillMaxSize().padding(horizontal = pad)) {
        Spacer(Modifier.height(8.dp))
        Text("资源库", fontSize = 28.sp, fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain)
        Spacer(Modifier.height(4.dp))
        Text("本地 ${videoCount} 个视频 · 云端与网络源", fontSize = 12.sp,
            color = DesignTokens.textSub)
        Spacer(Modifier.height(14.dp))
        SourceCard("本地影片", "MediaStore 扫描 · 海报墙 · 点播", "📁") { onOpenLibrary() }
        Spacer(Modifier.height(DesignTokens.gap.dp))
        SourceCard("SMB / 网络共享", "libsmb2 直连 · 硬解过本地代理 · 断点下载", "🗄") { onOpenSmb() }
        Spacer(Modifier.height(DesignTokens.gap.dp))
        SourceCard("Emby / Jellyfin", "AuthenticateByName 登录 · 直连流播放", "☁") { onOpenEmby() }
        Spacer(Modifier.height(DesignTokens.gap.dp))
        SourceCard("私密区", "生物特征门 · 受保护观看空间", "🔒") { onOpenPrivate() }
    }
}

@Composable
private fun SourceCard(title: String, desc: String, icon: String, onClick: () -> Unit) {
    GlassSurface(Modifier.fillMaxWidth().clickable { onClick() }) {
        Row(Modifier.fillMaxWidth().padding(16.dp), verticalAlignment = Alignment.CenterVertically) {
            Box(
                Modifier
                    .size(44.dp)
                    .clip(CircleShape)
                    .background(Color(0xFFEFE9F2)),
                contentAlignment = Alignment.Center
            ) { Text(icon, fontSize = 20.sp) }
            Spacer(Modifier.width(12.dp))
            Column {
                Text(title, fontSize = DesignTokens.fontRowTitle.sp, fontWeight = FontWeight.Bold,
                    color = DesignTokens.textMain)
                Text(desc, fontSize = 11.sp, color = DesignTokens.textSub)
            }
            Spacer(Modifier.weight(1f))
            Text("›", fontSize = 22.sp, color = DesignTokens.textSub)
        }
    }
}

/** 设置页（set_a 帧：inset 分组卡 59pt 行 + 节头 + 多行脚注"敢解释"）。 */
@Composable
fun SettingsPage(clearTextOk: Boolean, buildLabel: String) {
    Column(Modifier.fillMaxSize().padding(horizontal = pad)) {
        Spacer(Modifier.height(8.dp))
        Text("设置", fontSize = 28.sp, fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain)
        Spacer(Modifier.height(14.dp))
        GroupSection("播放") {
            GroupRow("网络明文传输", if (clearTextOk) "已开启" else "已关闭")
            GroupRow("解码", "硬解优先 · 失败报 reason")
        }
        Spacer(Modifier.height(8.dp))
        Text("本卡为设置页骨架：播放/字幕/解码各组随功能卡逐项接入（236 §六）。",
            fontSize = DesignTokens.fontFootnote.sp, color = DesignTokens.textSub,
            lineHeight = 15.sp)
        Spacer(Modifier.height(14.dp))
        GroupSection("关于") {
            GroupRow("构建", buildLabel)
            GroupRow("UI 对齐", "设计稿 1.1 录屏规格（236 卡）")
        }
        Spacer(Modifier.height(8.dp))
        Text("kivo video 的界面规格来自 设计稿 真机录屏逐帧拆解；所有数值为实测。",
            fontSize = DesignTokens.fontFootnote.sp, color = DesignTokens.textSub,
            lineHeight = 15.sp)
    }
}

@Composable
private fun GroupSection(header: String, content: @Composable () -> Unit) {
    Text(header, fontSize = 13.sp, color = DesignTokens.textSub,
        modifier = Modifier.padding(start = 4.dp))
    Spacer(Modifier.height(6.dp))
    GlassSurface(Modifier.fillMaxWidth()) { Column { content() } }
}

@Composable
private fun GroupRow(title: String, value: String) {
    Row(
        Modifier
            .fillMaxWidth()
            .height(DesignTokens.rowHeight.dp)
            .padding(horizontal = 16.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(title, fontSize = DesignTokens.fontRowTitle.sp, color = DesignTokens.textMain)
        Spacer(Modifier.weight(1f))
        Text(value, fontSize = 15.sp, color = DesignTokens.textSub)
    }
}

/** 我的页（有观看统计/Pro——数据未建，v1 极简档案位，明示占位）。 */
@Composable
fun ProfilePage(videoCount: Int) {
    Column(Modifier.fillMaxSize().padding(horizontal = pad)) {
        Spacer(Modifier.height(8.dp))
        Text("我的", fontSize = 28.sp, fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain)
        Spacer(Modifier.height(14.dp))
        GlassSurface(Modifier.fillMaxWidth()) {
            Row(Modifier.padding(16.dp), verticalAlignment = Alignment.CenterVertically) {
                Box(
                    Modifier.size(56.dp).clip(CircleShape).background(Color(0xFFEFE9F2)),
                    contentAlignment = Alignment.Center
                ) { Text("👤", fontSize = 26.sp) }
                Spacer(Modifier.width(12.dp))
                Column {
                    Text("kivo 观众", fontSize = DesignTokens.fontRowTitle.sp,
                        fontWeight = FontWeight.Bold, color = DesignTokens.textMain)
                    Text("媒体库 ${videoCount} 个视频 · 统计随功能接入",
                        fontSize = 11.sp, color = DesignTokens.textSub)
                }
            }
        }
    }
}

internal fun fmtSeen(positionMs: Long): String {
    val s = positionMs / 1000
    return if (s >= 3600) "%d:%02d:%02d".format(s / 3600, s / 60 % 60, s % 60)
    else "%d:%02d".format(s / 60, s % 60)
}

internal fun formatDate(addedSec: Long): String {
    val d = java.util.Calendar.getInstance()
    d.timeInMillis = addedSec * 1000L
    return "%04d-%02d-%02d".format(
        d.get(java.util.Calendar.YEAR), d.get(java.util.Calendar.MONTH) + 1,
        d.get(java.util.Calendar.DAY_OF_MONTH))
}
