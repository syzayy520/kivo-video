package com.kivo.cinema.ui.design.home.v6.sections

import androidx.compose.foundation.background
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
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.home.v6.QualityBadge
import com.kivo.cinema.ui.design.home.v6.V6Metrics
import com.kivo.cinema.ui.design.home.v6.V6SectionHeader
import com.kivo.cinema.ui.design.home.v6.qualityBadgeText

/**
 * 片库分类行（NAS 分节，如「电影」「剧集」）：S 海报 106×159 + 左下单枚画质标（文件名解析）。
 * 海报图由 Shell 的 posterOf 提供（库内封面 / 抽帧 / TMDB 兜底），本文件只管排版。
 */
@Composable
internal fun V6LibraryRow(
    title: String,
    entries: List<HomeEntry>,
    fg: Color,
    sub: Color,
    posterOf: @Composable (HomeEntry) -> (@Composable () -> Unit)?,
    onOpen: (HomeEntry) -> Unit,
    onMore: (() -> Unit)? = null,
) {
    if (entries.isEmpty()) return
    Column {
        V6SectionHeader(title, fg, sub, onMore = onMore)
        Spacer(Modifier.height(V6Metrics.headerGap))
        LazyRow(
            horizontalArrangement = Arrangement.spacedBy(V6Metrics.cardGap),
            contentPadding = PaddingValues(horizontal = V6Metrics.screenPad),
        ) {
            items(entries) { entry -> LibraryPoster(entry, fg, sub, posterOf, onOpen) }
        }
    }
}

@Composable
private fun LibraryPoster(
    entry: HomeEntry,
    fg: Color,
    sub: Color,
    posterOf: @Composable (HomeEntry) -> (@Composable () -> Unit)?,
    onOpen: (HomeEntry) -> Unit,
) {
    val shape = RoundedCornerShape(V6Metrics.rPoster)
    Column(Modifier.width(V6Metrics.posterW).clickable { onOpen(entry) }) {
        Box(Modifier.size(V6Metrics.posterW, V6Metrics.posterH).clip(shape).background(DesignTokens.posterPlaceholder)) {
            Box(Modifier.fillMaxSize()) { posterOf(entry)?.invoke() }
            qualityBadgeText(entry.key.substringAfterLast('/'))?.let {
                QualityBadge(it, Modifier.align(Alignment.BottomStart).padding(6.dp))
            }
            Box(Modifier.fillMaxSize().border(1.dp, V6Metrics.strokeOnLight, shape))
        }
        Spacer(Modifier.height(8.dp))
        Text(entry.title, color = fg, fontSize = V6Metrics.fsCardTitle, fontWeight = FontWeight.Medium,
            maxLines = 1, overflow = TextOverflow.Ellipsis)
        // 副信息只写年份（剧集写集数）：整行都是 NAS，每张卡再写一遍「NAS」只是噪音。
        Text(entry.year?.takeIf { it > 0 }?.toString() ?: entry.episodeLabel.orEmpty(), color = sub,
            fontSize = V6Metrics.fsMeta, maxLines = 1, overflow = TextOverflow.Ellipsis)
    }
}
