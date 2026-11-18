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
import com.kivo.cinema.ui.design.ContinueCard
import com.kivo.cinema.ui.design.fmtSeen
import com.kivo.cinema.ui.design.relDays

/**
 * 继续观看横排 —— 多源版（本地 / SMB / Emby）。标题 13dp、列表 12dp、卡间距 10dp、
 * 水平 13dp 与旧版完全同距，只把入参从「本地 MediaItem+Progress」换成跨源 [HomeEntry]。
 *
 * 来源与时间文案在**本层**拼（数据层只给 [HomeEntry.addedSec]/[HomeEntry.sourceLabel] 原始值），
 * 这样 `library` 不依赖任何 UI 格式化函数。
 */
@Composable
internal fun ContinueRow(
    entries: List<HomeEntry>,
    fg: Color,
    sub: Color,
    posterOf: @Composable (HomeEntry) -> (@Composable () -> Unit)?,
    onOpenItem: (HomeEntry) -> Unit,
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
            itemsIndexed(entries) { _, entry ->
                ContinueCard(
                    title = entry.title,
                    meta = metaOf(entry),
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

/** "3 天前 · Emby"；入库时间未知时只给来源，不硬编一个假时间。 */
private fun metaOf(entry: HomeEntry): String =
    if (entry.addedSec > 0) "${relDays(entry.addedSec)} · ${entry.sourceLabel}"
    else entry.sourceLabel
