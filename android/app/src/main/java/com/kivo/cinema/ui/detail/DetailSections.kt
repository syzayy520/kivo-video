package com.kivo.cinema.ui.detail

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
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
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage

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
                Text(member.name, fontSize = 13.sp, color = fg,
                    maxLines = 1, overflow = TextOverflow.Ellipsis)
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

/** 相似作品（E13）：单列竖排，海报 92×138 + 右文（片名/★评分/日期）。 */
@Composable
internal fun SimilarList(items: List<TmdbDetail.SimilarItem>, fg: Color, fgSub: Color,
                         onOpen: (TmdbDetail.SimilarItem) -> Unit) {
    Column(Modifier.padding(horizontal = DETAIL_PAD_DP.dp)) {
        items.forEach { m ->
            Row(
                Modifier.fillMaxWidth().clickable { onOpen(m) }
                    .padding(vertical = 6.dp),
                horizontalArrangement = Arrangement.spacedBy(12.dp),
            ) {
                Box(Modifier.width(92.dp).height(138.dp)
                    .clip(RoundedCornerShape(DesignTokens.radiusPoster.dp))) {
                    TmdbImage(m.posterUrl, m.title, Modifier.fillMaxSize())
                }
                Column(Modifier.weight(1f)) {
                    Text(m.title, fontSize = 15.sp, fontWeight = FontWeight.Bold,
                        color = fg, maxLines = 2,
                        overflow = TextOverflow.Ellipsis)
                    Spacer(Modifier.height(4.dp))
                    Text(buildString {
                        if (m.rating > 0f) append("★ %.1f  ".format(m.rating))
                        append(m.date)
                    }, fontSize = 13.sp, color = fgSub, maxLines = 1)
                    if (m.overview.isNotBlank()) {
                        Spacer(Modifier.height(4.dp))
                        Text(m.overview, fontSize = 13.sp, lineHeight = 17.sp,
                            color = fgSub.copy(alpha = .85f), maxLines = 3,
                            overflow = TextOverflow.Ellipsis)
                    }
                }
            }
        }
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
            val body = androidx.compose.ui.geometry.Rect(0f, bodyTop, w, h)
            // 天线
            drawLine(fg.copy(alpha = .22f), Offset(w * .38f, bodyTop), Offset(w * .26f, 0f),
                strokeWidth = 2.4.dp.toPx(), cap = androidx.compose.ui.graphics.StrokeCap.Round)
            drawLine(fg.copy(alpha = .22f), Offset(w * .62f, bodyTop), Offset(w * .74f, 0f),
                strokeWidth = 2.4.dp.toPx(), cap = androidx.compose.ui.graphics.StrokeCap.Round)
            // 机身
            drawRoundRect(fg.copy(alpha = .16f), topLeft = body.topLeft,
                size = androidx.compose.ui.geometry.Size(w, h - bodyTop),
                cornerRadius = androidx.compose.ui.geometry.CornerRadius(9.dp.toPx()))
            // 屏幕
            val pad = 9.dp.toPx()
            drawRoundRect(fg.copy(alpha = .26f),
                topLeft = Offset(pad, bodyTop + pad),
                size = androidx.compose.ui.geometry.Size(w - pad * 2, h - bodyTop - pad * 2),
                cornerRadius = androidx.compose.ui.geometry.CornerRadius(5.dp.toPx()))
            // 播放三角
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
