package com.kivo.cinema.emby.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusDirection
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.platform.LocalSoftwareKeyboardController
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.*
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.emby.EmbySessionModel

@Composable
fun EmbyConnectionScreen(model: EmbySessionModel, onBack: () -> Unit) {
    Column(Modifier.fillMaxSize()) {
        TextButton(onClick = onBack, modifier = Modifier.padding(start = 8.dp, top = 4.dp)) {
            Text("‹  资源库", fontSize = 16.sp)
        }
        BoxWithConstraints(Modifier.weight(1f).fillMaxWidth()) {
            val wide = maxWidth >= 840.dp
            val padding = if (wide) 48.dp else 24.dp
            Column(Modifier.fillMaxSize().verticalScroll(rememberScrollState())
                .padding(horizontal = padding, vertical = 24.dp),
                verticalArrangement = Arrangement.Center,
                horizontalAlignment = Alignment.CenterHorizontally) {
                if (wide) Row(Modifier.widthIn(max = 1040.dp).fillMaxWidth(),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(64.dp)) {
                    Column(Modifier.weight(1f)) { ConnectionIntro() }
                    Column(Modifier.width(420.dp)) { ConnectionForm(model) }
                } else Column(Modifier.widthIn(max = 460.dp).fillMaxWidth()) {
                    ConnectionIntro()
                    Spacer(Modifier.height(32.dp))
                    ConnectionForm(model)
                }
            }
        }
    }
}

@Composable
private fun ConnectionIntro() {
    Surface(color = MaterialTheme.colorScheme.primary.copy(alpha = .09f),
        shape = RoundedCornerShape(18.dp)) {
        Text("Emby", color = MaterialTheme.colorScheme.primary, fontSize = 19.sp,
            fontWeight = FontWeight.Bold, modifier = Modifier.padding(horizontal = 20.dp, vertical = 14.dp))
    }
    Spacer(Modifier.height(22.dp))
    Text("你的电影，\n在这里相遇", fontSize = 32.sp, lineHeight = 42.sp, fontWeight = FontWeight.Bold)
    Spacer(Modifier.height(12.dp))
    Text("连接 Emby 服务器，浏览你的电影收藏。", fontSize = 15.sp, lineHeight = 24.sp,
        color = MaterialTheme.colorScheme.onSurfaceVariant)
}

@Composable
private fun ConnectionForm(model: EmbySessionModel) {
    val focus = LocalFocusManager.current
    val keyboard = LocalSoftwareKeyboardController.current
    var visible by rememberSaveable { mutableStateOf(false) }
    val submit = { keyboard?.hide(); focus.clearFocus(); model.connect() }
    Text("连接媒体库", fontSize = 20.sp, fontWeight = FontWeight.SemiBold)
    Spacer(Modifier.height(16.dp))
    Surface(shape = RoundedCornerShape(20.dp), color = MaterialTheme.colorScheme.surface) {
        Column(Modifier.padding(18.dp), verticalArrangement = Arrangement.spacedBy(14.dp)) {
            ConnectionField("服务器地址", model.server, "https://your-server.com", !model.connecting,
                KeyboardType.Uri, ImeAction.Next, onNext = { focus.moveFocus(FocusDirection.Next) }) {
                model.server = it
            }
            HorizontalDivider(color = MaterialTheme.colorScheme.background)
            ConnectionField("账号", model.username, "输入你的用户名", !model.connecting,
                KeyboardType.Text, ImeAction.Next, onNext = { focus.moveFocus(FocusDirection.Next) }) {
                model.username = it
            }
            HorizontalDivider(color = MaterialTheme.colorScheme.background)
            Column {
                Text("密码", fontSize = 13.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
                TextField(value = model.password, onValueChange = { model.password = it },
                    enabled = !model.connecting, singleLine = true,
                    placeholder = { Text("可选，免密码账号可留空", fontSize = 14.sp) },
                    visualTransformation = if (visible) VisualTransformation.None else PasswordVisualTransformation(),
                    trailingIcon = { TextButton(onClick = { visible = !visible }) { Text(if (visible) "隐藏" else "显示") } },
                    keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Password, imeAction = ImeAction.Done),
                    keyboardActions = KeyboardActions(onDone = { submit() }),
                    colors = connectionFieldColors(), modifier = Modifier.fillMaxWidth())
            }
        }
    }
    Spacer(Modifier.height(12.dp))
    Text("地址可包含端口或服务路径。", fontSize = 12.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
    model.error?.let { reason ->
        Spacer(Modifier.height(16.dp))
        Surface(color = MaterialTheme.colorScheme.errorContainer, shape = RoundedCornerShape(14.dp)) {
            Text(reason, color = MaterialTheme.colorScheme.onErrorContainer, fontSize = 14.sp,
                lineHeight = 21.sp, modifier = Modifier.fillMaxWidth().padding(16.dp))
        }
    }
    Spacer(Modifier.height(24.dp))
    Button(onClick = { submit() }, enabled = !model.connecting && model.server.isNotBlank() && model.username.isNotBlank(),
        shape = RoundedCornerShape(16.dp), modifier = Modifier.fillMaxWidth().heightIn(min = 54.dp)) {
        if (model.connecting) {
            CircularProgressIndicator(Modifier.size(18.dp), strokeWidth = 2.dp)
            Spacer(Modifier.width(10.dp))
        }
        Text(if (model.connecting) "正在连接…" else "连接媒体库", fontSize = 16.sp, fontWeight = FontWeight.SemiBold)
    }
    if (model.connecting) TextButton(onClick = { model.cancel() }, modifier = Modifier.fillMaxWidth()) { Text("取消连接") }
}

@Composable
private fun ConnectionField(label: String, value: String, hint: String, enabled: Boolean,
    type: KeyboardType, action: ImeAction, onNext: () -> Unit, onChange: (String) -> Unit) {
    Column {
        Text(label, fontSize = 13.sp, color = MaterialTheme.colorScheme.onSurfaceVariant)
        TextField(value = value, onValueChange = onChange, enabled = enabled,
            singleLine = true, placeholder = { Text(hint, fontSize = 14.sp) },
            keyboardOptions = KeyboardOptions(keyboardType = type, autoCorrectEnabled = false, imeAction = action),
            keyboardActions = KeyboardActions(onNext = { onNext() }), colors = connectionFieldColors(),
            modifier = Modifier.fillMaxWidth())
    }
}

@Composable
private fun connectionFieldColors() = TextFieldDefaults.colors(
    focusedContainerColor = MaterialTheme.colorScheme.surface,
    unfocusedContainerColor = MaterialTheme.colorScheme.surface,
    disabledContainerColor = MaterialTheme.colorScheme.surface,
    focusedIndicatorColor = androidx.compose.ui.graphics.Color.Transparent,
    unfocusedIndicatorColor = androidx.compose.ui.graphics.Color.Transparent,
    disabledIndicatorColor = androidx.compose.ui.graphics.Color.Transparent)
