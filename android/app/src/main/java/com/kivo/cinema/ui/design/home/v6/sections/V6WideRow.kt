package com.kivo.cinema.ui.design.home.v6.sections

import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.ui.design.TmdbImage
import com.kivo.cinema.ui.design.home.v6.V6Metrics
import com.kivo.cinema.ui.design.home.v6.V6SectionHeader

/**
 * 16:9 剧照行（热门剧集）：248×140，页面中段的第二种比例，打破连续的 2:3 海报节奏。
 * 剧照缺失时退回竖版海报（裁中间），不留空卡。
 */
@Composable
internal fun V6WideRow(
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
            items(list) { m ->
                Column(Modifier.width(V6Metrics.wideW).clickable { onOpen(m) }) {
                    val shape = RoundedCornerShape(V6Metrics.rCard)
                    Box(Modifier.size(V6Metrics.wideW, V6Metrics.wideH).clip(shape)) {
                        TmdbImage(m.backdropUrl.ifBlank { m.posterUrl }, m.title, Modifier.fillMaxSize())
                        Box(Modifier.fillMaxSize().border(1.dp, V6Metrics.strokeOnLight, shape))
                    }
                    Spacer(Modifier.height(10.dp))
                    Text(m.title, color = fg, fontSize = V6Metrics.fsWideTitle, fontWeight = FontWeight.Medium,
                        maxLines = 1, overflow = TextOverflow.Ellipsis)
                    Text(m.tagLine(), color = sub, fontSize = V6Metrics.fsMeta, maxLines = 1,
                        overflow = TextOverflow.Ellipsis)
                }
            }
        }
    }
}
