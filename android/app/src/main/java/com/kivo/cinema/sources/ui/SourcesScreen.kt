package com.kivo.cinema.sources.ui

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.grid.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.repeatOnLifecycle
import kotlinx.coroutines.delay
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.sources.SourceManagerModel
import com.kivo.cinema.sources.SourceRecord

/** Source overview, not a list of protocol implementations. Local/file entries remain usable. */
@Composable
fun SourcesScreen(model: SourceManagerModel, videoCount: Int, onOpenLibrary: () -> Unit,
    onOpenSmb: () -> Unit, onOpenPrivate: () -> Unit, onOpenEmby: (String) -> Unit) {
    var management by remember { mutableStateOf<SourceRecord?>(null) }
    var choosingType by remember { mutableStateOf(false) }
    LaunchedEffect(Unit) { model.refresh() }
    SourcesTheme {
        val lifecycle = LocalLifecycleOwner.current.lifecycle
        val minimumCardWidth = 160.dp * LocalDensity.current.fontScale.coerceAtLeast(1f)
        Box(Modifier.fillMaxSize(), contentAlignment = Alignment.TopCenter) {
            LazyVerticalGrid(columns = GridCells.Adaptive(minimumCardWidth),
                modifier = Modifier.widthIn(max = 1200.dp).fillMaxSize(),
                contentPadding = PaddingValues(horizontal = DesignTokens.screenPad.dp, vertical = 8.dp),
                horizontalArrangement = Arrangement.spacedBy(12.dp), verticalArrangement = Arrangement.spacedBy(12.dp)) {
                item(span = { GridItemSpan(maxLineSpan) }) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Column(Modifier.weight(1f)) {
                            Text("资源库", fontSize = 28.sp, fontWeight = FontWeight.Bold)
                            Text("连接你的私人影院", fontSize = 12.sp, color = MaterialTheme.colorScheme.onSurfaceVariant,
                                modifier = Modifier.padding(top = 4.dp))
                        }
                        FilledTonalButton(onClick = { choosingType = true }, enabled = model.ready && !model.busy,
                            contentPadding = PaddingValues(horizontal = 16.dp, vertical = 12.dp)) { Text("＋ 添加") }
                    }
                }
                item(span = { GridItemSpan(maxLineSpan) }) {
                    Surface(shape = RoundedCornerShape(DesignTokens.radiusCard.dp), color = Color.White.copy(alpha = .76f)) {
                        Column {
                            EntryCard("本地影片", "$videoCount 个视频", onOpenLibrary)
                            HorizontalDivider(Modifier.padding(start = 16.dp), color = DesignTokens.hairline)
                            EntryCard("网络共享", "SMB", onOpenSmb)
                            HorizontalDivider(Modifier.padding(start = 16.dp), color = DesignTokens.hairline)
                            EntryCard("私密区", "", onOpenPrivate)
                        }
                    }
                }
                item(span = { GridItemSpan(maxLineSpan) }) {
                    Row(Modifier.padding(top = 12.dp, bottom = 2.dp), verticalAlignment = Alignment.CenterVertically) {
                        Text("媒体服务器", Modifier.weight(1f), fontSize = 16.sp, fontWeight = FontWeight.Medium,
                            color = MaterialTheme.colorScheme.onSurfaceVariant)
                        if (model.ready) Text("${model.records.size} 个", fontSize = 12.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
                        IconButton(onClick = { model.records.forEach { model.summaries.request(it, force = true) } },
                            enabled = model.ready) { Text("↻", fontSize = 22.sp,
                                modifier = Modifier.semantics { contentDescription = "刷新服务器状态与媒体数量" }) }
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
                            Button(onClick = { choosingType = true }) { Text("添加资源库") }
                        }
                    }
                }
                items(model.records, key = { it.id }) { source ->
                    LaunchedEffect(source.id, source.revision, lifecycle) {
                        lifecycle.repeatOnLifecycle(Lifecycle.State.STARTED) {
                            while (true) {
                                model.summaries.request(source, force = true)
                                delay(60_000)
                            }
                        }
                    }
                    SourceCard(source, { onOpenEmby(source.id) }, { management = source },
                        model.summaries.values[source.id]?.takeIf { it.revision == source.revision })
                }
                if (!model.editing && model.error != null) item(span = { GridItemSpan(maxLineSpan) }) {
                    Text(model.error!!, color = MaterialTheme.colorScheme.error)
                }
            }
        }
        management?.let { source -> SourceManagementSheet(source, { management = null },
            { management = null; model.edit(source) }, { management = null; model.remove(source) }) }
        if (model.editing) EmbySourceForm(model)
        if (choosingType) SourceTypeSheet({ choosingType = false },
            { choosingType = false; model.edit() }, { choosingType = false; onOpenSmb() })
    }
}

@Composable
private fun EntryCard(title: String, description: String, onClick: () -> Unit) {
    Surface(onClick = onClick, color = Color.Transparent) {
        Row(Modifier.heightIn(min = 59.dp).padding(horizontal = 16.dp, vertical = 12.dp), verticalAlignment = Alignment.CenterVertically) {
            Text(title, Modifier.weight(1f), fontSize = 17.sp, fontWeight = FontWeight.Medium)
            if (description.isNotEmpty()) Text(description, fontSize = 12.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
            Text("›", Modifier.padding(start = 12.dp), fontSize = 22.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
        }
    }
}
