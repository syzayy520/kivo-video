package com.kivo.cinema.emby.aggregate.ui

import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.grid.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Search
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.emby.aggregate.AggregateFilter
import com.kivo.cinema.emby.aggregate.AggregateSearchModel
import com.kivo.cinema.emby.aggregate.AggregateSource
import com.kivo.cinema.emby.aggregate.rememberLocalMatches
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.GlassSurface
import kotlinx.coroutines.launch

/**
 * 256 全局搜索屏：固定区（播放器同款玻璃搜索栏 + 状态行 + 手机折叠摘要）不随结果滚动；
 * 六排筛选是网格首项（窄屏单行左滑/宽屏换行），随结果滚走、摘要常驻可折叠。
 * 单一职责：搜索页布局与状态呈现；查询/合并在模型，本地匹配在 LocalSearch，chip 在 AggregateFilterRows。
 */
@Composable
fun AggregateSearchScreen(
    model: AggregateSearchModel, sources: List<AggregateSource>,
    onItem: (AggregateSearchModel.Result) -> Unit,
) {
    BoxWithConstraints(Modifier.fillMaxSize()) {
        val wide = maxWidth >= 700.dp
        val minCard = if (wide) 160.dp else 128.dp
        val local = rememberLocalMatches(model.query)
        var expanded by rememberSaveable { mutableStateOf(false) }
        val scope = rememberCoroutineScope()
        val grid = rememberLazyGridState()
        val nearEnd by remember {
            derivedStateOf { (grid.layoutInfo.visibleItemsInfo.lastOrNull()?.index ?: -1) >= model.results.size - 12 }
        }
        LaunchedEffect(nearEnd, model.results.size) {
            if (nearEnd && model.error == null) model.loadMore()
        }
        Column(Modifier.fillMaxSize()) {
            Column(Modifier.fillMaxWidth().padding(horizontal = 16.dp)) {
                SearchBar(model)
                val empty = model.results.isEmpty() && local.items.isEmpty()
                val status = when {
                    model.error != null -> model.error!!
                    model.loading && empty -> "正在搜索…"
                    empty -> "没有找到匹配内容，试试其他片名或放宽筛选。"
                    else -> "${model.results.size + local.items.size} 个结果"
                }
                Text(status, fontSize = 13.sp,
                    color = if (model.error != null) MaterialTheme.colorScheme.error
                            else MaterialTheme.colorScheme.onSurfaceVariant,
                    modifier = Modifier.padding(top = 6.dp, start = 4.dp))
                if (!wide) FilterSummary(model, sources, expanded) {
                    expanded = !expanded
                    // 网格按 key 锚定滚动：顶部插入筛选排后须回顶，否则新排落在视口外
                    if (expanded) scope.launch { grid.scrollToItem(0) }
                }
            }
            LazyVerticalGrid(columns = GridCells.Adaptive(minCard), state = grid,
                modifier = Modifier.weight(1f),
                contentPadding = PaddingValues(start = 16.dp, end = 16.dp, top = 8.dp, bottom = 24.dp),
                horizontalArrangement = Arrangement.spacedBy(14.dp),
                verticalArrangement = Arrangement.spacedBy(18.dp)) {
                if (wide || expanded) {
                    item(span = { GridItemSpan(maxLineSpan) }) {
                        AggregateFilterRows(model, sources, wide)
                    }
                }
                model.partialFailure?.let { reason ->
                    item(span = { GridItemSpan(maxLineSpan) }) {
                        Text("部分资源库未接入：$reason", fontSize = 12.sp,
                            color = MaterialTheme.colorScheme.error,
                            modifier = Modifier.padding(vertical = 4.dp))
                    }
                }
                if (model.query.isNotBlank()) {
                    item(span = { GridItemSpan(maxLineSpan) }) {
                        Text(local.denied ?: "本地 · ${local.items.size} 个匹配",
                            fontSize = 14.sp, fontWeight = FontWeight.Bold,
                            color = if (local.denied != null) MaterialTheme.colorScheme.error
                                    else MaterialTheme.colorScheme.onSurface,
                            modifier = Modifier.padding(top = 4.dp))
                    }
                    items(local.items, key = { "local:" + it.id }) { item ->
                        LocalResultCard(item)
                    }
                }
                items(model.results, key = { it.source.id + ":" + it.item.id }) { result ->
                    AggregateResultCard(result) { onItem(result) }
                }
                item(span = { GridItemSpan(maxLineSpan) }) {
                    Column(Modifier.fillMaxWidth().padding(16.dp), horizontalAlignment = Alignment.CenterHorizontally) {
                        when {
                            model.loading -> CircularProgressIndicator(Modifier.size(24.dp), strokeWidth = 2.dp)
                            model.error != null -> {
                                Text(model.error!!, color = MaterialTheme.colorScheme.error, fontSize = 14.sp)
                                TextButton(onClick = model::loadMore) { Text("重试") }
                            }
                            !model.hasMore && model.results.isNotEmpty() -> Text("— 已经到底了 —", fontSize = 12.sp,
                                color = MaterialTheme.colorScheme.onSurfaceVariant)
                        }
                    }
                }
            }
        }
    }
}

