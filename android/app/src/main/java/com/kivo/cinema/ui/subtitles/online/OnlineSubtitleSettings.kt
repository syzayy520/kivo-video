package com.kivo.cinema.ui.subtitles.online

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.subtitle.online.OnlineSubtitlePrefs
import com.kivo.cinema.subtitle.online.OpenSubtitlesClient
import com.kivo.cinema.subtitle.online.SubHdClient
import com.kivo.cinema.tmdb.TmdbClient
import com.kivo.cinema.ui.design.settings.ChevronRow
import com.kivo.cinema.ui.design.settings.Footnote
import com.kivo.cinema.ui.design.settings.SectionCard
import com.kivo.cinema.ui.design.settings.SwitchRow
import com.kivo.cinema.ui.design.settings.ValueRow
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * 设置 → 播放体验 → 字幕：OpenSubtitles 鉴权与自动选中 + 连接测试。
 * 账号密码只存私有 SharedPreferences（online_subtitles），不回显明文。
 * 「测试连接」让用户填完 key 后一键验证两源是否可达，不用进播放面板。
 */
@Composable
internal fun OnlineSubtitleSettings(prefs: OnlineSubtitlePrefs, onChanged: () -> Unit) {
    val context = LocalContext.current
    var showLogin by remember { mutableStateOf(false) }
    var apiKeyInput by remember { mutableStateOf(prefs.apiKey) }
    var usernameInput by remember { mutableStateOf(prefs.username) }
    var passwordInput by remember { mutableStateOf(prefs.password) }
    var loginState by remember { mutableStateOf<String?>(null) }
    var testRunning by remember { mutableStateOf(false) }
    var osResult by remember { mutableStateOf<String?>(null) }
    var hdResult by remember { mutableStateOf<String?>(null) }
    val scope = rememberCoroutineScope()

    fun runConnectionTest() {
        testRunning = true
        osResult = "测试中…"
        hdResult = "测试中…"
        scope.launch {
            val tmdb = TmdbClient(context)
            val os = withContext(Dispatchers.IO) {
                runCatching {
                    val items = OpenSubtitlesClient(tmdb, prefs).search("odyssey", languages = "en")
                    "✓ 连通（${items.size} 条结果）"
                }.getOrElse { "✗ ${it.message ?: "连接失败"}" }
            }
            val hd = withContext(Dispatchers.IO) {
                runCatching {
                    val items = SubHdClient(tmdb).search("odyssey")
                    "✓ 连通（${items.size} 条结果）"
                }.getOrElse { "✗ ${it.message ?: "连接失败"}" }
            }
            osResult = "OpenSubtitles: $os"
            hdResult = "SubHD: $hd"
            testRunning = false
        }
    }

    SectionCard("在线字幕（OpenSubtitles）") {
        ValueRow("Api-Key", if (apiKeyInput.isBlank()) "匿名公共配额" else "已填写")
        ChevronRow("填写 Api-Key / 登录账号", onClick = { showLogin = true })
        ChevronRow(
            if (testRunning) "测试中…" else "测试连接",
            onClick = { if (!testRunning) runConnectionTest() },
        )
        if (osResult != null) Footnote(osResult ?: "")
        if (hdResult != null) Footnote(hdResult ?: "")
        SwitchRow("下载后自动选中字幕", prefs.autoSelect,
            subtitle = "关闭时仅下载到本地字幕库，不切换当前字幕") {
            prefs.autoSelect = it
            onChanged()
        }
        if (loginState != null) Footnote(loginState ?: "")
    }

    if (showLogin) AlertDialog(
        onDismissRequest = { showLogin = false },
        title = { Text("OpenSubtitles 鉴权", fontSize = 17.sp) },
        text = {
            Column {
                OutlinedTextField(value = apiKeyInput, onValueChange = { apiKeyInput = it },
                    label = { Text("Api-Key（可空=匿名）") }, singleLine = true)
                Spacer(Modifier.height(8.dp))
                OutlinedTextField(value = usernameInput, onValueChange = { usernameInput = it },
                    label = { Text("账号（下载需要）") }, singleLine = true)
                Spacer(Modifier.height(8.dp))
                OutlinedTextField(value = passwordInput, onValueChange = { passwordInput = it },
                    label = { Text("密码") }, singleLine = true)
            }
        },
        confirmButton = {
            TextButton(onClick = {
                prefs.apiKey = apiKeyInput
                prefs.username = usernameInput
                prefs.password = passwordInput
                onChanged()
                showLogin = false
                if (usernameInput.isNotBlank() && passwordInput.isNotBlank()) {
                    loginState = "正在登录…"
                    scope.launch {
                        loginState = withContext(Dispatchers.IO) {
                            runCatching {
                                OpenSubtitlesClient(TmdbClient(context), prefs)
                                    .login(usernameInput, passwordInput)
                            }.fold({ "登录成功，可下载字幕" }, { it.message ?: "登录失败" })
                        }
                    }
                }
            }) { Text("保存并登录") }
        },
        dismissButton = { TextButton(onClick = { showLogin = false }) { Text("取消") } },
    )
}