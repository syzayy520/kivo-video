package com.kivo.cinema.sources.ui

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.kivo.cinema.sources.SourceRecord
import com.kivo.cinema.sources.SourceRepository.Control

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SourceManagementSheet(record: SourceRecord, onDismiss: () -> Unit, onEdit: () -> Unit, onDelete: () -> Unit,
    onControl: (Control, Boolean) -> Unit) {
    var confirming by remember { mutableStateOf(false) }
    ModalBottomSheet(onDismissRequest = onDismiss, sheetMaxWidth = 560.dp,
        sheetState = rememberModalBottomSheetState(skipPartiallyExpanded = true),
        containerColor = MaterialTheme.colorScheme.surfaceContainerLow) {
        Column(Modifier.fillMaxWidth().verticalScroll(rememberScrollState())
            .padding(horizontal = 20.dp).padding(bottom = 24.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)) {
            Text(record.name, style = MaterialTheme.typography.titleMedium)
            Text(record.address, style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant)
            Text("账号 · ${record.username}", style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant)
            Surface(shape = MaterialTheme.shapes.medium) {
                Column {
                    TextButton(onClick = onEdit, shape = MaterialTheme.shapes.medium,
                        modifier = Modifier.fillMaxWidth().heightIn(min = 56.dp)) {
                        Text("编辑连接", Modifier.fillMaxWidth().padding(horizontal = 8.dp))
                    }
                    ManagementAction(if (record.defaultHome) "取消默认首页" else "设为默认首页") {
                        onControl(Control.DEFAULT_HOME, !record.defaultHome)
                    }
                    ManagementAction(if (record.aggregateEnabled) "禁用首页聚合" else "启用首页聚合") {
                        onControl(Control.AGGREGATE, !record.aggregateEnabled)
                    }
                    ManagementAction(if (record.aggregateSearchEnabled) "禁用聚合搜索" else "启用聚合搜索") {
                        onControl(Control.AGGREGATE_SEARCH, !record.aggregateSearchEnabled)
                    }
                    ManagementAction(if (record.hidden) "取消隐藏" else "隐藏并加锁") {
                        onControl(Control.HIDDEN, !record.hidden)
                    }
                    HorizontalDivider(Modifier.padding(horizontal = 16.dp), color = MaterialTheme.colorScheme.outlineVariant)
                    TextButton(onClick = { confirming = true }, shape = MaterialTheme.shapes.medium,
                        modifier = Modifier.fillMaxWidth().heightIn(min = 56.dp)) {
                        Text("移除资源库", Modifier.fillMaxWidth().padding(horizontal = 8.dp),
                            color = MaterialTheme.colorScheme.error)
                    }
                }
            }
            Text("隐藏后，通过资源库右上角的锁验证身份后显示。退到后台将重新锁定。",
                style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
            Text("聚合偏好已独立保存，将在首页聚合与跨服务器搜索接入后生效。服务器内搜索不受影响。",
                style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
            TextButton(onClick = onDismiss, modifier = Modifier.fillMaxWidth().heightIn(min = 48.dp)) { Text("取消") }
        }
    }
    if (confirming) AlertDialog(onDismissRequest = { confirming = false },
        title = { Text("移除“${record.name}”？") },
        text = { Text("移除本机保存的连接与账号。服务器上的影片会保留。") },
        confirmButton = { TextButton(onClick = onDelete) { Text("移除", color = MaterialTheme.colorScheme.error) } },
        dismissButton = { TextButton(onClick = { confirming = false }) { Text("取消") } })
}

@Composable
private fun ManagementAction(title: String, onClick: () -> Unit) {
    HorizontalDivider(Modifier.padding(horizontal = 16.dp), color = MaterialTheme.colorScheme.outlineVariant)
    TextButton(onClick = onClick, shape = MaterialTheme.shapes.medium,
        modifier = Modifier.fillMaxWidth().heightIn(min = 48.dp)) {
        Text(title, Modifier.fillMaxWidth().padding(horizontal = 8.dp), style = MaterialTheme.typography.bodyMedium)
    }
}
