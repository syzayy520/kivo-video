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
import androidx.compose.animation.animateColorAsState
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
import androidx.compose.ui.draw.drawBehind
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
    onAmbient: (Color) -> Unit = {},
    tmdbError: String? = null,
    onRetryTmdb: () -> Unit = {},
) {
    val pad = DesignTokens.screenPad.dp
    // 第一屏背景自适应（黑鸽页全暗/狗狗页全米——帧实证；固定渐变仅其余 tab 使用）
    val ambient = remember { mutableStateOf(DesignTokens.bgCream) }
    // 环境色平滑过渡（animateColorAsState 在本 compose 版本不可用——手写插值）
    val ambientAnim = ambient.value
    val confDp = androidx.compose.ui.platform.LocalConfiguration.current.screenHeightDp
    val gradHeightPx = confDp * 1.5f * (androidx.compose.ui.platform.LocalDensity.current.density)
    LazyVerticalGrid(
        columns = GridCells.Fixed(1),
        modifier = Modifier
            .fillMaxSize()
            .drawBehind {
                // 内容坐标系：渐变占据前 1.5 屏，其后纯 pageBackground（无缝）
                drawRect(DesignTokens.pageBackground)
                drawRect(
                    brush = androidx.compose.ui.graphics.Brush.verticalGradient(
                        0f to ambient.value,
                        0.45f to ambient.value,
                        0.72f to DesignTokens.washFill,
                        1f to DesignTokens.pageBackground),
                    topLeft = androidx.compose.ui.geometry.Offset(0f, 0f),
                    size = androidx.compose.ui.geometry.Size(size.width, gradHeightPx))
            },
        contentPadding = androidx.compose.foundation.layout.PaddingValues(bottom = 10.dp),
    ) {
        // hero（TMDB）——环境色只覆盖首屏高度，下滑马上出正常渐变（用户实拍行为）
        if (heroCandidates.isNotEmpty()) {
            item(span = { androidx.compose.foundation.lazy.grid.GridItemSpan(maxLineSpan) }) {
                // 1.5 屏高渐变块：环境色保持 → washFill 洗色 → 渐白（无硬边，随内容滚动逐渐变白）
                val screenH = androidx.compose.ui.platform.LocalConfiguration.current.screenHeightDp.dp
                Box(
                    Modifier
                        .fillMaxWidth()
                        .height(screenH * 1.5f)
                        .background(androidx.compose.ui.graphics.Brush.verticalGradient(
                            0f to ambientAnim,
                            0.55f to ambientAnim,
                            0.8f to DesignTokens.washFill,
                            1f to DesignTokens.pageBackground))
                ) {
                    HeroCarousel(heroCandidates, onAmbient = { ambient.value = it })
                }
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
        listOf(
            "正在热映" to nowPlaying,
            "正在播出" to onTheAir,
            "本周趋势" to trending,
        ).forEach { (title, list) ->
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
        // 错误态（home.error.network.* 五类原文 + 重试——拆解 M04-S04）
        if (nowPlaying.isEmpty() && onTheAir.isEmpty() && trending.isEmpty() && tmdbError != null) {
            item(span = { androidx.compose.foundation.lazy.grid.GridItemSpan(maxLineSpan) }) {
                val kindText = when (tmdbError) {
                    "timeout" -> "连接超时，请稍后重试"
                    "offline" -> "网络未连接，请检查网络后重试"
                    "secure" -> "无法建立安全连接，请检查服务器设置"
                    "server" -> "无法连接服务器，请稍后重试"
                    else -> "网络连接已中断，请重试"
                }
                GlassSurface(Modifier.fillMaxWidth().padding(horizontal = DesignTokens.screenPad.dp)) {
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
                                .clip(androidx.compose.foundation.shape.RoundedCornerShape(8.dp))
                                .clickable { onRetryTmdb() }
                                .padding(horizontal = 16.dp, vertical = 6.dp))
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
internal fun HeroCarousel(candidates: List<com.kivo.cinema.tmdb.TmdbHome.Item>,
                          onAmbient: (Color) -> Unit = {}) {
    // 手动轮播（用户实拍证伪自动滚动）
    val pagerState = androidx.compose.foundation.pager.rememberPagerState(
        pageCount = { candidates.size })
    var fgLight by remember { mutableStateOf(true) }
    LaunchedEffect(pagerState.currentPage, candidates.size) {
        val m = candidates.getOrNull(pagerState.currentPage)
        if (m == null) return@LaunchedEffect
        val bmp = kotlinx.coroutines.withContext(kotlinx.coroutines.Dispatchers.IO) {
            com.kivo.cinema.ui.design.TmdbImageCache.get(m.posterUrl)
                ?: com.kivo.cinema.tmdb.TmdbClient.staticBitmap(m.posterUrl)
        }
        bmp?.let {
            fgLight = luminanceBelow(it) < 0.55f
            onAmbient(ambientBelow(it))
        }
    }
    val fg = if (fgLight) Color.White else DesignTokens.textMain

    androidx.compose.foundation.pager.HorizontalPager(
        state = pagerState,
        modifier = Modifier.fillMaxWidth(),
    ) { idx ->
        val m = candidates[idx]
        // hero v5（IMG_0082 帧机制）：海报 2:3 铺满全宽，下半渐暗，内容叠印在海报下部
        Box(
            Modifier
                .fillMaxWidth()
                .height(585.dp)   // 393pt 设计基准 × 1.5（2:3 海报整张，帧测钉死）
        ) {
            TmdbImage(m.posterUrl, m.title, Modifier.fillMaxSize())
            // 渐暗遮罩：55% 处开始向底部压暗（文字可读性——IMG_0082 实证）
            Box(
                Modifier
                    .fillMaxSize()
                    .background(androidx.compose.ui.graphics.Brush.verticalGradient(
                        0.5f to Color.Transparent,
                        1f to Color.Black.copy(alpha = 0.62f))))
            Column(
                Modifier
                    .fillMaxSize()
                    .padding(horizontal = 16.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
            ) {
                Spacer(Modifier.weight(0.62f))
                Text(m.date, fontSize = DesignTokens.fontHeroMeta.sp,
                    color = Color.White, textAlign = androidx.compose.ui.text.style.TextAlign.Center,
                    modifier = Modifier.fillMaxWidth())
                Spacer(Modifier.height(6.dp))
                if (m.overview.isNotBlank()) {
                    Text(m.overview, fontSize = DesignTokens.fontHeroSynopsis.sp,
                        color = Color.White.copy(alpha = 0.92f), maxLines = 2,
                        overflow = TextOverflow.Ellipsis, lineHeight = 21.sp,
                        textAlign = androidx.compose.ui.text.style.TextAlign.Center,
                        modifier = Modifier.fillMaxWidth())
                    Spacer(Modifier.height(6.dp))
                }
                RatingBadges(listOf("TMDB" to m.rating),
                    modifier = Modifier.fillMaxWidth(), alignCenter = true,
                    adaptiveLight = true)
                Spacer(Modifier.height(10.dp))
                // 圆点叠印海报底部（IMG_0082：活动点白）
                Row(horizontalArrangement = Arrangement.Center,
                    modifier = Modifier.fillMaxWidth()) {
                    candidates.forEachIndexed { i, _ ->
                        Box(
                            Modifier
                                .padding(horizontal = 4.dp)
                                .size(if (i == pagerState.currentPage) 9.dp else 6.dp)
                                .clip(CircleShape)
                                .background(if (i == pagerState.currentPage) Color.White
                                    else Color.White.copy(alpha = 0.35f))
                        )
                    }
                }
            }
        }
    }
}

/** backdrop 底部 1/3 平均色——第一屏背景自适应源（内容即色彩）。 */
internal fun ambientBelow(bmp: android.graphics.Bitmap): Color {
    val y0 = (bmp.height * 2) / 3
    var r = 0L; var g = 0L; var b = 0L; var n = 0L
    var y = y0
    while (y < bmp.height) {
        var x = 0
        while (x < bmp.width) {
            val c = bmp.getPixel(x, y)
            r += (c shr 16 and 0xFF); g += (c shr 8 and 0xFF); b += (c and 0xFF); n++
            x += 16
        }
        y += 16
    }
    if (n == 0L) return DesignTokens.bgCream
    return Color((r / n).toInt(), (g / n).toInt(), (b / n).toInt())
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
