package com.kivo.cinema.sources.ui.workspace

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalClipboardManager
import androidx.compose.ui.platform.LocalSoftwareKeyboardController
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.text.input.VisualTransformation
import androidx.compose.ui.unit.dp
import com.kivo.cinema.sources.SourceManagerModel
import com.kivo.cinema.sources.SourceShareParser

import com.kivo.cinema.sources.ui.SourcePastePreview

@Suppress("DEPRECATION")
@Composable
internal fun SourceConnectionContent(model: SourceManagerModel, onCancel: () -> Unit) {
    val clipboard = LocalClipboardManager.current
    val keyboard = LocalSoftwareKeyboardController.current
    var visible by remember { mutableStateOf(false) }
    var preview by remember { mutableStateOf<SourceShareParser.Preview?>(null) }
    var selected by remember { mutableStateOf<String?>(null) }
        Column(Modifier.fillMaxWidth().fillMaxHeight()) {
            Column(Modifier.weight(1f).verticalScroll(rememberScrollState())
                .padding(horizontal = 20.dp).padding(top = 20.dp), verticalArrangement = Arrangement.spacedBy(14.dp)) {
                Text(if (model.editingExisting) "编辑资源库" else "添加资源库",
                    style = MaterialTheme.typography.titleMedium)
                Text("连接 Emby，把喜欢的影片收在一起。", style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant)
                OutlinedButton(onClick = {
                    keyboard?.hide()
                    preview = SourceShareParser.parse(clipboard.getText()?.text.orEmpty())
                    selected = preview?.addresses?.singleOrNull()
                }, enabled = !model.busy, modifier = Modifier.fillMaxWidth()) { Text("粘贴识别") }
                Surface(shape = MaterialTheme.shapes.medium) {
                    Column {
                        SourceInput(model.name, { model.name = it }, "名称", "例如：家里的电影库", !model.busy)
                        HorizontalDivider(Modifier.padding(horizontal = 16.dp), color = MaterialTheme.colorScheme.outlineVariant)
                        SourceInput(model.address, { model.address = it }, "服务器地址", "https://example.com:443",
                            !model.busy, KeyboardType.Uri)
                    }
                }
                Surface(shape = MaterialTheme.shapes.medium) {
                    Column {
                        SourceInput(model.username, { model.username = it }, "账号", "输入服务器账号", !model.busy)
                        HorizontalDivider(Modifier.padding(horizontal = 16.dp), color = MaterialTheme.colorScheme.outlineVariant)
                        TextField(model.password, { model.password = it }, label = { Text("密码") },
                            placeholder = { Text("免密码账号可留空") }, enabled = !model.busy, singleLine = true,
                            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Password),
                            visualTransformation = if (visible) VisualTransformation.None else PasswordVisualTransformation(),
                            trailingIcon = { TextButton(onClick = { visible = !visible }, enabled = !model.busy) {
                                Text(if (visible) "隐藏" else "显示")
                            } }, colors = sourceInputColors(), modifier = Modifier.fillMaxWidth())
                    }
                }
                model.error?.let { Text(it, color = MaterialTheme.colorScheme.error) }
                Spacer(Modifier.height(4.dp))
            }
            Row(Modifier.fillMaxWidth().padding(20.dp), horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                OutlinedButton(onClick = onCancel, enabled = !model.saving,
                    modifier = Modifier.weight(1f).heightIn(min = 52.dp)) {
                    Text(if (model.verifying) "取消连接" else "取消")
                }
                Button(onClick = { keyboard?.hide(); model.save() }, enabled = !model.busy,
                    modifier = Modifier.weight(2f).heightIn(min = 52.dp)) {
                    if (model.busy) {
                        CircularProgressIndicator(Modifier.size(18.dp), strokeWidth = 2.dp,
                            color = MaterialTheme.colorScheme.onPrimary)
                        Spacer(Modifier.width(10.dp))
                    }
                    Text(when { model.saving -> "正在保存"; model.verifying -> "正在验证连接"; else -> "连接并保存" })
                }
            }
        }
    preview?.let { p -> SourcePastePreview(p, selected, { selected = it }, {
        selected?.let { model.applyPreview(p, it) }; preview = null
    }, { preview = null }) }
}

@Composable
private fun SourceInput(value: String, onChange: (String) -> Unit, label: String, hint: String,
    enabled: Boolean, keyboardType: KeyboardType = KeyboardType.Text) {
    TextField(value, onChange, label = { Text(label) }, placeholder = { Text(hint) },
        enabled = enabled, singleLine = true, keyboardOptions = KeyboardOptions(keyboardType = keyboardType),
        colors = sourceInputColors(), modifier = Modifier.fillMaxWidth())
}

@Composable
private fun sourceInputColors() = TextFieldDefaults.colors(
    focusedContainerColor = MaterialTheme.colorScheme.surface,
    unfocusedContainerColor = MaterialTheme.colorScheme.surface,
    disabledContainerColor = MaterialTheme.colorScheme.surface,
    focusedIndicatorColor = Color.Transparent, unfocusedIndicatorColor = Color.Transparent,
    disabledIndicatorColor = Color.Transparent,
)
