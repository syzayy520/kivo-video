package com.kivo.cinema.sources.ui

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.kivo.cinema.sources.SourceRecord

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SourceManagementSheet(record: SourceRecord, onDismiss: () -> Unit, onEdit: () -> Unit, onDelete: () -> Unit) {
    var confirming by remember { mutableStateOf(false) }
    ModalBottomSheet(onDismissRequest = onDismiss, sheetMaxWidth = 560.dp,
        containerColor = MaterialTheme.colorScheme.surfaceContainerLow) {
        Column(Modifier.fillMaxWidth().verticalScroll(rememberScrollState())
            .padding(horizontal = 20.dp).padding(bottom = 24.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)) {
            Text(record.name, style = MaterialTheme.typography.headlineSmall)
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
                    HorizontalDivider(Modifier.padding(horizontal = 16.dp), color = MaterialTheme.colorScheme.outlineVariant)
                    TextButton(onClick = { confirming = true }, shape = MaterialTheme.shapes.medium,
                        modifier = Modifier.fillMaxWidth().heightIn(min = 56.dp)) {
                        Text("移除资源库", Modifier.fillMaxWidth().padding(horizontal = 8.dp),
                            color = MaterialTheme.colorScheme.error)
                    }
                }
            }
            TextButton(onClick = onDismiss, modifier = Modifier.fillMaxWidth().heightIn(min = 48.dp)) { Text("取消") }
        }
    }
    if (confirming) AlertDialog(onDismissRequest = { confirming = false },
        title = { Text("移除“${record.name}”？") },
        text = { Text("移除本机保存的连接与账号。服务器上的影片会保留。") },
        confirmButton = { TextButton(onClick = onDelete) { Text("移除", color = MaterialTheme.colorScheme.error) } },
        dismissButton = { TextButton(onClick = { confirming = false }) { Text("取消") } })
}
