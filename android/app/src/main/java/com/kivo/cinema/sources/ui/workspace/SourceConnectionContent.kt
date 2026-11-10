package com.kivo.cinema.sources.ui.workspace

import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalClipboardManager
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalSoftwareKeyboardController
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.text.input.VisualTransformation
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.smb.discovery.SmbDiscovery
import com.kivo.cinema.sources.SourceManagerModel
import com.kivo.cinema.sources.SourceRecord
import com.kivo.cinema.sources.SourceShareParser

import com.kivo.cinema.sources.ui.SourcePastePreview

/**
 * 连接表单（243 D2/D5）。同一份骨架服务两种协议，差别只在文案与字段语义：
 *
 * | 字段 | Emby | SMB |
 * | --- | --- | --- |
 * | 地址 | `https://host:443` | `smb://host[:port][/共享][/路径]` |
 * | 账号 | 必填 | 留空 = 匿名 |
 * | 名称 | 空则用主机名 | 空则用共享名（无共享用主机名） |
 */
@Suppress("DEPRECATION")
@Composable
internal fun SourceConnectionContent(model: SourceManagerModel, onCancel: () -> Unit) {
    val clipboard = LocalClipboardManager.current
    val keyboard = LocalSoftwareKeyboardController.current
    var visible by remember { mutableStateOf(false) }
    var preview by remember { mutableStateOf<SourceShareParser.Preview?>(null) }
    var selected by remember { mutableStateOf<String?>(null) }
    val smb = model.protocol == SourceRecord.PROTOCOL_SMB
    Column(Modifier.fillMaxWidth().fillMaxHeight()) {
        Column(Modifier.weight(1f).verticalScroll(rememberScrollState())
            .padding(horizontal = 20.dp).padding(top = 20.dp), verticalArrangement = Arrangement.spacedBy(14.dp)) {
            Text(
                when {
                    smb && model.editingExisting -> "编辑 SMB"
                    smb -> "添加 SMB"
                    model.editingExisting -> "编辑资源库"
                    else -> "添加资源库"
                },
                style = MaterialTheme.typography.titleMedium,
            )
            Text(
                if (smb) "连接局域网共享文件夹。地址里不写共享名时，保存后可以再选。"
                else "连接 Emby，把喜欢的影片收在一起。",
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )
            if (smb) SmbCandidates(model)
            OutlinedButton(onClick = {
                keyboard?.hide()
                preview = SourceShareParser.parse(clipboard.getText()?.text.orEmpty())
                selected = preview?.addresses?.singleOrNull()
            }, enabled = !model.busy, modifier = Modifier.fillMaxWidth()) { Text("粘贴识别") }
            Surface(shape = MaterialTheme.shapes.medium) {
                Column {
                    SourceInput(model.name, { model.name = it }, "名称",
                        if (smb) "选填，默认用共享名" else "例如：家里的电影库", !model.busy)
                    HorizontalDivider(Modifier.padding(horizontal = 16.dp), color = MaterialTheme.colorScheme.outlineVariant)
                    SourceInput(model.address, { model.address = it },
                        if (smb) "共享地址" else "服务器地址",
                        if (smb) "smb://192.168.1.10/视频" else "https://example.com:443",
                        !model.busy, KeyboardType.Uri)
                }
            }
            Surface(shape = MaterialTheme.shapes.medium) {
                Column {
                    SourceInput(model.username, { model.username = it },
                        if (smb) "用户名" else "账号",
                        if (smb) "留空表示匿名访问" else "输入服务器账号", !model.busy)
                    HorizontalDivider(Modifier.padding(horizontal = 16.dp), color = MaterialTheme.colorScheme.outlineVariant)
                    TextField(model.password, { model.password = it }, label = { Text("密码") },
                        placeholder = { Text(if (smb) "匿名访问可留空" else "免密码账号可留空") },
                        enabled = !model.busy, singleLine = true,
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

/**
 * 局域网里广播 `_smb._tcp` 的设备（243 D8）。点一下就把地址填进表单；
 * 发现不到任何设备时不占空间 —— 手工输入永远是主路径。
 */
@Composable
private fun SmbCandidates(model: SourceManagerModel) {
    val context = LocalContext.current
    var candidates by remember { mutableStateOf<List<SmbDiscovery.Found>>(emptyList()) }
    DisposableEffect(Unit) {
        val discovery = SmbDiscovery(context)
        discovery.start { found ->
            if (candidates.none { it.host == found.host }) candidates = candidates + found
        }
        onDispose { discovery.stop() }
    }
    if (candidates.isEmpty()) return
    Column(verticalArrangement = Arrangement.spacedBy(6.dp)) {
        Text("可用地址", fontSize = 12.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
        Row(Modifier.horizontalScroll(rememberScrollState()),
            horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            candidates.forEach { candidate ->
                AssistChip(
                    onClick = { if (!model.busy) model.address = "smb://" + candidate.host },
                    label = {
                        Text(
                            candidate.host + (if (candidate.label.isBlank()) "" else "  " + candidate.label),
                            fontSize = 12.sp,
                        )
                    },
                )
            }
        }
    }
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
