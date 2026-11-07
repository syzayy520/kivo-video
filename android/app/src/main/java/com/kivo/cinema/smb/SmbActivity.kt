package com.kivo.cinema.smb

import android.content.Intent
import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.clickable
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.material3.TextFieldDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.text.input.VisualTransformation
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * SMB 源管理 UI（220 设计稿 D4，表单实测规格）：
 *
 * FORM 态：表单（主机/端口/共享/路径/用户/密码）+ 测试连通性（连上即进 BROWSE）
 * BROWSE 态：目录列表（文件夹/文件），点目录进入、点文件起播、返回上一级
 *
 * 设计稿 规格（05 号文档 §三/§6.5）：浅色渐变背景、白色圆角行、左标签右值、
 * 行高 ~59pt（Android 88dp 近似）、错误 reason 用户可见（G9）。
 *
 * 单一职责：SMB 源的表单/浏览/起播转发——协议逻辑全在 Smb2Native.Source。
 */
class SmbActivity : ComponentActivity() {

    // 表单态
    private var host by mutableStateOf("192.168.5.1")
    private var port by mutableStateOf("445")
    private var share by mutableStateOf("")
    private var path by mutableStateOf("")
    private var username by mutableStateOf("")
    private var password by mutableStateOf("")

    // 状态
    private var status by mutableStateOf("填服务器信息，测试连通性")
    private var testing by mutableStateOf(false)

    // 浏览态
    private var browsing by mutableStateOf(false)
    private var currentPath by mutableStateOf("")
    private var entries by mutableStateOf(listOf<SmbSource.Entry>())
    private var cfg by mutableStateOf<SmbConfig?>(null)

    private var source: Smb2Native.Source? = null

    companion object {
        private const val TAG = "KivoSmbUi"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent { SmbScreen() }
    }

    override fun onDestroy() {
        runCatching { source?.close() }
        source = null
        super.onDestroy()
    }

    private fun navigate(path0: String) {
        try {
            val e = source!!.list(path0)
            currentPath = path0
            entries = e
            status = "目录：/${currentPath}（${e.size} 项）"
        } catch (e: Exception) {
            status = "无法读取文件夹：${e.message}"   // 设计稿 R3 文案
        }
    }

    private fun testAndBrowse() {
        testing = true
        status = "正在测试…"   // 设计稿 R2: library.form.testingConnectivity
        Thread {
            try {
                val c = SmbConfig(
                    host = host.trim(), port = port.trim().toIntOrNull() ?: 445,
                    share = share.trim(), path = path.trim(),
                    username = username.trim(), password = password
                )
                val src = Smb2Native.Source(c)
                src.connect()
                runCatching { source?.close() }
                source = src
                cfg = c
                val root = src.list("")
                currentPath = ""
                entries = root
                browsing = true
                status = "可连接（${root.size} 项）"   // 设计稿 R2: reachable
            } catch (e: Exception) {
                status = "无法连接服务器，请检查地址与网络。（${e.message}）"   // 设计稿 R3
            }
            testing = false
        }.start()
    }

    private fun openFile(entry: SmbSource.Entry) {
        val c = cfg ?: return
        val full = if (currentPath.isEmpty()) entry.name else "${currentPath}/${entry.name}"
        val intent = Intent(this, com.kivo.cinema.ui.compose.PlayerComposeActivity::class.java)
            .putExtra(
                "kivo.smb",
                c.copy(path = full).toJson()
            )
        startActivity(intent)
    }

    // ---- UI ----

