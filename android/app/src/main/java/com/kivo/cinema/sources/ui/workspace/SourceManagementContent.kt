package com.kivo.cinema.sources.ui.workspace

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.selection.toggleable
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.sources.SourceRecord
import com.kivo.cinema.sources.SourceRepository.Control
import com.kivo.cinema.sources.ui.cards.SourceIcon
import com.kivo.cinema.sources.ui.cards.SourceIconKind

/** Shared management page; the shell decides whether it is a pane or a sheet. */
@Composable
internal fun SourceManagementContent(record: SourceRecord, onDismiss: () -> Unit, onEdit: () -> Unit,
    onDelete: () -> Unit, onControl: (Control, Boolean) -> Unit, busy: Boolean, error: String?) {
    var confirming by remember(record.id) { mutableStateOf(false) }
    Column(Modifier.fillMaxWidth().verticalScroll(rememberScrollState()).padding(20.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp)) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("资源库管理", Modifier.weight(1f), fontSize = 18.sp, fontWeight = FontWeight.Medium)
            TextButton(onClick = onDismiss, enabled = !busy) { Text("关闭") }
        }
        Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(12.dp)) {
            SourceIcon(if (record.isSmb) SourceIconKind.SMB else SourceIconKind.EMBY)
            Column(Modifier.weight(1f)) {
                Text(record.name, fontSize = 16.sp, fontWeight = FontWeight.Medium)
                Text(
                    if (record.isSmb) "SMB 网络共享 · ${record.address.removePrefix("smb://")}"
                    else "Emby 媒体服务器",
                    fontSize = 12.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
            }
        }
        Surface(shape = MaterialTheme.shapes.medium) {
            TextButton(onClick = onEdit, enabled = !busy, modifier = Modifier.fillMaxWidth().heightIn(min = 52.dp)) {
                Text("编辑连接", Modifier.weight(1f).padding(start = 8.dp))
                Text("›", Modifier.padding(end = 8.dp))
            }
        }
        Surface(shape = MaterialTheme.shapes.medium) {
            Column {
                Preference("默认首页", "启动时直接进入此服务器", record.defaultHome, !busy) { onControl(Control.DEFAULT_HOME, it) }
                Preference("首页聚合", "允许内容加入首页聚合", record.aggregateEnabled, !busy) { onControl(Control.AGGREGATE, it) }
                Preference("聚合搜索", "允许跨服务器搜索此资源库", record.aggregateSearchEnabled, !busy) { onControl(Control.AGGREGATE_SEARCH, it) }
                Preference("隐藏并加锁", "验证身份后显示，退后台重新锁定", record.hidden, !busy) { onControl(Control.HIDDEN, it) }
            }
        }
        Text("聚合偏好已独立保存，将在首页聚合与跨服务器搜索接入后生效。服务器内搜索不受影响。",
            fontSize = 12.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
        error?.let { Text(it, color = MaterialTheme.colorScheme.error, fontSize = 13.sp) }
        if (busy) LinearProgressIndicator(Modifier.fillMaxWidth())
        TextButton(onClick = { confirming = true }, enabled = !busy,
            modifier = Modifier.fillMaxWidth().heightIn(min = 48.dp)) {
            Text("移除资源库", color = MaterialTheme.colorScheme.error)
        }
    }
    if (confirming) AlertDialog(onDismissRequest = { confirming = false },
        title = { Text("移除“${record.name}”？") },
        text = { Text("移除本机保存的连接与账号。服务器上的影片会保留。") },
        confirmButton = { TextButton(onClick = { confirming = false; onDelete() }, enabled = !busy) {
            Text("移除", color = MaterialTheme.colorScheme.error)
        } }, dismissButton = { TextButton(onClick = { confirming = false }) { Text("取消") } })
}

@Composable
private fun Preference(title: String, description: String, checked: Boolean, enabled: Boolean, onChange: (Boolean) -> Unit) {
    Row(Modifier.fillMaxWidth().toggleable(checked, enabled = enabled, role = Role.Switch, onValueChange = onChange)
        .padding(horizontal = 16.dp, vertical = 10.dp), verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(8.dp)) {
        Column(Modifier.weight(1f)) {
            Text(title, fontSize = 14.sp)
            Text(description, fontSize = 11.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
        }
        Switch(checked, onCheckedChange = null, enabled = enabled)
    }
}
