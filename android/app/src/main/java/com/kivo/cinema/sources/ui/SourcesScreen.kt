package com.kivo.cinema.sources.ui

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.grid.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.kivo.cinema.sources.SourceManagerModel
import com.kivo.cinema.sources.SourceRecord

/** Source overview, not a list of protocol implementations. Local/file entries remain usable. */
@Composable
fun SourcesScreen(model: SourceManagerModel, videoCount: Int, onOpenLibrary: () -> Unit,
    onOpenSmb: () -> Unit, onOpenPrivate: () -> Unit, onOpenEmby: (String) -> Unit) {
    var management by remember { mutableStateOf<SourceRecord?>(null) }
    LaunchedEffect(Unit) { model.refresh() }
    MaterialTheme(colorScheme = lightColorScheme(primary = Color(0xFF347861),
        primaryContainer = Color(0xFFE6F0EB), onPrimaryContainer = Color(0xFF285B48),
        surface = Color(0xFFFCFBFD), onSurface = Color(0xFF24252C), onSurfaceVariant = Color(0xFF73717E))) {
        Box(Modifier.fillMaxSize(), contentAlignment = Alignment.TopCenter) {
            LazyVerticalGrid(columns = GridCells.Adaptive(280.dp),
                modifier = Modifier.widthIn(max = 1200.dp).fillMaxSize(), contentPadding = PaddingValues(20.dp),
                horizontalArrangement = Arrangement.spacedBy(16.dp), verticalArrangement = Arrangement.spacedBy(16.dp)) {
                item(span = { GridItemSpan(maxLineSpan) }) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Column(Modifier.weight(1f)) {
                            Text("资源库", style = MaterialTheme.typography.headlineLarge, fontWeight = FontWeight.Bold)
                            Text("你的影片，从这里开始", color = MaterialTheme.colorScheme.onSurfaceVariant)
                        }
                        FilledTonalButton(onClick = { model.edit() }, enabled = model.ready && !model.busy) { Text("＋ 添加") }
                    }
                }
                item { EntryCard("本地影片", "$videoCount 个视频", onOpenLibrary) }
                item { EntryCard("网络共享", "浏览 SMB 文件夹", onOpenSmb) }
                item { EntryCard("私密区", "受保护的观看空间", onOpenPrivate) }
                item(span = { GridItemSpan(maxLineSpan) }) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Text("媒体服务器", Modifier.weight(1f), style = MaterialTheme.typography.titleMedium)
                        if (model.ready) Text("${model.records.size} 个资源库", color = MaterialTheme.colorScheme.onSurfaceVariant)
                    }
                }
                if (model.loadError != null) item(span = { GridItemSpan(maxLineSpan) }) {
                    Surface(shape = RoundedCornerShape(20.dp)) {
                        Column(Modifier.padding(20.dp)) {
                            Text(model.loadError!!, color = MaterialTheme.colorScheme.error)
                            TextButton(onClick = model::refresh) { Text("重新读取") }
                        }
                    }
                } else if (!model.ready) item { CircularProgressIndicator(Modifier.padding(20.dp)) }
                else if (model.records.isEmpty()) item(span = { GridItemSpan(maxLineSpan) }) {
                    Surface(shape = RoundedCornerShape(24.dp)) {
                        Column(Modifier.padding(28.dp), verticalArrangement = Arrangement.spacedBy(12.dp)) {
                            Text("把你的电影库带进来", style = MaterialTheme.typography.titleLarge)
                            Text("添加 Emby 后，连接与账号会保存在这台设备上。", color = MaterialTheme.colorScheme.onSurfaceVariant)
                            Button(onClick = { model.edit() }) { Text("添加 Emby") }
                        }
                    }
                }
                items(model.records, key = { it.id }) { source ->
                    SourceCard(source, { onOpenEmby(source.id) }, { management = source })
                }
                if (!model.editing && model.error != null) item(span = { GridItemSpan(maxLineSpan) }) {
                    Text(model.error!!, color = MaterialTheme.colorScheme.error)
                }
            }
        }
        management?.let { source -> SourceManagementSheet(source, { management = null },
            { management = null; model.edit(source) }, { management = null; model.remove(source) }) }
        if (model.editing) EmbySourceForm(model)
    }
}

@Composable
private fun EntryCard(title: String, description: String, onClick: () -> Unit) {
    Surface(onClick = onClick, shape = RoundedCornerShape(22.dp)) {
        Row(Modifier.padding(20.dp), verticalAlignment = Alignment.CenterVertically) {
            Column(Modifier.weight(1f)) {
                Text(title, style = MaterialTheme.typography.titleMedium, fontWeight = FontWeight.SemiBold)
                Text(description, style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
            }
            Text("›", Modifier.padding(start = 12.dp), style = MaterialTheme.typography.headlineSmall)
        }
    }
}
