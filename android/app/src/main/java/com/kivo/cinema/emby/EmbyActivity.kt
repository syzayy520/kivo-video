package com.kivo.cinema.emby

import android.content.Intent
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.material3.TextFieldDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.text.input.VisualTransformation
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.Executors

/**
 * Emby 客户端 UI（224 设计稿 D4，设计稿 0114 表单规格 + 222 海报墙规格）：
 *
 * FORM 态：服务器/用户/密码 + 测试连接（连上即进海报墙）
 * LIBRARY 态：电影海报墙（2:3 卡 3 列，名称 + 年份徽标），点卡片直连流起播
 *
 * 证据锚：规格 §6.2 库类型枚举 Emby/Jellyfin 首位；表单规格 = SmbActivity 同款
 * （05 号文档 §三）；海报墙 = MediaLibraryActivity 同款（2:3 卡）。
 * 错误 reason 用户可见（G9）；直连失败不静默降级（G3，回退 reason 由会话出版）。
 */
class EmbyActivity : ComponentActivity() {

    private var server by mutableStateOf("")
    private var username by mutableStateOf("")
    private var password by mutableStateOf("")

    private var status by mutableStateOf("填 Emby 服务器与账号，测试连接")
    private var testing by mutableStateOf(false)

    private var inLibrary by mutableStateOf(false)
    private var items by mutableStateOf(listOf<EmbyItem>())
    private var client by mutableStateOf<EmbyClient?>(null)
    private var auth by mutableStateOf<EmbyClient.Auth?>(null)

    private val posters = mutableStateMapOf<String, Bitmap>()   // 线程池 put → 重组
    private val posterFetching = ConcurrentHashMap.newKeySet<String>()
    private val posterPool = Executors.newFixedThreadPool(4)

    companion object {
        private const val TAG = "KivoEmbyUi"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent { EmbyScreen() }
    }

    override fun onDestroy() {
        posterPool.shutdownNow()
        super.onDestroy()
    }

    private fun testAndBrowse() {
        testing = true
        status = "正在连接…"
        Thread {
            try {
                val c = EmbyClient(server)
                val a = c.login(username, password)
                val list = c.items(a)
                client = c
                auth = a
                items = list
                inLibrary = true
                status = "已连接：${list.size} 部电影"
            } catch (e: Exception) {
                Log.w(TAG, "emby connect failed", e)
                status = "连接失败：${e.message}"   // G9
            }
            testing = false
        }.start()
    }

    private fun openItem(item: EmbyItem) {
        val c = client ?: return
        val a = auth ?: return
        val url = c.streamUrl(a, item.id)
        startActivity(
            Intent(this, com.kivo.cinema.ui.compose.PlayerComposeActivity::class.java)
                .putExtra("kivo.playurl", url)
                .putExtra("kivo.label", item.name)
                .putExtra("kivo.routing", "emby-direct")
        )
    }

    /** 海报懒加载：4 线程池 + 内存缓存；SnapshotStateMap put 自带重组。 */
    private fun ensurePoster(item: EmbyItem) {
        if (item.posterUrl.isEmpty() || posters.containsKey(item.id)) return
        if (!posterFetching.add(item.id)) return
        posterPool.execute {
            try {
                val bytes = client!!.fetchBytes(item.posterUrl)
                val bmp = BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
                if (bmp != null) posters[item.id] = bmp
            } catch (e: Exception) {
                Log.w(TAG, "poster failed ${item.id}", e)
            } finally {
                posterFetching.remove(item.id)
            }
        }
    }

    // ---- UI ----

    @Composable
    private fun EmbyScreen() {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .background(Color(0xFFF2F2F7))   // 设计稿 groupedFill（05 号文档 §二）
                .padding(16.dp)
        ) {
            if (!inLibrary) FormView() else LibraryView()
            Spacer(Modifier.height(8.dp))
            Text(status, fontSize = 13.sp, color = Color(0xFF8A8A8E))
        }
    }

    @Composable
    private fun FormView() {
        Column(verticalArrangement = Arrangement.spacedBy(10.dp)) {
            Text("Emby", fontSize = 28.sp, fontWeight = FontWeight.Bold,
                modifier = Modifier.padding(vertical = 8.dp))
            FormRow("服务器", server) { server = it }
            FormRow("用户", username) { username = it }
            FormRow("密码", password, password = true) { password = it }
            Spacer(Modifier.height(4.dp))
            Button(
                onClick = { testAndBrowse() },
                enabled = !testing && server.isNotBlank() && username.isNotBlank(),
                colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF34C759)),
                modifier = Modifier.fillMaxWidth().height(52.dp)
            ) {
                Text(if (testing) "正在连接…" else "测试连接", fontSize = 16.sp)
            }
            Text(
                "例：https://emby.example.com 或 emby.example.com（自动补 https 与 /emby 前缀）。",
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
                modifier = Modifier.width(72.dp))
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
    private fun LibraryView() {
        Column {
            Text("电影 ${items.size}", fontSize = 22.sp, fontWeight = FontWeight.Bold,
                modifier = Modifier.padding(vertical = 8.dp))
            LazyVerticalGrid(
                columns = GridCells.Fixed(3),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                items(items) { item ->
                    ensurePoster(item)
                    PosterCard(item, bmp = posters[item.id],
                        onClick = { openItem(item) })
                }
            }
        }
    }

    @Composable
    private fun PosterCard(item: EmbyItem, bmp: Bitmap?, onClick: () -> Unit) {
        Column(
            modifier = Modifier.clickable { onClick() }
        ) {
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .aspectRatio(2f / 3f)   // 设计稿 2:3 海报卡
                    .background(
                        if (bmp != null) Color(0xFFDDDDDE) else Color(0xFFC8C8CC),
                        RoundedCornerShape(10.dp)
                    ),
                contentAlignment = Alignment.Center
            ) {
                if (bmp != null) {
                    Image(
                        bitmap = bmp.asImageBitmap(),
                        contentDescription = item.name,
                        contentScale = ContentScale.Crop,
                        modifier = Modifier.fillMaxSize()
                    )
                } else {
                    Text("🎬", fontSize = 28.sp)
                }
            }
            Text(item.name, fontSize = 12.sp, fontWeight = FontWeight.Medium,
                color = Color(0xFF1A1A1A), maxLines = 1,
                modifier = Modifier.padding(top = 4.dp))
            Text(item.year?.toString() ?: "", fontSize = 11.sp,
                color = Color(0xFF8A8A8E))
        }
    }
}
