package com.kivo.cinema.ui.detail

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
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
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage

/** 家长指南卡片高（帧实测 55×0.914）。 */
private const val DETAIL_GUIDE_CARD_DP = 50f
/** 相似作品卡宽（帧实测 331×0.914）。 */
private const val DETAIL_SIMILAR_CARD_DP = 302f
/** 相似作品卡间距。 */
private const val DETAIL_SIMILAR_GAP_DP = 23f
/** 演职员头像直径（帧实测 55dp）。 */
private const val DETAIL_CAST_AVATAR_DP = 55f
/** 头像→名字间距（帧实测 20.3dp 含字面上留白，故取 18）。 */
private const val DETAIL_CAST_NAME_GAP_DP = 18f

/**
 * 详情页各节组件（240 §1 E7-E13）。文字色随色场传入（暗场白字 / 浅场深字）；
 * 节标题 22sp 粗；无数据的节由调用方整节隐藏（不渲染空壳）。
 */

/** 节标题（E7）：帧实测同一字符串墨高比 22sp:18sp = 1.227，故本页用 18sp。 */
@Composable
internal fun SectionHeader(title: String, color: Color) {
    Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
        Text(title, fontSize = DETAIL_SECTION_TITLE_SP.sp,
            fontWeight = FontWeight.Bold, color = color,
            modifier = Modifier.padding(horizontal = DETAIL_PAD_DP.dp))
    }
}

/** 演职员行（E7）：头像 Ø50 圆 + 名 12sp + 角色 10sp 灰，横滑。 */
@Composable
internal fun CastRow(cast: List<TmdbDetail.CastMember>, fg: Color, fgSub: Color) {
    LazyRow(
        horizontalArrangement = Arrangement.spacedBy(14.dp),
        contentPadding = PaddingValues(horizontal = DETAIL_PAD_DP.dp),
    ) {
        items(cast) { member ->
            Column(horizontalAlignment = Alignment.CenterHorizontally,
                modifier = Modifier.width(72.dp)) {
                Box(Modifier.size(DETAIL_CAST_AVATAR_DP.dp).clip(CircleShape)) {
                    val url = member.profileUrl
                    if (url != null) {
                        TmdbImage(url, member.name, Modifier.fillMaxSize())
                    } else {
                        Box(Modifier.fillMaxSize()
                            .background(Color(0xFFE3DEEA), CircleShape))
                    }
                }
                Spacer(Modifier.height(DETAIL_CAST_NAME_GAP_DP.dp))
                Text(member.name, fontSize = 13.sp, lineHeight = 16.sp, color = fg,
                    maxLines = 2, overflow = TextOverflow.Ellipsis)
                if (member.role.isNotEmpty()) {
                    Spacer(Modifier.height(1.dp))
                    Text(member.role, fontSize = 11.sp,
                        color = fgSub, maxLines = 1,
                        overflow = TextOverflow.Ellipsis)
                }
            }
        }
    }
}

/** 分类 chips（E8）：白底全圆胶囊 13sp 粗（画布元素，始终白底深字）。 */
@Composable
internal fun GenreChips(genres: List<String>) {
    Row(Modifier.padding(horizontal = DETAIL_PAD_DP.dp),
        horizontalArrangement = Arrangement.spacedBy(10.dp)) {
        genres.take(3).forEach { g ->
            Text(g, fontSize = 15.sp, fontWeight = FontWeight.Bold,
                color = DesignTokens.textMain,
                modifier = Modifier
                    .clip(RoundedCornerShape(50))
                    .background(DesignTokens.canvasFill)
                    .padding(horizontal = 16.dp, vertical = 7.dp))
        }
    }
}

/** 预告片卡（E9）：16:9 圆角 + 缩略图 + 中央播放钮；点击跳外部观看。 */
@Composable
internal fun TrailerCard(key: String, name: String, onClick: () -> Unit) {
    Box(
        Modifier
            .padding(horizontal = DETAIL_PAD_DP.dp)
            .fillMaxWidth()
            .aspectRatio(16f / 9f)
            .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
            .clickable { onClick() }
    ) {
        TmdbImage("https://img.youtube.com/vi/$key/hqdefault.jpg", name,
            Modifier.fillMaxSize())
        Box(Modifier.fillMaxSize().background(Color.Black.copy(alpha = 0.25f)))
        Box(Modifier.align(Alignment.Center).size(52.dp)
            .clip(CircleShape).background(Color.Black.copy(alpha = 0.55f)),
            contentAlignment = Alignment.Center) {
            Text("▶", color = Color.White, fontSize = 20.sp)
        }
    }
}

