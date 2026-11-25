package com.kivo.cinema.ui.subtitles.online

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.RadioButton
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.subtitle.remote.RemoteAiEngineClient
import com.kivo.cinema.subtitle.remote.RemoteEnginePrefs
import com.kivo.cinema.subtitle.remote.RemoteProvider
import com.kivo.cinema.ui.design.settings.ChevronRow
import com.kivo.cinema.ui.design.settings.Footnote
import com.kivo.cinema.ui.design.settings.SectionCard
import com.kivo.cinema.ui.design.settings.SwitchRow
import com.kivo.cinema.ui.design.settings.ValueRow
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * 设置 → AI 翻译引擎（design.md ai-online-2026-11-25 §三）：
 * 本地模型（默认）/ 在线接口二选一；在线配置 = 服务商 + Key + 模型（+ 自定义地址）。
 * 失败语义：远程失败给非空中文原因，不静默回退；隐私明示文本出设备。
 */
@Composable
internal fun AiEngineSettings(onChanged: () -> Unit) {
    val context = LocalContext.current
    val prefs = remember { RemoteEnginePrefs(context) }
    var showProvider by remember { mutableStateOf(false) }
    var showConfig by remember { mutableStateOf(false) }
    var showLibrary by remember { mutableStateOf(false) }
    var testRunning by remember { mutableStateOf(false) }
    var testResult by remember { mutableStateOf<String?>(null) }
    val scope = rememberCoroutineScope()

    SectionCard("AI 翻译引擎") {
        SwitchRow("使用在线接口翻译", prefs.useRemote,
            subtitle = "关闭 = 本地模型；开启后外语字幕经所选服务商译成中文") {
            prefs.useRemote = it
            testResult = null
            onChanged()
        }
        ValueRow("服务商", prefs.provider.label)
        ValueRow("模型", prefs.effectiveModel() ?: "未配置")
        ValueRow("接口密钥", if (prefs.apiKey.isBlank()) "未填写" else "已填写")
        ChevronRow("选择服务商", onClick = { showProvider = true })
        ChevronRow("填写接口配置", onClick = { showConfig = true })
        ChevronRow(
            if (testRunning) "测试中…" else "测试连接",
            onClick = {
                if (!testRunning) {
                    testRunning = true
                    testResult = "测试中…"
                    scope.launch {
                        testResult = withContext(Dispatchers.IO) {
                            val configError = prefs.configError()
                            if (configError != null) {
                                "✗ $configError"
                            } else {
                                RemoteAiEngineClient(context).testConnection().fold(
                                    { "✓ 连通（$it 个模型）" },
                                    { "✗ ${it.message ?: "连接失败"}" },
                                )
                            }
                        }
                        testRunning = false
                    }
                }
            },
        )
        if (testResult != null) Footnote(testResult ?: "")
        ChevronRow("打开字幕库（已翻译 / 已下载）", onClick = { showLibrary = true })
        Footnote("开启在线翻译即表示字幕文本将发送至所选服务商；翻译费用由您在该服务商的账户承担。国内网络访问 OpenAI/Gemini 可在应用数据目录 tmdb_proxy.txt 配置代理。")
    }

    if (showLibrary) {
        com.kivo.cinema.ui.subtitles.library.SubtitleLibraryDialog(onDismiss = { showLibrary = false })
    }

    if (showProvider) AlertDialog(
        onDismissRequest = { showProvider = false },
        title = { Text("选择服务商", fontSize = 17.sp) },
        text = {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                RemoteProvider.entries.forEach { candidate ->
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        RadioButton(
                            selected = prefs.provider == candidate,
                            onClick = {
                                prefs.provider = candidate
                                prefs.model = ""
                                onChanged()
                                showProvider = false
                            },
                        )
                        Text(candidate.label, fontSize = 15.sp)
                    }
                }
            }
        },
        confirmButton = { TextButton(onClick = { showProvider = false }) { Text("关闭") } },
    )

    if (showConfig) {
        // 输入必须走 Compose 本地状态：直接绑 SharedPreferences 读值不触发重组，
        // 敲键盘后 TextField 永远显示旧值（2026-11-25 真机实测「填写不进去」）。
        // OnlineSubtitleSettings 登录弹窗同款模式：本地态输入，保存才落盘。
        var keyInput by remember { mutableStateOf(prefs.apiKey) }
        var modelInput by remember { mutableStateOf(prefs.model) }
        var urlInput by remember { mutableStateOf(prefs.customBaseUrl) }
        AlertDialog(
            onDismissRequest = { showConfig = false },
            title = { Text("接口配置（${prefs.provider.label}）", fontSize = 17.sp) },
            text = {
                Column {
                    if (prefs.provider == RemoteProvider.CUSTOM) {
                        OutlinedTextField(
                            value = urlInput,
                            onValueChange = { urlInput = it },
                            label = { Text("接口地址（如 https://…/v1）") }, singleLine = true,
                        )
                        Spacer(Modifier.height(8.dp))
                    }
                    OutlinedTextField(
                        value = keyInput, onValueChange = { keyInput = it },
                        label = { Text("API Key") }, singleLine = true,
                    )
                    Spacer(Modifier.height(8.dp))
                    OutlinedTextField(
                        value = modelInput, onValueChange = { modelInput = it },
                        label = {
                            Text("模型名" + (prefs.provider.defaultModel.takeIf { it.isNotBlank() }
                                ?.let { "（默认 $it）" } ?: ""))
                        },
                        singleLine = true,
                    )
                }
            },
            confirmButton = {
                TextButton(onClick = {
                    prefs.apiKey = keyInput
                    prefs.model = modelInput
                    prefs.customBaseUrl = urlInput
                    onChanged()
                    showConfig = false
                }) { Text("保存") }
            },
            dismissButton = { TextButton(onClick = { showConfig = false }) { Text("取消") } },
        )
    }
}