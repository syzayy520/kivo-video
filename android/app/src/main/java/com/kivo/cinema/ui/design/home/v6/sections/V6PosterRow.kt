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
 * TMDB 海报行（院线新片 / 今日动漫）：S 海报 106×159，一屏 3 张 + 第 4 张露头；海报上不放操作按钮（想看在详情页）。
 * 副信息由调用方给（院线写上映日，动漫写年份），本文件只管排版。
 */
@Composable
internal fun V6PosterRow(
    title: String,
    list: List<TmdbHome.Item>,
    fg: Color,
    sub: Color,
    meta: (TmdbHome.Item) -> String,
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
                Column(Modifier.width(V6Metrics.posterW).clickable { onOpen(m) }) {
                    val shape = RoundedCornerShape(V6Metrics.rPoster)
                    Box(Modifier.size(V6Metrics.posterW, V6Metrics.posterH).clip(shape)) {
                        TmdbImage(m.posterUrl, m.title, Modifier.fillMaxSize())
                        Box(Modifier.fillMaxSize().border(1.dp, V6Metrics.strokeOnLight, shape))
                    }
                    Spacer(Modifier.height(8.dp))
                    Text(m.title, color = fg, fontSize = V6Metrics.fsCardTitle, fontWeight = FontWeight.Medium,
                        maxLines = 1, overflow = TextOverflow.Ellipsis)
                    Text(meta(m), color = sub, fontSize = V6Metrics.fsMeta, maxLines = 1)
                }
            }
        }
    }
}

/** 「2026-11-20」→「11 月 20 日」；格式不对时原样返回。 */
internal fun monthDay(date: String): String {
    val p = date.split('-')
    return if (p.size == 3) "${p[1].toIntOrNull() ?: return date} 月 ${p[2].toIntOrNull() ?: return date} 日" else date
}
