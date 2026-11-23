package com.kivo.cinema.ui.design.home.v6.list

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.asPaddingValues
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.GridItemSpan
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.home.sections.NasPoster
import com.kivo.cinema.ui.design.home.v6.QualityBadge
import com.kivo.cinema.ui.design.home.v6.V6Metrics
import com.kivo.cinema.ui.design.home.v6.qualityBadgeText

/**
 * NAS 分类「查看全部」视图：与 TMDB 榜单页同一套外框（[V6ListTopBar]），三列 NAS 海报 + 左下画质标 + 片名 / 年份。
 * [entries] 为 null 表示正在读索引；空列表给出可见说明，不留白屏。
 */
@Composable
internal fun NasCategoryScreen(title: String, entries: List<HomeEntry>?, onBack: () -> Unit, onOpen: (HomeEntry) -> Unit) {
    val top = WindowInsets.statusBars.asPaddingValues().calculateTopPadding()
    Box(Modifier.fillMaxSize().background(DesignTokens.pageBackground)) {
        LazyVerticalGrid(
            columns = GridCells.Fixed(3),
            contentPadding = PaddingValues(start = V6Metrics.screenPad, end = V6Metrics.screenPad, top = top + 68.dp, bottom = 40.dp),
            horizontalArrangement = Arrangement.spacedBy(V6Metrics.cardGap),
            verticalArrangement = Arrangement.spacedBy(20.dp),
        ) {
            items(entries.orEmpty()) { e -> NasGridPoster(e, onOpen) }
            item(span = { GridItemSpan(3) }) {
                when {
                    entries == null -> V6ListHint("正在读取片库…")
                    entries.isEmpty() -> V6ListHint("这个分类还没有影片，可在 设置 › 数据与资源 里重新扫描")
                    else -> V6ListHint("共 ${entries.size} 部")
                }
            }
        }
        V6ListTopBar(title, top, onBack)
    }
}

@Composable
private fun NasGridPoster(e: HomeEntry, onOpen: (HomeEntry) -> Unit) {
    val shape = RoundedCornerShape(V6Metrics.rPoster)
    Column(Modifier.clickable { onOpen(e) }) {
        Box(Modifier.fillMaxWidth().aspectRatio(2f / 3f).clip(shape).background(DesignTokens.posterPlaceholder)) {
            NasPoster(e, Modifier.fillMaxSize(), shape)
            qualityBadgeText(e.key.substringAfterLast('/'))?.let {
                QualityBadge(it, Modifier.align(Alignment.BottomStart).padding(6.dp))
            }
            Box(Modifier.fillMaxSize().border(1.dp, V6Metrics.strokeOnLight, shape))
        }
        Spacer(Modifier.height(8.dp))
        Text(e.title, color = DesignTokens.textMain, fontSize = V6Metrics.fsCardTitle, fontWeight = FontWeight.Medium,
            maxLines = 1, overflow = TextOverflow.Ellipsis)
        Text(e.year?.takeIf { it > 0 }?.toString() ?: e.episodeLabel.orEmpty(), color = DesignTokens.textSub,
            fontSize = V6Metrics.fsMeta, maxLines = 1)
    }
}
