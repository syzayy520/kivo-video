package com.kivo.cinema.emby.aggregate.ui

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.grid.*
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.emby.aggregate.AggregateSource
import com.kivo.cinema.emby.aggregate.GenreWallData

/**
 * 256 类型墙屏：24 渐变类型卡网格 + "— 已经到底了 —" 脚注。手机两列、平板自适应多列。
 * 单一职责：类型墙布局；单卡在 GenreCard，数据在 GenreWallData。点卡回搜索并预选该 genre。
 */
@Composable
fun GenreWallScreen(sources: List<AggregateSource>, onGenre: (String) -> Unit) {
    BoxWithConstraints(Modifier.fillMaxSize()) {
        val columns = if (maxWidth >= 700.dp) GridCells.Adaptive(240.dp) else GridCells.Fixed(2)
        LazyVerticalGrid(columns = columns, modifier = Modifier.fillMaxSize(),
            contentPadding = PaddingValues(16.dp),
            horizontalArrangement = Arrangement.spacedBy(12.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)) {
            items(GenreWallData.wall, key = { it.name }) { genre ->
                GenreCard(genre, sources.firstOrNull()) { onGenre(genre.name) }
            }
            item(span = { GridItemSpan(maxLineSpan) }) {
                Box(Modifier.fillMaxWidth().padding(20.dp), contentAlignment = Alignment.Center) {
                    Text("— 已经到底了 —", fontSize = 12.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
                }
            }
        }
    }
}