/** 剧照行（E10）：横滑 16:9 卡，宽 220dp。 */
@Composable
internal fun StillsRow(stills: List<String>) {
    LazyRow(
        horizontalArrangement = Arrangement.spacedBy(10.dp),
        contentPadding = PaddingValues(horizontal = DETAIL_PAD_DP.dp),
    ) {
        items(stills) { url ->
            Box(Modifier.width(220.dp).aspectRatio(16f / 9f)
                .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))) {
                TmdbImage(url, "剧照", Modifier.fillMaxSize())
            }
        }
    }
}

/** 相似作品（E13，帧实测为**横向卡片流**）：16:9 海报 151×85 + 右侧 片名/★分·日期/3 行简介。 */
@Composable
internal fun SimilarList(items: List<TmdbDetail.SimilarItem>, fg: Color, fgSub: Color,
                         onOpen: (TmdbDetail.SimilarItem) -> Unit) {
    LazyRow(
        horizontalArrangement = Arrangement.spacedBy(DETAIL_SIMILAR_GAP_DP.dp),
        contentPadding = PaddingValues(horizontal = DETAIL_PAD_DP.dp),
    ) {
        items(items) { m ->
            Row(
                Modifier.width(DETAIL_SIMILAR_CARD_DP.dp).clickable { onOpen(m) },
                horizontalArrangement = Arrangement.spacedBy(9.dp),
            ) {
                Box(Modifier.width(151.dp).height(85.dp)
                    .clip(RoundedCornerShape(DesignTokens.radiusPoster.dp))) {
                    TmdbImage(m.posterUrl, m.title, Modifier.fillMaxSize())
                }
                Column(Modifier.weight(1f)) {
                    Text(m.title, fontSize = 16.sp, fontWeight = FontWeight.Bold,
                        color = fg, maxLines = 1, overflow = TextOverflow.Ellipsis)
                    Spacer(Modifier.height(2.dp))
                    Text(buildString {
                        if (m.rating > 0f) append("★ %.1f  ".format(m.rating))
                        append(m.date)
                    }, fontSize = 13.sp, color = fgSub, maxLines = 1)
                    if (m.overview.isNotBlank()) {
                        Spacer(Modifier.height(4.dp))
                        Text(m.overview, fontSize = 13.sp, lineHeight = 17.sp,
                            color = fg.copy(alpha = .88f), maxLines = 3,
                            overflow = TextOverflow.Ellipsis)
                    }
                }
            }
        }
    }
}

/** 「加入收藏夹」图标（帧实测）：圆角方框描边 + 顶上一道堆叠线 + 左下圆环带加号。 */
@Composable
internal fun CollectionPlusIcon(color: Color, modifier: Modifier = Modifier) {
    Canvas(modifier) {
        val w = size.width
        val h = size.height
        val stroke = 1.9.dp.toPx()
        drawLine(color, Offset(w * .20f, h * .10f), Offset(w * .80f, h * .10f),
            strokeWidth = stroke, cap = StrokeCap.Round)
        drawRoundRect(color, topLeft = Offset(w * .16f, h * .26f),
            size = androidx.compose.ui.geometry.Size(w * .68f, h * .60f),
            cornerRadius = androidx.compose.ui.geometry.CornerRadius(3.dp.toPx()),
            style = Stroke(stroke))
        val cx = w * .18f
        val cy = h * .82f
        val cr = w * .17f
        drawCircle(color, cr, Offset(cx, cy), style = Stroke(stroke))
        val pl = cr * .55f
        drawLine(color, Offset(cx - pl, cy), Offset(cx + pl, cy),
            strokeWidth = stroke * .85f, cap = StrokeCap.Round)
        drawLine(color, Offset(cx, cy - pl), Offset(cx, cy + pl),
            strokeWidth = stroke * .85f, cap = StrokeCap.Round)
    }
}

