package com.kivo.cinema.ui.design.home.v6.sections

import androidx.compose.foundation.background
import androidx.compose.foundation.border
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
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Shadow
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.ui.design.TmdbImage
import com.kivo.cinema.ui.design.home.v6.V6Metrics
import com.kivo.cinema.ui.design.home.v6.V6SectionHeader

/**
 * 排行榜海报行（高分剧集）：S 海报左上压大号名次（对标 Apple TV「Top Charts」），整榜可横滑、每部都能点开。
 * 名次下方一层浅暗角保证白字在亮海报上也读得清；副信息「★ 9.3 · 2023」，分数来自 TMDB（标题旁不再写来源，减少噪音）。
 */
@Composable
internal fun V6RankedRow(
    title: String,
    list: List<TmdbHome.Item>,
    fg: Color,
    sub: Color,
    onOpen: (TmdbHome.Item) -> Unit,
    onMore: (() -> Unit)? = null,
) {
    if (list.isEmpty()) return
    Column {
        V6SectionHeader(title, fg, sub, onMore = onMore)
        Spacer(Modifier.height(V6Metrics.headerGap))
        LazyRow(
            horizontalArrangement = Arrangement.spacedBy(V6Metrics.cardGap),
            contentPadding = PaddingValues(horizontal = V6Metrics.screenPad),
        ) {
            itemsIndexed(list) { i, m -> RankedPoster(i + 1, m, fg, sub, onOpen) }
        }
    }
}

@Composable
private fun RankedPoster(rank: Int, m: TmdbHome.Item, fg: Color, sub: Color, onOpen: (TmdbHome.Item) -> Unit) {
    val shape = RoundedCornerShape(V6Metrics.rPoster)
    Column(Modifier.width(V6Metrics.posterW).clickable { onOpen(m) }) {
        Box(Modifier.size(V6Metrics.posterW, V6Metrics.posterH).clip(shape)) {
            TmdbImage(m.posterUrl, m.title, Modifier.fillMaxSize())
            Box(Modifier.fillMaxSize().background(Brush.radialGradient(
                listOf(Color.Black.copy(alpha = .38f), Color.Transparent), center = Offset.Zero, radius = 180f)))
            Text(
                "$rank",
                modifier = Modifier.padding(start = 8.dp, top = 2.dp),
                style = TextStyle(
                    color = Color.White, fontSize = 34.sp, fontWeight = FontWeight.Black,
                    shadow = Shadow(Color.Black.copy(alpha = .35f), Offset(0f, 2f), 8f),
                ),
            )
            Box(Modifier.fillMaxSize().border(1.dp, V6Metrics.strokeOnLight, shape))
        }
        Spacer(Modifier.height(8.dp))
        Text(m.title, color = fg, fontSize = V6Metrics.fsCardTitle, fontWeight = FontWeight.Medium,
            maxLines = 1, overflow = TextOverflow.Ellipsis)
        Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            Text("★", color = V6Metrics.brass, fontSize = 11.sp)
            Text(" %.1f · %s".format(m.rating, m.year()), color = sub, fontSize = V6Metrics.fsMeta, maxLines = 1)
        }
    }
}
