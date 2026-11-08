package com.kivo.cinema.sources.ui

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.selection.selectable
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.unit.dp
import com.kivo.cinema.sources.SourceShareParser

/** Shows sanitized parser output only; filling a draft never initiates a connection. */
@Composable
internal fun SourcePastePreview(
    preview: SourceShareParser.Preview, selected: String?,
    onSelect: (String) -> Unit, onFill: () -> Unit, onDismiss: () -> Unit,
) {
    AlertDialog(onDismissRequest = onDismiss, title = { Text("检查识别结果") }, text = {
        Column(Modifier.verticalScroll(rememberScrollState()), verticalArrangement = Arrangement.spacedBy(12.dp)) {
            preview.addresses.forEach { candidate ->
                Row(Modifier.fillMaxWidth().selectable(selected == candidate, role = Role.RadioButton,
                    onClick = { onSelect(candidate) }).padding(vertical = 8.dp),
                    verticalAlignment = Alignment.CenterVertically) {
                    RadioButton(selected == candidate, onClick = null)
                    Spacer(Modifier.width(12.dp))
                    Text(candidate, Modifier.weight(1f))
                }
            }
            Text("名称：${preview.name ?: "未识别"}")
            Text("账号：${if (preview.username != null) "已识别" else "未识别"}")
            Text("密码：${when (preview.password) { null -> "未识别"; "" -> "空密码"; else -> "已识别（隐藏）" }}")
            preview.issues.forEach { Text(it, color = MaterialTheme.colorScheme.error) }
        }
    }, confirmButton = {
        TextButton(onClick = onFill, enabled = selected != null &&
            preview.issues.all { it == "检测到多个地址，请选择后再连接" }) { Text("填入表单") }
    }, dismissButton = { TextButton(onClick = onDismiss) { Text("取消") } })
}
