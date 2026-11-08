package com.kivo.cinema.sources.ui

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.kivo.cinema.sources.SourceRecord

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SourceManagementSheet(record: SourceRecord, onDismiss: () -> Unit, onEdit: () -> Unit, onDelete: () -> Unit) {
    var confirming by remember { mutableStateOf(false) }
    ModalBottomSheet(onDismissRequest = onDismiss, sheetMaxWidth = 560.dp) {
        Column(Modifier.fillMaxWidth().padding(horizontal = 24.dp).padding(bottom = 28.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)) {
            Text(record.name, style = MaterialTheme.typography.headlineSmall)
            Text(record.address, style = MaterialTheme.typography.bodyMedium)
            FilledTonalButton(onClick = onEdit, modifier = Modifier.fillMaxWidth()) { Text("编辑连接") }
            TextButton(onClick = { confirming = true }, modifier = Modifier.fillMaxWidth()) {
                Text("移除资源库", color = MaterialTheme.colorScheme.error)
            }
        }
    }
    if (confirming) AlertDialog(onDismissRequest = { confirming = false },
        title = { Text("移除“${record.name}”？") },
        text = { Text("移除本机保存的连接与账号。服务器上的影片会保留。") },
        confirmButton = { TextButton(onClick = onDelete) { Text("移除") } },
        dismissButton = { TextButton(onClick = { confirming = false }) { Text("取消") } })
}