/** 心形描边图标（帧实测：空心、圆头描边）。 */
@Composable
internal fun HeartIcon(color: Color, modifier: Modifier = Modifier) {
    Canvas(modifier) {
        val w = size.width
        val h = size.height
        val p = Path().apply {
            moveTo(w * .5f, h * .93f)
            cubicTo(w * .10f, h * .62f, w * .03f, h * .33f, w * .24f, h * .17f)
            cubicTo(w * .38f, h * .06f, w * .48f, h * .18f, w * .5f, h * .27f)
            cubicTo(w * .52f, h * .18f, w * .62f, h * .06f, w * .76f, h * .17f)
            cubicTo(w * .97f, h * .33f, w * .90f, h * .62f, w * .5f, h * .93f)
            close()
        }
        drawPath(p, color, style = Stroke(1.9.dp.toPx(),
            cap = StrokeCap.Round, join = StrokeJoin.Round))
    }
}

/** 空态块（无本地资源，240 §6.1）：TV 图标 + 暂无可用资源 + 添加到待看。 */
@Composable
internal fun EmptyResourceBlock(fg: Color) {
    Column(Modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
        Canvas(Modifier.size(86.dp, 64.dp)) {
            val w = size.width
            val h = size.height
            val bodyTop = h * .16f
            drawLine(fg.copy(alpha = .22f), Offset(w * .38f, bodyTop), Offset(w * .26f, 0f),
                strokeWidth = 2.4.dp.toPx(), cap = StrokeCap.Round)
            drawLine(fg.copy(alpha = .22f), Offset(w * .62f, bodyTop), Offset(w * .74f, 0f),
                strokeWidth = 2.4.dp.toPx(), cap = StrokeCap.Round)
            drawRoundRect(fg.copy(alpha = .16f), topLeft = Offset(0f, bodyTop),
                size = androidx.compose.ui.geometry.Size(w, h - bodyTop),
                cornerRadius = androidx.compose.ui.geometry.CornerRadius(9.dp.toPx()))
            val pad = 9.dp.toPx()
            drawRoundRect(fg.copy(alpha = .26f),
                topLeft = Offset(pad, bodyTop + pad),
                size = androidx.compose.ui.geometry.Size(w - pad * 2, h - bodyTop - pad * 2),
                cornerRadius = androidx.compose.ui.geometry.CornerRadius(5.dp.toPx()))
            val cx = w / 2f
            val cy = bodyTop + (h - bodyTop) / 2f
            val r = 9.dp.toPx()
            val tri = Path().apply {
                moveTo(cx - r * .5f, cy - r)
                lineTo(cx + r, cy)
                lineTo(cx - r * .5f, cy + r)
                close()
            }
            drawPath(tri, fg.copy(alpha = .85f))
        }
        Spacer(Modifier.height(14.dp))
        Text("暂无可用资源", fontSize = 15.sp, fontWeight = FontWeight.Bold, color = fg)
        Spacer(Modifier.height(6.dp))
        Text("添加到待看", fontSize = 12.sp, color = fg.copy(alpha = .55f))
    }
}

/** 片源/出品方 logo（居中，高 26dp）。 */
@Composable
internal fun StudioLogo(url: String, fg: Color) {
    Box(Modifier.fillMaxWidth().padding(top = 26.dp), contentAlignment = Alignment.Center) {
        TmdbImage(url, "出品方", Modifier.width(150.dp).height(26.dp),
            bg = Color.Transparent, fit = true)
    }
}

/**
 * 家长指南（240 §6.1 E-家长指南）：两卡并排，左侧色条 + 项名 + 说明。
 * 数据 = TMDB `release_dates`（分级 60 国 + 少量描述符）；**无严重度/票数**（TMDB 不提供，
 * 与参照的 IMDb Parents Guide 口径不同，故不显示假的「轻微/严重（N 票）」）。
 */
@Composable
internal fun ParentalGuide(certs: List<Pair<String, String>>, notes: List<String>,
                           fg: Color, fgSub: Color) {
    Row(
        Modifier.padding(horizontal = DETAIL_PAD_DP.dp).fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        if (certs.isNotEmpty()) {
            GuideCard("分级", certs.joinToString(" · ") { "${it.first} ${it.second}" },
                Color(0xFFE8B33A), fg, fgSub, Modifier.weight(1f))
        }
        if (notes.isNotEmpty()) {
            GuideCard("内容提示", notes.take(2).joinToString("、"),
                Color(0xFFD8443C), fg, fgSub, Modifier.weight(1f))
        }
    }
}

