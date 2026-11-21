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
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.stats.WatchStore
import com.kivo.cinema.ui.design.ContinueCard
import com.kivo.cinema.ui.design.fmtSeen
import com.kivo.cinema.ui.design.relDays
import com.kivo.cinema.ui.design.sourceOf

/**
 * 继续观看横排 —— ✅ 从 `DiscoverPage.kt` 等值搬迁（标题 13dp、列表 12dp、
 * 卡间距 10dp、水平 13dp 不变）。
 *
 * 【为什么把 `sourceOf` 留在 DiscoverPage】它是 Shell/其它页面也会用到的来源判定，
 * 迁移到 sections 会让「来源解析」这条职责跟着视图文件走；这里只调用，不拥有。
 */
@Composable
internal fun ContinueRow(
    items: List<com.kivo.cinema.library.HomeEntry>,
    fg: Color,
    sub: Color,
    posterOf: @Composable (com.kivo.cinema.library.HomeEntry) -> (@Composable () -> Unit)?,
    onOpenItem: (com.kivo.cinema.library.HomeEntry) -> Unit,
    onShowWatching: (() -> Unit)? = null,
) {
    Column {
        Spacer(Modifier.height(13.dp))
        HomeSectionHeader("继续观看", fg, onShowWatching)
        Spacer(Modifier.height(12.dp))
        LazyRow(
            horizontalArrangement = Arrangement.spacedBy(10.dp),
            contentPadding = PaddingValues(horizontal = 13.dp),
        ) {
            itemsIndexed(items) { _, entry ->
                ContinueCard(
                    title = entry.title,
                    meta = "${relDays(entry.addedSec)} · ${entry.sourceLabel}",
                    seenText = "看到 " + fmtSeen(entry.positionMs),
                    progress = entry.progress,
                    posterBlock = { posterOf(entry)?.invoke() },
                    onClick = { onOpenItem(entry) },
                    titleColor = fg,
                    metaColor = sub,
                )
            }
        }
    }
}