package com.kivo.cinema.emby.ui

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.grid.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.emby.EmbyItem
import com.kivo.cinema.emby.EmbySessionModel

@Composable
fun EmbyLibraryScreen(
    model: EmbySessionModel, onBack: () -> Unit,
    title: String, series: Boolean, onItem: (EmbyItem) -> Unit,
) {
    val noun = if (series) "剧集" else "电影"
    var sorting by remember { mutableStateOf(false) }
    val library = model.library
    val list = library.items
    Column(Modifier.fillMaxSize()) {
        Row(Modifier.fillMaxWidth().padding(horizontal = 12.dp, vertical = 4.dp),
            verticalAlignment = Alignment.CenterVertically) {
            TextButton(onClick = onBack) { Text("‹  返回", fontSize = 16.sp) }
            Spacer(Modifier.weight(1f))
        }
        Column(Modifier.padding(horizontal = 24.dp)) {
            Text(title, fontSize = 30.sp, fontWeight = FontWeight.Bold)
            Text("${model.serverName} · 已加载 ${library.items.size} 部${library.total?.let { " / 共 $it 部" } ?: ""}", fontSize = 13.sp,
                color = MaterialTheme.colorScheme.onSurfaceVariant, modifier = Modifier.padding(top = 6.dp))
            Spacer(Modifier.height(20.dp))
            OutlinedTextField(value = library.query, onValueChange = { library.search(it) },
                placeholder = { Text("搜索服务器中的$noun") }, singleLine = true,
                shape = RoundedCornerShape(16.dp), modifier = Modifier.fillMaxWidth(),
                trailingIcon = { if (library.query.isNotEmpty()) TextButton(onClick = { library.search("") }) { Text("清除") } })
            Row(Modifier.fillMaxWidth().padding(vertical = 8.dp), verticalAlignment = Alignment.CenterVertically) {
                Text(if (library.query.isBlank()) "全部$noun" else library.total?.let { "$it 个搜索结果" } ?: "搜索结果",
                    fontSize = 14.sp, color = MaterialTheme.colorScheme.onSurfaceVariant,
                    modifier = Modifier.weight(1f))
                Box {
                    TextButton(onClick = { sorting = true }) { Text(listOf("最近添加", "名称", "年份")[library.sort] + " ▾") }
                    DropdownMenu(expanded = sorting, onDismissRequest = { sorting = false }) {
                        listOf("最近添加", "名称", "年份").forEachIndexed { index, name ->
                            DropdownMenuItem(text = { Text(if (library.sort == index) "✓  $name" else name) },
                                onClick = { library.sortBy(index); sorting = false })
                        }
                    }
                }
            }
        }
        if (list.isEmpty()) Box(Modifier.weight(1f).fillMaxWidth().padding(32.dp), contentAlignment = Alignment.Center) {
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Text(if (library.loading) "正在搜索$noun…" else if (library.error != null) "暂时无法加载" else if (library.query.isBlank()) "这里还没有$noun" else "没有找到匹配的$noun", fontSize = 19.sp, fontWeight = FontWeight.SemiBold)
                Spacer(Modifier.height(10.dp))
                Text(library.error ?: if (library.loading) "正在从服务器获取结果。" else if (library.query.isBlank()) "可以检查服务器里的${noun}库和账号权限。" else "试试其他片名。",
                    color = MaterialTheme.colorScheme.onSurfaceVariant, fontSize = 14.sp)
                if (library.error != null) TextButton(onClick = { library.loadMore() }) { Text("重试") }
                if (library.query.isNotBlank()) TextButton(onClick = { library.search("") }) { Text("清除搜索") }
            }
        } else key(library.sort, library.query) {
            val grid = rememberLazyGridState()
            val nearEnd by remember { derivedStateOf {
                (grid.layoutInfo.visibleItemsInfo.lastOrNull()?.index ?: -1) >= list.size - 12
            } }
            LaunchedEffect(nearEnd, list.size) {
                if (nearEnd && library.error == null) library.loadMore()
            }
            LazyVerticalGrid(state = grid,columns = GridCells.Adaptive(128.dp), modifier = Modifier.weight(1f),
                contentPadding = PaddingValues(start = 24.dp, end = 24.dp, bottom = 24.dp),
                horizontalArrangement = Arrangement.spacedBy(16.dp), verticalArrangement = Arrangement.spacedBy(22.dp)) {
                items(list, key = { it.id }) { item -> EmbyPosterCard(model, item) { onItem(item) } }
                item(span = { GridItemSpan(maxLineSpan) }) {
                    Column(Modifier.fillMaxWidth().padding(12.dp), horizontalAlignment = Alignment.CenterHorizontally) {
                        when {
                            library.loading -> CircularProgressIndicator(Modifier.size(24.dp), strokeWidth = 2.dp)
                            library.error != null -> {
                                Text(library.error!!, color = MaterialTheme.colorScheme.error)
                                TextButton(onClick = { library.loadMore() }) { Text("重试加载") }
                            }
                            !library.hasMore -> Text("已显示全部$noun", color = MaterialTheme.colorScheme.onSurfaceVariant)
                        }
                    }
                }
            }
        }
    }
}