@Composable
private fun GuideCard(title: String, value: String, accent: Color,
                      fg: Color, fgSub: Color, modifier: Modifier = Modifier) {
    Row(
        modifier.height(DETAIL_GUIDE_CARD_DP.dp)
            .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
            .background(DesignTokens.canvasFill)
            .padding(horizontal = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Box(Modifier.width(3.dp).height(30.dp)
            .clip(RoundedCornerShape(2.dp)).background(accent))
        Spacer(Modifier.width(10.dp))
        Column {
            Text(title, fontSize = 15.sp, fontWeight = FontWeight.Bold, color = fg,
                maxLines = 1, overflow = TextOverflow.Ellipsis)
            Spacer(Modifier.height(3.dp))
            Text(value, fontSize = 13.sp, color = fgSub, maxLines = 1,
                overflow = TextOverflow.Ellipsis)
        }
    }
}

/** 播放资源行（240 §9.3）：标题 + 清晰度胶囊。 */
@Composable
internal fun PlaySourceHeader(quality: String, fg: Color) {
    Row(
        Modifier.padding(horizontal = DETAIL_PAD_DP.dp).fillMaxWidth(),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text("播放资源:", fontSize = 18.sp, fontWeight = FontWeight.Bold, color = fg)
        Spacer(Modifier.width(12.dp))
        Text(
            quality, fontSize = 15.sp, fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
            modifier = Modifier
                .clip(RoundedCornerShape(50))
                .background(DesignTokens.canvasFill)
                .padding(horizontal = 18.dp, vertical = 6.dp),
        )
    }
}

/** 媒体信息卡（240 §9.3）：白卡 = 分辨率 / 容量·码率 / 源名+绿勾。 */
@Composable
internal fun MediaCard(m: MediaMatch.Matched) {
    Column(
        Modifier.padding(horizontal = DETAIL_PAD_DP.dp).fillMaxWidth()
            .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
            .background(DesignTokens.canvasFill)
            .padding(horizontal = 14.dp, vertical = 14.dp),
    ) {
        Text(m.resolution, fontSize = 17.sp, fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain)
        Spacer(Modifier.height(8.dp))
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(formatSize(m.sizeBytes), fontSize = 13.sp, color = DesignTokens.textSub)
            Spacer(Modifier.width(14.dp))
            Text(formatBitrate(m.bitrateBps), fontSize = 13.sp, color = DesignTokens.textSub)
        }
        Spacer(Modifier.height(10.dp))
        Row(verticalAlignment = Alignment.CenterVertically) {
            Box(Modifier.size(7.dp).clip(CircleShape).background(Color(0xFF34C759)))
            Spacer(Modifier.width(6.dp))
            Text("本地", fontSize = 13.sp, color = DesignTokens.textMain)
            Spacer(Modifier.weight(1f))
            Text("✓", fontSize = 15.sp, color = Color(0xFF34C759))
        }
    }
}

/** 展开媒体信息（240 §9.3）：卡下居中一行 + 折角。 */
@Composable
internal fun ExpandMediaInfoRow(fgSub: Color) {
    Row(
        Modifier.fillMaxWidth().padding(top = 12.dp),
        horizontalArrangement = Arrangement.Center,
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text("展开媒体信息", fontSize = 13.sp, color = fgSub)
        Spacer(Modifier.width(6.dp))
        Text("⌄", fontSize = 13.sp, color = fgSub)
    }
}

private fun formatSize(bytes: Long): String = when {
    bytes >= 1073741824L -> "%.1f GB".format(bytes / 1073741824.0)
    bytes >= 1048576L -> "%.0f MB".format(bytes / 1048576.0)
    else -> "%.0f KB".format(bytes / 1024.0)
}

private fun formatBitrate(bps: Long): String =
    if (bps >= 1_000_000L) "%.2fMbps".format(bps / 1_000_000.0)
    else "%.0fkbps".format(bps / 1000.0)
