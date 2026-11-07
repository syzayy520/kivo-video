package com.kivo.cinema.ui.rex

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
private val pad = RexTokens.screenPad.dp

/** 发现首页（grid_02 帧）：hero（首个条目全出血）+ 最近添加横滑 + 全部网格节。 */
@Composable
fun DiscoverPage(
    items: List<MediaItem>,
    posterOf: @Composable (MediaItem) -> (@Composable () -> Unit)?,
    onOpenItem: (MediaItem) -> Unit,
    onOpenLibrary: () -> Unit,
) {
    Column(Modifier.fillMaxSize()) {
        if (items.isEmpty()) {
            Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                Text("还没有视频——先在资源库添加片源或下载",
                    fontSize = 14.sp, color = RexTokens.textSub)
            }
            return
        }
        val hero = items.first()
        Column(
            Modifier
                .fillMaxWidth()
                .clickable { onOpenItem(hero) }
        ) {
            // hero 全出血（16:9~ 高度按屏宽裁，遮罩下压）
            Box(
                Modifier
                    .fillMaxWidth()
                    .aspectRatio(16f / 10f)
                    .clip(RoundedCornerShape(bottomStart = RexTokens.radiusCard.dp,
                        bottomEnd = RexTokens.radiusCard.dp))
                    .background(Color(0xFF24242A))
            ) {
                posterOf(hero)?.invoke()
                Box(
                    Modifier
                        .fillMaxSize()
                        .background(
                            androidx.compose.ui.graphics.Brush.verticalGradient(
                                listOf(Color.Transparent, Color.Black.copy(alpha = 0.55f))))
                )
                Text(hero.title, fontSize = 24.sp, fontWeight = FontWeight.Bold,
                    color = Color.White, maxLines = 1, overflow = TextOverflow.Ellipsis,
                    modifier = Modifier
                        .align(Alignment.BottomStart)
                        .padding(pad))
            }
            Spacer(Modifier.height(10.dp))
            // 类型·日期 行（17pt 居中——Rex hero 语义；本地数据用 添加日期 顶位，明示）
            Text("${hero.width}×${hero.height} · ${formatDate(hero.addedSec)}",
                fontSize = RexTokens.fontHeroMeta.sp, color = RexTokens.textMain,
                modifier = Modifier.fillMaxWidth(),
                textAlign = androidx.compose.ui.text.style.TextAlign.Center)
            Spacer(Modifier.height(6.dp))
            // 圆点指示器（活动点黑且大——v1 静态 1/1）
            Row(horizontalArrangement = Arrangement.Center,
                modifier = Modifier.fillMaxWidth()) {
                Box(
                    Modifier
                        .size(9.dp)
                        .clip(CircleShape)
                        .background(RexTokens.textMain)
                )
            }
        }
        Spacer(Modifier.height(14.dp))
        // 最近添加（继续观看位的 v1 形态：无进度数据——进度条 0，明示）
        SectionHeader("最近添加", onMore = onOpenLibrary)
        Spacer(Modifier.height(10.dp))
        LazyRow(horizontalArrangement = Arrangement.spacedBy(pad),
            contentPadding = androidx.compose.foundation.layout.PaddingValues(horizontal = pad)) {
            rowItems(items.take(10)) { item ->
                ContinueCard(
                    title = item.title,
                    meta = formatDate(item.addedSec),
                    seenText = null,
                    progress = 0f,
                    poster = null,
                    posterBlock = { posterOf(item)?.invoke() },
                    onClick = { onOpenItem(item) },
                )
            }
        }
        Spacer(Modifier.height(14.dp))
        // 全部（网格预览 6 个 → 进媒体库）
        SectionHeader("媒体库", onMore = onOpenLibrary)
        Spacer(Modifier.height(10.dp))
        LazyVerticalGrid(
            columns = GridCells.Fixed(3),
            horizontalArrangement = Arrangement.spacedBy(RexTokens.gap.dp),
            verticalArrangement = Arrangement.spacedBy(RexTokens.gap.dp),
            contentPadding = androidx.compose.foundation.layout.PaddingValues(
                horizontal = pad, vertical = pad),
            modifier = Modifier.fillMaxSize(),
        ) {
            items(items.take(12)) { item ->
                RexPosterCard(
                    title = item.title,
                    date = formatDate(item.addedSec),
                    poster = null,
                    posterBlock = { posterOf(item)?.invoke() },
                    onClick = { onOpenItem(item) },
                )
            }
        }
    }
}

