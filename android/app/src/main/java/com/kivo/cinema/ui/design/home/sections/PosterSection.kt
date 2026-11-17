package com.kivo.cinema.ui.design.home.sections

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.kivo.cinema.tmdb.TmdbHome

/**
 * 海报节（正在热映 / 正在播出 / 今日动漫）—— ✅ 从 `DiscoverPage.kt` 等值搬迁，
 * key/顺序/间距（标题 13dp + 16dp、卡间距 12dp、水平 13dp、卡宽 104dp）保持不变。
 */
@Composable
internal fun PosterSection(
    title: String,
    list: List<TmdbHome.Item>,
    fg: Color,
    onOpenTmdbItem: (TmdbHome.Item) -> Unit,
) {
    Column {
        Spacer(Modifier.height(13.dp))
        HomeSectionHeader(title, fg)
        Spacer(Modifier.height(16.dp))
        LazyRow(
            horizontalArrangement = Arrangement.spacedBy(12.dp),
            contentPadding = PaddingValues(horizontal = 13.dp),
        ) {
            items(list) { m ->
                TmdbPosterCard(m, fg) { onOpenTmdbItem(m) }
            }
        }
    }
}