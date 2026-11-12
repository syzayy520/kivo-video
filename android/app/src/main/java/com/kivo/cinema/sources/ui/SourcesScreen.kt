package com.kivo.cinema.sources.ui

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.clickable
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
import com.kivo.cinema.sources.SourceRepository.Control
import com.kivo.cinema.sources.access.SourceAccessSession
import androidx.compose.ui.res.painterResource
import com.kivo.cinema.R
import androidx.activity.compose.BackHandler
import androidx.compose.runtime.saveable.rememberSaveable
import com.kivo.cinema.sources.ui.cards.*
import com.kivo.cinema.sources.ui.workspace.*

/** Source overview, not a list of protocol implementations. Local/file entries remain usable. */
@Composable
fun SourcesScreen(model: SourceManagerModel, videoCount: Int?, onOpenLibrary: () -> Unit,
    onOpenPrivate: () -> Unit, onOpenSource: (SourceRecord) -> Unit,
    onUnlock: (() -> Unit) -> Unit,
    onOpenAggregate: () -> Unit = {},
    navigation: @Composable () -> Unit = {},
    overviewModifier: Modifier = Modifier,
    onNavigationGuard: (((() -> Unit) -> Unit)?) -> Unit = {}) {
    val workspace = rememberSaveable(saver = SourceWorkspaceState.Saver) { SourceWorkspaceState() }
    val grid = rememberLazyGridState()
    LaunchedEffect(Unit) { model.refresh() }
    val visible = model.visibleRecords
    val source = visible.firstOrNull { it.id == workspace.sourceId }
    val editingVisible = model.editing && model.draftVisible
    val panelOpen = editingVisible || workspace.choosingType || source != null
    fun navigate(action: () -> Unit) = workspace.navigate(model, action)
    fun close() = navigate { workspace.clear() }
    DisposableEffect(workspace, model) {
        onNavigationGuard { action -> workspace.navigate(model, action) }
        onDispose { onNavigationGuard(null) }
    }
    // Closing an editor returns to its management page; a second back closes that page.
    val dismissEditor = { navigate { workspace.choosingType = false } }
    BackHandler(panelOpen) {
        if (editingVisible) dismissEditor() else close()
    }
    LaunchedEffect(SourceAccessSession.unlocked) {
        if (!SourceAccessSession.unlocked) {
            val hiddenId = model.records.firstOrNull { it.id == workspace.sourceId }?.hidden == true
            if (hiddenId) workspace.clear()
            if (!model.draftVisible) { workspace.keepEditing(); model.dismiss() }
        }
    }
    SourcesTheme {
        val lifecycle = LocalLifecycleOwner.current.lifecycle
        val minimumCardWidth = 160.dp * LocalDensity.current.fontScale.coerceAtLeast(1f)
        SourceWorkspace(panelOpen, navigation = navigation, overview = {
        Box(Modifier.fillMaxSize().then(overviewModifier), contentAlignment = Alignment.TopCenter) {
            LazyVerticalGrid(columns = GridCells.Adaptive(minimumCardWidth), state = grid,
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
                        IconButton(onClick = {
                            if (SourceAccessSession.unlocked) SourceAccessSession.lock() else onUnlock({})
                        }) { Icon(painterResource(R.drawable.source_lock), if (SourceAccessSession.unlocked) "锁定隐藏资源库" else "解锁隐藏资源库",
                            tint = if (SourceAccessSession.unlocked) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.onSurfaceVariant) }
                        FilledTonalButton(onClick = { navigate { workspace.sourceId = null; workspace.choosingType = true } }, enabled = model.ready && !model.busy,
                            contentPadding = PaddingValues(horizontal = 16.dp, vertical = 12.dp)) { Text("＋ 添加") }
                    }
                }
                item(span = { GridItemSpan(maxLineSpan) }, key = "storage-heading") {
                    Text("本地与存储", fontSize = 13.sp, color = MaterialTheme.colorScheme.onSurfaceVariant,
                        modifier = Modifier.padding(top = 10.dp, bottom = 2.dp))
                }
                item(key = "local") { StorageEntryCard("本地影片", videoCount?.let { "$it 个视频" } ?: "查看本机影片", SourceIconKind.LOCAL) { navigate(onOpenLibrary) } }
                item(key = "private") { StorageEntryCard("私密区", if (SourceAccessSession.unlocked) "已解锁" else "数字密码保护", SourceIconKind.LOCK) { navigate(onOpenPrivate) } }
                // 243：已保存的 SMB 源属于"本地与存储"，与"媒体服务器"分开。
                items(visible.filter { it.isSmb }, key = { it.id }) { source ->
                    SourceCard(source, { navigate { onOpenSource(source) } },
                        { navigate { workspace.sourceId = source.id; workspace.choosingType = false } })
                }
                item(span = { GridItemSpan(maxLineSpan) }, key = "aggregate-entry") {
                    Surface(shape = RoundedCornerShape(20.dp), color = Color(0xFF238A59),
                        modifier = Modifier.fillMaxWidth().padding(top = 12.dp)
                            .clickable { navigate(onOpenAggregate) }) {
                        Row(Modifier.padding(horizontal = 20.dp, vertical = 16.dp),
                            verticalAlignment = Alignment.CenterVertically) {
                            Column(Modifier.weight(1f)) {
                                Text("聚合搜索", fontSize = 17.sp, fontWeight = FontWeight.Bold, color = Color.White)
                                Text("跨所有开启聚合搜索的服务器搜片 · 类型墙", fontSize = 12.sp,
                                    color = Color.White.copy(alpha = 0.85f), modifier = Modifier.padding(top = 2.dp))
                            }
                            Text("›", fontSize = 22.sp, color = Color.White)
                        }
                    }
                }
                item(span = { GridItemSpan(maxLineSpan) }) {
                    Row(Modifier.padding(top = 12.dp, bottom = 2.dp), verticalAlignment = Alignment.CenterVertically) {
                        Text("媒体服务器", Modifier.weight(1f), fontSize = 16.sp, fontWeight = FontWeight.Medium,
                            color = MaterialTheme.colorScheme.onSurfaceVariant)
                        val servers = visible.filter { !it.isSmb }
                        if (model.ready) Text("${servers.size} 个", fontSize = 12.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
                        IconButton(onClick = { servers.forEach { model.summaries.request(it, force = true) } },
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
                else if (visible.none { !it.isSmb }) item(span = { GridItemSpan(maxLineSpan) }) {
                    Surface(shape = RoundedCornerShape(24.dp)) {
                        Column(Modifier.padding(28.dp), verticalArrangement = Arrangement.spacedBy(12.dp)) {
                            Text("把你的电影库带进来", style = MaterialTheme.typography.titleLarge)
                            Text("添加 Emby 后，连接与账号会保存在这台设备上。", color = MaterialTheme.colorScheme.onSurfaceVariant)
                            Button(onClick = { navigate { workspace.sourceId = null; workspace.choosingType = true } }) { Text("添加资源库") }
                        }
                    }
                }
                items(visible.filter { !it.isSmb }, key = { it.id }) { source ->
                    LaunchedEffect(source.id, source.revision, lifecycle) {
                        lifecycle.repeatOnLifecycle(Lifecycle.State.STARTED) {
                            while (true) {
                                model.summaries.request(source, force = true)
                                delay(60_000)
                            }
                        }
                    }
                    SourceCard(source, { navigate { onOpenSource(source) } }, { navigate { workspace.sourceId = source.id; workspace.choosingType = false } },
                        model.summaries.values[source.id]?.takeIf { it.revision == source.revision })
                }
                if (!model.editing && model.error != null) item(span = { GridItemSpan(maxLineSpan) }) {
                    Text(model.error!!, color = MaterialTheme.colorScheme.error)
                }
            }
        }
        }, panel = { wide ->
            val dismiss = { if (editingVisible) dismissEditor() else close() }
            val edit = { model.edit(source) }
            val remove = { source?.let(model::remove) ?: Unit }
            val control: (Control, Boolean) -> Unit = { kind, enabled ->
                source?.let { record ->
                    if (kind == Control.HIDDEN && enabled) onUnlock { model.setControl(record, kind, enabled) }
                    else model.setControl(record, kind, enabled)
                }
            }
            val emby = { workspace.choosingType = false; model.edit() }
            val smb = { workspace.choosingType = false; model.startNew(SourceRecord.PROTOCOL_SMB) }
            when {
                editingVisible -> if (wide) key(model.editingId) { SourceConnectionContent(model, dismiss) }
                    else EmbySourceForm(model, dismiss)
                workspace.choosingType -> if (wide) SourceTypeContent(dismiss, emby, smb)
                    else SourceTypeSheet(dismiss, emby, smb)
                source != null -> if (wide) key(source.id) {
                    SourceManagementContent(source, dismiss, edit, remove, control, model.busy, model.error)
                } else SourceManagementSheet(source, dismiss, edit, remove, control, model.busy, model.error)
            }
        })
        if (workspace.pending != null) AlertDialog(onDismissRequest = workspace::keepEditing,
            title = { Text(if (model.verifying) "取消连接并离开？" else "放弃未保存的修改？") },
            text = { Text("当前内容尚未保存。") },
            confirmButton = { TextButton(onClick = { workspace.discard(model) }) { Text("放弃修改") } },
            dismissButton = { TextButton(onClick = workspace::keepEditing) { Text("继续编辑") } })
    }
}
