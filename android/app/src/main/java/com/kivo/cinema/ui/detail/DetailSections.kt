package com.kivo.cinema.ui.detail

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
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbDetail
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage

/**
 * 详情页各节组件（240 §1 E7-E13）。浅色场上的深色文字；节标题 22sp 粗；
 * 无数据的节由调用方整节隐藏（不渲染空壳）。
 */

/** 节标题（E7/首页同款：22sp 粗 + 可选 ›）。 */
@Composable
internal fun SectionHeader(title: String, onMore: (() -> Unit)? = null) {
    Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
        Text(title, fontSize = DesignTokens.fontSectionHeader.sp,
            fontWeight = FontWeight.Bold, color = DesignTokens.textMain,
            modifier = Modifier.padding(horizontal = DesignTokens.screenPad.dp))
        Spacer(Modifier.weight(1f))
        if (onMore != null) {
            Text("›", fontSize = 26.sp, color = DesignTokens.textMain,
                modifier = Modifier.padding(horizontal = DesignTokens.screenPad.dp))
        }
    }
}

/** 演职员行（E7）：头像 Ø50 圆 + 名 12sp 粗 + 角色 10sp 灰，横滑。 */
@Composable
internal fun CastRow(cast: List<TmdbDetail.CastMember>) {
    LazyRow(
        horizontalArrangement = Arrangement.spacedBy(12.dp),
        contentPadding = PaddingValues(horizontal = DesignTokens.screenPad.dp),
    ) {
        items(cast) { member ->
            Column(horizontalAlignment = Alignment.CenterHorizontally,
                modifier = Modifier.width(64.dp)) {
                Box(Modifier.size(50.dp).clip(CircleShape)) {
                    member.profileUrl?.let {
                        TmdbImage(it, member.name, Modifier.fillMaxSize())
                    } ?: Box(Modifier.fillMaxSize()
                        .background(Color(0xFFE3DEEA), CircleShape))
                }
                Spacer(Modifier.height(4.dp))
                Text(member.name, fontSize = 12.sp, color = DesignTokens.textMain,
                    maxLines = 1, overflow = TextOverflow.Ellipsis)
                if (member.role.isNotEmpty()) Text(member.role, fontSize = 10.sp,
                    color = DesignTokens.textSub, maxLines = 1,
                    overflow = TextOverflow.Ellipsis)
            }
        }
    }
}

/** 分类 chips（E8）：白底全圆胶囊 13sp 粗。 */
@Composable
internal fun GenreChips(genres: List<String>) {
    Row(Modifier.padding(horizontal = DesignTokens.screenPad.dp),
        horizontalArrangement = Arrangement.spacedBy(10.dp)) {
        genres.take(3).forEach { g ->
            Text(g, fontSize = 13.sp, fontWeight = FontWeight.Bold,
                color = DesignTokens.textMain,
                modifier = Modifier
                    .clip(RoundedCornerShape(50))
                    .background(DesignTokens.canvasFill)
                    .padding(horizontal = 16.dp, vertical = 6.dp))
        }
    }
}

/** 预告片卡（E9）：16:9 圆角 + 青管缩略图 + 中央播放钮；点击跳 YouTube 观看。 */
@Composable
internal fun TrailerCard(key: String, name: String, onClick: () -> Unit) {
    Box(
        Modifier
            .padding(horizontal = DesignTokens.screenPad.dp)
            .fillMaxWidth()
            .height(220.dp)
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
        Text(name, fontSize = 12.sp, color = Color.White,
            modifier = Modifier.align(Alignment.BottomStart)
                .padding(10.dp), maxLines = 1, overflow = TextOverflow.Ellipsis)
    }
}

/** 相似作品列表（E13）：海报 ~92×138 + 片名 + ★评分 + 日期，两列网格流。 */
@Composable
internal fun SimilarList(items: List<TmdbDetail.SimilarItem>, onOpen: (TmdbDetail.SimilarItem) -> Unit) {
    items.chunked(2).forEach { pair ->
        Row(Modifier.padding(horizontal = DesignTokens.screenPad.dp, vertical = 6.dp),
            horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            pair.forEach { m ->
                Column(
                    Modifier.weight(1f).clickable { onOpen(m) },
                ) {
                    Box(Modifier.height(138.dp).fillMaxWidth()
                        .clip(RoundedCornerShape(DesignTokens.radiusPoster.dp))) {
                        TmdbImage(m.posterUrl, m.title, Modifier.fillMaxSize())
                    }
                    Spacer(Modifier.height(6.dp))
                    Text(m.title, fontSize = 15.sp, fontWeight = FontWeight.Bold,
                        color = DesignTokens.textMain, maxLines = 1,
                        overflow = TextOverflow.Ellipsis)
                    Text(buildString {
                        if (m.rating > 0f) append("★ %.1f  ".format(m.rating))
                        append(m.date)
                    }, fontSize = 13.sp, color = DesignTokens.textSub, maxLines = 1)
                }
            }
            if (pair.size == 1) Spacer(Modifier.weight(1f))
        }
    }
}
