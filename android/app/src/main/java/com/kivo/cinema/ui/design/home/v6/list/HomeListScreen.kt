package com.kivo.cinema.ui.design.home.v6.list

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.asPaddingValues
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.GridItemSpan
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.lazy.grid.rememberLazyGridState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.foundation.Canvas
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.tmdb.TmdbHome
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.TmdbImage
import com.kivo.cinema.ui.design.home.sections.TmdbErrorCard
import com.kivo.cinema.ui.design.home.v6.V6Metrics

/**
 * 「查看全部」视图：玻璃返回钮 + 居中标题（浮在内容上）+ 三列海报网格；滚到倒数两行自动拉下一页。
 * 加载中 / 失败（五类原文 + 重试）/ 到底 / 空 四种状态都有可见文案，不留白屏。
 */
@Composable
internal fun HomeListScreen(title: String, pager: HomeListPager, onBack: () -> Unit, onOpen: (TmdbHome.Item) -> Unit) {
    val grid = rememberLazyGridState()
    val top = WindowInsets.statusBars.asPaddingValues().calculateTopPadding()
    val nearEnd by remember {
        derivedStateOf { (grid.layoutInfo.visibleItemsInfo.lastOrNull()?.index ?: 0) >= pager.items.size - 6 }
    }
    LaunchedEffect(Unit) { pager.loadNext() }
    LaunchedEffect(nearEnd, pager.items.size) { if (nearEnd && pager.items.isNotEmpty()) pager.loadNext() }

    Box(Modifier.fillMaxSize().background(DesignTokens.pageBackground)) {
        LazyVerticalGrid(
            columns = GridCells.Fixed(3),
            state = grid,
            contentPadding = PaddingValues(start = V6Metrics.screenPad, end = V6Metrics.screenPad, top = top + 68.dp, bottom = 40.dp),
            horizontalArrangement = Arrangement.spacedBy(V6Metrics.cardGap),
            verticalArrangement = Arrangement.spacedBy(20.dp),
        ) {
            items(pager.items, key = { it.id }) { m -> GridPoster(m, onOpen) }
            item(span = { GridItemSpan(3) }) { Footer(pager) }
        }
        V6ListTopBar(title, top, onBack)
    }
}

@Composable
private fun GridPoster(m: TmdbHome.Item, onOpen: (TmdbHome.Item) -> Unit) {
    val shape = RoundedCornerShape(V6Metrics.rPoster)
    Column(Modifier.clickable { onOpen(m) }) {
        Box(Modifier.fillMaxWidth().aspectRatio(2f / 3f).clip(shape)) {
            TmdbImage(m.posterUrl, m.title, Modifier.fillMaxSize())
            Box(Modifier.fillMaxSize().border(1.dp, V6Metrics.strokeOnLight, shape))
        }
        Spacer(Modifier.height(8.dp))
        Text(m.title, color = DesignTokens.textMain, fontSize = V6Metrics.fsCardTitle, fontWeight = FontWeight.Medium,
            maxLines = 1, overflow = TextOverflow.Ellipsis)
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("★", color = V6Metrics.brass, fontSize = 10.sp)
            Text(" %.1f · %s".format(m.rating, m.year()), color = DesignTokens.textSub, fontSize = V6Metrics.fsMeta, maxLines = 1)
        }
    }
}

@Composable
private fun Footer(pager: HomeListPager) {
    val error = pager.error
    when {
        error != null -> TmdbErrorCard(error) { pager.loadNext() }
        pager.loading -> V6ListHint(if (pager.items.isEmpty()) "正在加载…" else "正在加载更多…")
        pager.reachedEnd && pager.items.isEmpty() -> V6ListHint("这个榜单暂时没有内容")
        pager.reachedEnd -> V6ListHint("已显示全部 ${pager.items.size} 部")
    }
}
