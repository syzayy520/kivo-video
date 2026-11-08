package com.kivo.cinema.sources.ui

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalClipboardManager
import androidx.compose.ui.platform.LocalSoftwareKeyboardController
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.text.input.VisualTransformation
import androidx.compose.ui.unit.dp
import com.kivo.cinema.sources.SourceManagerModel
import com.kivo.cinema.sources.SourceShareParser

@OptIn(ExperimentalMaterial3Api::class)
@Suppress("DEPRECATION")
@Composable
fun EmbySourceForm(model: SourceManagerModel) {
    val clipboard = LocalClipboardManager.current
    val keyboard = LocalSoftwareKeyboardController.current
    var visible by remember { mutableStateOf(false) }
    var preview by remember { mutableStateOf<SourceShareParser.Preview?>(null) }
    var selected by remember { mutableStateOf<String?>(null) }
    val state = rememberModalBottomSheetState(skipPartiallyExpanded = true,
        confirmValueChange = { !model.saving })
    ModalBottomSheet(onDismissRequest = model::dismiss, sheetState = state, sheetMaxWidth = 560.dp) {
        Column(Modifier.fillMaxWidth().imePadding().verticalScroll(rememberScrollState())
            .padding(horizontal = 24.dp).padding(bottom = 28.dp), verticalArrangement = Arrangement.spacedBy(14.dp)) {
            Text("Emby 连接", style = MaterialTheme.typography.headlineSmall)
            Text("为收藏取个名字，随时回来继续观看。", style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant)
            OutlinedButton(onClick = {
                preview = SourceShareParser.parse(clipboard.getText()?.text.orEmpty())
                selected = preview?.addresses?.singleOrNull()
            }, enabled = !model.busy, modifier = Modifier.fillMaxWidth()) { Text("粘贴识别") }
            OutlinedTextField(model.name, { model.name = it }, label = { Text("名称") },
                placeholder = { Text("例如：家里的电影库") }, enabled = !model.busy, singleLine = true,
                modifier = Modifier.fillMaxWidth())
            OutlinedTextField(model.address, { model.address = it }, label = { Text("服务器地址") },
                placeholder = { Text("https://example.com:443") }, enabled = !model.busy, singleLine = true,
                keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Uri), modifier = Modifier.fillMaxWidth())
            OutlinedTextField(model.username, { model.username = it }, label = { Text("账号") },
                enabled = !model.busy, singleLine = true, modifier = Modifier.fillMaxWidth())
            OutlinedTextField(model.password, { model.password = it }, label = { Text("密码") },
                supportingText = { Text("免密码账号可留空") }, enabled = !model.busy, singleLine = true,
                keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Password),
                visualTransformation = if (visible) VisualTransformation.None else PasswordVisualTransformation(),
                trailingIcon = { TextButton(onClick = { visible = !visible }) { Text(if (visible) "隐藏" else "显示") } },
                modifier = Modifier.fillMaxWidth())
            model.error?.let { Text(it, color = MaterialTheme.colorScheme.error) }
            Button(onClick = { keyboard?.hide(); model.save() }, enabled = !model.busy,
                modifier = Modifier.fillMaxWidth().heightIn(min = 52.dp)) {
                if (model.busy) { CircularProgressIndicator(Modifier.size(18.dp), strokeWidth = 2.dp); Spacer(Modifier.width(10.dp)) }
                Text(when { model.saving -> "正在保存"; model.verifying -> "正在验证连接"; else -> "连接并保存" })
            }
            TextButton(onClick = model::dismiss, enabled = !model.saving, modifier = Modifier.fillMaxWidth()) {
                Text(if (model.verifying) "取消连接" else "取消")
            }
        }
    }
    preview?.let { p ->
        AlertDialog(onDismissRequest = { preview = null }, title = { Text("检查识别结果") }, text = {
            Column(Modifier.verticalScroll(rememberScrollState()), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                p.addresses.forEach { candidate ->
                    Row { RadioButton(selected == candidate, { selected = candidate }); Text(candidate, Modifier.padding(top = 10.dp)) }
                }
                Text("名称：${p.name ?: "未识别"}")
                Text("账号：${if (p.username != null) "已识别" else "未识别"}")
                Text("密码：${when (p.password) { null -> "未识别"; "" -> "空密码"; else -> "已识别（隐藏）" }}")
                p.issues.forEach { Text(it, color = MaterialTheme.colorScheme.error) }
            }
        }, confirmButton = {
            TextButton(onClick = { selected?.let { model.applyPreview(p, it) }; preview = null },
                enabled = selected != null && p.issues.all { it == "检测到多个地址，请选择后再连接" }) { Text("填入表单") }
        }, dismissButton = { TextButton(onClick = { preview = null }) { Text("取消") } })
    }
}