/** 资源库页（Rex 云服务矩阵语义 + 我们的源集合；玻璃卡矩阵）。 */
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
            color = RexTokens.textMain)
        Spacer(Modifier.height(4.dp))
        Text("本地 ${videoCount} 个视频 · 云端与网络源", fontSize = 12.sp,
            color = RexTokens.textSub)
        Spacer(Modifier.height(14.dp))
        SourceCard("本地媒体库", "MediaStore 扫描 · 海报墙 · 点播", "📁") { onOpenLibrary() }
        Spacer(Modifier.height(RexTokens.gap.dp))
        SourceCard("SMB / 网络共享", "libsmb2 直连 · 硬解过本地代理 · 断点下载", "🗄") { onOpenSmb() }
        Spacer(Modifier.height(RexTokens.gap.dp))
        SourceCard("Emby / Jellyfin", "AuthenticateByName 登录 · 直连流播放", "☁") { onOpenEmby() }
        Spacer(Modifier.height(RexTokens.gap.dp))
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
                Text(title, fontSize = RexTokens.fontRowTitle.sp, fontWeight = FontWeight.Bold,
                    color = RexTokens.textMain)
                Text(desc, fontSize = 11.sp, color = RexTokens.textSub)
            }
            Spacer(Modifier.weight(1f))
            Text("›", fontSize = 22.sp, color = RexTokens.textSub)
        }
    }
}

/** 设置页（set_a 帧：inset 分组卡 59pt 行 + 节头 + 多行脚注"敢解释"）。 */
@Composable
fun SettingsPage(clearTextOk: Boolean, buildLabel: String) {
    Column(Modifier.fillMaxSize().padding(horizontal = pad)) {
        Spacer(Modifier.height(8.dp))
        Text("设置", fontSize = 28.sp, fontWeight = FontWeight.Bold,
            color = RexTokens.textMain)
        Spacer(Modifier.height(14.dp))
        GroupSection("播放") {
            GroupRow("网络明文传输", if (clearTextOk) "已开启" else "已关闭")
            GroupRow("解码", "硬解优先 · 失败报 reason")
        }
        Spacer(Modifier.height(8.dp))
        Text("本卡为设置页骨架：播放/字幕/解码各组随功能卡逐项接入（236 §六）。",
            fontSize = RexTokens.fontFootnote.sp, color = RexTokens.textSub,
            lineHeight = 15.sp)
        Spacer(Modifier.height(14.dp))
        GroupSection("关于") {
            GroupRow("构建", buildLabel)
            GroupRow("UI 对齐", "Rex 1.1 录屏规格（236 卡）")
        }
        Spacer(Modifier.height(8.dp))
        Text("kivo video 的界面规格来自 Rex 真机录屏逐帧拆解；所有数值为实测。",
            fontSize = RexTokens.fontFootnote.sp, color = RexTokens.textSub,
            lineHeight = 15.sp)
    }
}

@Composable
private fun GroupSection(header: String, content: @Composable () -> Unit) {
    Text(header, fontSize = 13.sp, color = RexTokens.textSub,
        modifier = Modifier.padding(start = 4.dp))
    Spacer(Modifier.height(6.dp))
    GlassSurface(Modifier.fillMaxWidth()) { Column { content() } }
}

@Composable
private fun GroupRow(title: String, value: String) {
    Row(
        Modifier
            .fillMaxWidth()
            .height(RexTokens.rowHeight.dp)
            .padding(horizontal = 16.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(title, fontSize = RexTokens.fontRowTitle.sp, color = RexTokens.textMain)
        Spacer(Modifier.weight(1f))
        Text(value, fontSize = 15.sp, color = RexTokens.textSub)
    }
}

/** 我的页（Rex 有观看统计/Pro——数据未建，v1 极简档案位，明示占位）。 */
@Composable
fun ProfilePage(videoCount: Int) {
    Column(Modifier.fillMaxSize().padding(horizontal = pad)) {
        Spacer(Modifier.height(8.dp))
        Text("我的", fontSize = 28.sp, fontWeight = FontWeight.Bold,
            color = RexTokens.textMain)
        Spacer(Modifier.height(14.dp))
        GlassSurface(Modifier.fillMaxWidth()) {
            Row(Modifier.padding(16.dp), verticalAlignment = Alignment.CenterVertically) {
                Box(
                    Modifier.size(56.dp).clip(CircleShape).background(Color(0xFFEFE9F2)),
                    contentAlignment = Alignment.Center
                ) { Text("👤", fontSize = 26.sp) }
                Spacer(Modifier.width(12.dp))
                Column {
                    Text("kivo 观众", fontSize = RexTokens.fontRowTitle.sp,
                        fontWeight = FontWeight.Bold, color = RexTokens.textMain)
                    Text("媒体库 ${videoCount} 个视频 · 统计随功能接入",
                        fontSize = 11.sp, color = RexTokens.textSub)
                }
            }
        }
    }
}

internal fun formatDate(addedSec: Long): String {
    val d = java.util.Calendar.getInstance()
    d.timeInMillis = addedSec * 1000L
    return "%04d-%02d-%02d".format(
        d.get(java.util.Calendar.YEAR), d.get(java.util.Calendar.MONTH) + 1,
        d.get(java.util.Calendar.DAY_OF_MONTH))
}