    @Composable
    private fun SmbScreen() {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .background(
                    Color(0xFFF2F2F7)   // 设计稿 groupedFill 浅色（05 号文档 §二）
                )
                .padding(16.dp)
        ) {
            if (!browsing) FormView() else BrowseView()
            Spacer(Modifier.height(8.dp))
            Text(status, fontSize = 13.sp, color = Color(0xFF8A8A8E), fontWeight = FontWeight.Normal)
        }
    }

    @Composable
    private fun FormView() {
        Column(verticalArrangement = Arrangement.spacedBy(10.dp)) {
            Text("SMB", fontSize = 28.sp, fontWeight = FontWeight.Bold,
                modifier = Modifier.padding(vertical = 8.dp))
            FormRow("主机", host) { host = it }
            FormRow("端口", port) { port = it }
            FormRow("共享", share) { share = it }
            FormRow("路径", path) { path = it }
            FormRow("用户", username) { username = it }
            FormRow("密码", password, password = true) { password = it }
            Spacer(Modifier.height(4.dp))
            Button(
                onClick = { testAndBrowse() },
                enabled = !testing && host.isNotBlank() && share.isNotBlank(),
                colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF34C759)),
                modifier = Modifier.fillMaxWidth().height(52.dp)
            ) {
                Text(if (testing) "正在测试…" else "测试连通性", fontSize = 16.sp)
            }
            Text(
                "先在路由器管理页开启 SMB 共享；匿名可读的共享无需填写用户名密码。",
                fontSize = 12.sp, color = Color(0xFF8A8A8E)
            )
        }
    }

    @Composable
    private fun FormRow(label: String, value: String, password: Boolean = false,
                        onChange: (String) -> Unit) {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            modifier = Modifier
                .fillMaxWidth()
                .background(Color.White, RoundedCornerShape(13.dp))
                .padding(horizontal = 16.dp, vertical = 4.dp)
        ) {
            Text(label, fontSize = 16.sp, color = Color(0xFF1A1A1A),
                modifier = Modifier.width(56.dp))
            TextField(
                value = value,
                onValueChange = onChange,
                singleLine = true,
                visualTransformation = if (password) PasswordVisualTransformation()
                                       else VisualTransformation.None,
                colors = TextFieldDefaults.colors(
                    focusedContainerColor = Color.Transparent,
                    unfocusedContainerColor = Color.Transparent,
                    focusedIndicatorColor = Color.Transparent,
                    unfocusedIndicatorColor = Color.Transparent,
                ),
                modifier = Modifier.weight(1f)
            )
        }
    }

    @Composable
    private fun BrowseView() {
        Column {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Button(
                    onClick = {
                        // 上一级（设计稿 R5）
                        val parent = currentPath.substringBeforeLast('/', missingDelimiterValue = "")
                        navigate(parent)
                    },
                    enabled = currentPath.isNotEmpty(),
                    colors = ButtonDefaults.buttonColors(containerColor = Color(0xFFE5E5EA))
                ) { Text("上一级", fontSize = 14.sp) }
                Spacer(Modifier.width(12.dp))
                Text("/${currentPath}", fontSize = 15.sp, fontWeight = FontWeight.Medium,
                    color = Color(0xFF1A1A1A))
            }
            Spacer(Modifier.height(10.dp))
            LazyColumn(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                items(entries) { e ->
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        modifier = Modifier
                            .fillMaxWidth()
                            .background(Color.White, RoundedCornerShape(13.dp))
                            .clickable {
                                if (e.isDirectory) {
                                    navigate(joinPath(currentPath, e.name))
                                } else {
                                    openFile(e)
                                }
                            }
                            .padding(16.dp)
                    ) {
                        Text(
                            if (e.isDirectory) "📁" else "🎬",
                            fontSize = 22.sp,
                            modifier = Modifier.width(40.dp)
                        )
                        Column(Modifier.weight(1f)) {
                            Text(e.name, fontSize = 15.sp, fontWeight = FontWeight.Medium,
                                color = Color(0xFF1A1A1A), maxLines = 1)
                            if (!e.isDirectory) {
                                Text(formatSize(e.size), fontSize = 12.sp,
                                    color = Color(0xFF8A8A8E))
                            }
                        }
                        if (e.isDirectory) {
                            Text("进入 ›", fontSize = 13.sp, color = Color(0xFF34C759),
                                modifier = Modifier.padding(start = 8.dp))
                        } else {
                            Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                                Button(
                                    onClick = { openFile(e) },
                                    colors = ButtonDefaults.buttonColors(
                                        containerColor = Color(0xFF34C759)
                                    ),
                                    contentPadding = androidx.compose.foundation.layout.PaddingValues(
                                        horizontal = 14.dp, vertical = 6.dp)
                                ) { Text("播放", fontSize = 13.sp) }
                                Button(
                                    onClick = { downloadFile(e) },
                                    colors = ButtonDefaults.buttonColors(
                                        containerColor = Color(0xFF00A1D6)
                                    ),
                                    contentPadding = androidx.compose.foundation.layout.PaddingValues(
                                        horizontal = 14.dp, vertical = 6.dp)
                                ) { Text("下载", fontSize = 13.sp) }
                            }
                        }
                    }
                }
            }
        }
    }

    private fun joinPath(base: String, name: String): String =
        base.trimEnd('/') + "/" + name

    // 下载链路持有的源与代理：DownloadService 说 HTTP，SMB 必须经 220b 同款
    // ByteSource→LocalRangeProxy 桥成 http://127.0.0.1 再交给它（SMB 主机不听 HTTP）。
    private val downloadBridges = mutableListOf<Pair<Smb2Native.Source, com.kivo.cinema.net.LocalRangeProxy>>()

    private fun downloadFile(entry: SmbSource.Entry) {
        val c = cfg ?: return
        val fullPath = if (currentPath.isEmpty()) entry.name else "${currentPath}/${entry.name}"
        status = "开始下载：${entry.name}"
        Thread {
            try {
                val src = Smb2Native.Source(c)
                src.connect()
                val bytes = src.openForRead(fullPath)
                val proxy = com.kivo.cinema.net.LocalRangeProxy(bytes)
                val port = proxy.start()
                downloadBridges.add(src to proxy)
                val intent = Intent(this, com.kivo.cinema.download.DownloadService::class.java)
                    .putExtra(com.kivo.cinema.download.DownloadService.EXTRA_URL,
                        "http://127.0.0.1:$port/kivo")
                    .putExtra(com.kivo.cinema.download.DownloadService.EXTRA_TITLE, entry.name)
                startForegroundService(intent)
            } catch (e: Exception) {
                status = "无法下载：${e.message}"   // G9：错误 reason 用户可见
            }
        }.start()
    }

    private fun formatSize(bytes: Long): String = when {
        bytes >= 1 shl 30 -> "%.1f GB".format(bytes.toDouble() / (1 shl 30))
        bytes >= 1 shl 20 -> "%.1f MB".format(bytes.toDouble() / (1 shl 20))
        bytes >= 1 shl 10 -> "%.1f KB".format(bytes.toDouble() / (1 shl 10))
        else -> "$bytes B"
    }
}