/** 固定搜索栏：播放器同款 GlassSurface 白玻璃 + hairline 描边，容器透明让玻璃透出来。 */
@Composable
private fun SearchBar(model: AggregateSearchModel) {
    GlassSurface(
        modifier = Modifier.fillMaxWidth()
            .border(1.dp, DesignTokens.hairline, RoundedCornerShape(DesignTokens.radiusCard.dp)),
        radius = DesignTokens.radiusCard,
    ) {
        TextField(value = model.query, onValueChange = model::search,
            placeholder = { Text("输入影片名称搜索", fontSize = 15.sp, color = DesignTokens.textSub) },
            singleLine = true,
            textStyle = TextStyle(fontSize = 15.sp, color = DesignTokens.textMain),
            leadingIcon = { Icon(Icons.Filled.Search, null, tint = DesignTokens.textSub,
                modifier = Modifier.size(20.dp)) },
            trailingIcon = {
                if (model.query.isNotEmpty()) TextButton(onClick = { model.search("") }) {
                    Text("清除", fontSize = 13.sp)
                }
            },
            colors = TextFieldDefaults.colors(
                focusedContainerColor = Color.Transparent,
                unfocusedContainerColor = Color.Transparent,
                focusedIndicatorColor = Color.Transparent,
                unfocusedIndicatorColor = Color.Transparent,
                cursorColor = MaterialTheme.colorScheme.primary),
            modifier = Modifier.fillMaxWidth())
    }
}

/** 折叠摘要：动态拼当前筛选（媒体库·排序·类别[·类型·地区·年份]），点按展开/收回六排。 */
@Composable
private fun FilterSummary(
    model: AggregateSearchModel, sources: List<AggregateSource>,
    expanded: Boolean, onToggle: () -> Unit,
) {
    val parts = buildList {
        add(if (model.selectedIds.size == sources.size) "媒体库" else "${model.selectedIds.size} 个媒体库")
        add(AggregateFilter.sortings[model.sortingIndex].label)
        add(AggregateFilter.categories[model.categoryIndex].label)
        model.genre?.let { add(it) }
        model.region?.let { add(it) }
        model.yearIndex?.let { add(AggregateFilter.yearOptions[it].label) }
    }
    Row(Modifier.fillMaxWidth().clickable(onClick = onToggle).padding(vertical = 8.dp),
        horizontalArrangement = Arrangement.Center, verticalAlignment = Alignment.CenterVertically) {
        Text(parts.joinToString(" · "), fontSize = 13.sp, fontWeight = FontWeight.Medium,
            color = MaterialTheme.colorScheme.primary, maxLines = 1, overflow = TextOverflow.Ellipsis,
            modifier = Modifier.weight(1f, fill = false))
        Spacer(Modifier.width(4.dp))
        Text(if (expanded) "▲" else "▼", fontSize = 10.sp, color = MaterialTheme.colorScheme.primary)
    }
}