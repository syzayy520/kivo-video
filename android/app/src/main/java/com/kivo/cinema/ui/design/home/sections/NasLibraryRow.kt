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

/**
 * 首页「本地片库」横排：展示 NAS 扫描索引里的影片（P3）。
 *
 * 与「继续观看」的区别：这里**没有播放进度**（progress=0），表达的是"库里有什么"。
 * 空列表时整行不渲染（返回空），调用方可以直接无条件放一个 item。
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
        Spacer(Modifier.height(12.dp))
        LazyRow(
            horizontalArrangement = Arrangement.spacedBy(10.dp),
            contentPadding = PaddingValues(horizontal = 13.dp),
        ) {
            itemsIndexed(entries) { _, entry ->
                ContinueCard(
                    title = entry.title,
                    meta = entry.sourceLabel,
                    seenText = null,
                    progress = 0f,
                    posterBlock = { posterOf(entry)?.invoke() },
                    onClick = { onOpenItem(entry) },
                    titleColor = fg,
                    metaColor = sub,
                )
            }
        }
    }
}
