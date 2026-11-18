package com.kivo.cinema.ui.design.home.sections

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.kivo.cinema.library.HomeEntry

/**
 * 首页「本地片库」某一分类的横排。
 *
 * 规格**与 TMDB 的 [PosterSection] 完全一致**（用户要求）：标题上留 13dp、标题下 16dp、
 * 卡间距 12dp、水平内边距 13dp、竖版 104×156 卡。空列表整行不渲染。
 */
@Composable
internal fun NasLibraryRow(
    title: String,
    entries: List<HomeEntry>,
    fg: Color,
    sub: Color,
    posterOf: @Composable (HomeEntry) -> (@Composable () -> Unit)?,
    onOpenItem: (HomeEntry) -> Unit,
) {
    if (entries.isEmpty()) return
    Column {
        Spacer(Modifier.height(13.dp))
        HomeSectionHeader(title, fg)
        Spacer(Modifier.height(16.dp))
        LazyRow(
            horizontalArrangement = Arrangement.spacedBy(12.dp),
            contentPadding = PaddingValues(horizontal = 13.dp),
        ) {
            itemsIndexed(entries) { _, entry ->
                NasLibraryCard(
                    entry = entry,
                    fg = fg,
                    sub = sub,
                    posterBlock = posterOf(entry),
                    onClick = { onOpenItem(entry) },
                )
            }
        }
    }
}
