package com.kivo.cinema.library

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.database.Cursor
import android.os.Bundle
import android.provider.MediaStore as MS
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.GlassCircleButton
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 本地媒体库宿主：MediaStore 查询 + 权限 + 私密标记状态。
 * 网格渲染（SU-01 2 列白卡）在 [com.kivo.cinema.library.PosterWall]（单职责拆分）。
 * D1 MediaStore 查询：content://media/external/video → [MediaItem] 列表。
 * 点击 → 详情页（详情内直接播放）。
 */
class MediaLibraryActivity : ComponentActivity() {

    private var items by mutableStateOf(listOf<MediaItem>())
    private var status by mutableStateOf("请求权限中…")
    private var permissionGranted by mutableStateOf(false)

    // 231 私密区：标记存储 + 重组计数（增删后 ++ 触发主网格过滤重算）
    private val privStore by lazy { com.kivo.cinema.privatezone.PrivateStore(this) }
    private var privEpoch by mutableStateOf(0)
    private var menuFor by mutableStateOf<MediaItem?>(null)
    private var sortDesc by mutableStateOf(true)

    private val permissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { granted ->
        permissionGranted = granted
        if (granted) {
            scanVideos()
        } else {
            status = "未授权视频访问——请在系统设置中开启"
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent { MediaLibraryScreen() }
        requestPermission()
    }

    override fun onResume() {
        super.onResume()
        // 私密区里可能加/移出过条目——回库强制重算过滤与计数（231 跨页状态同步）
        privEpoch++
        status = "本地视频 ${visibleItems().size} 个"
    }

    private fun requestPermission() {
        val perm = if (android.os.Build.VERSION.SDK_INT >= 33) {
            Manifest.permission.READ_MEDIA_VIDEO
        } else {
            Manifest.permission.READ_EXTERNAL_STORAGE
        }
        if (checkSelfPermission(perm) == PackageManager.PERMISSION_GRANTED) {
            permissionGranted = true
            scanVideos()
        } else {
            permissionLauncher.launch(perm)
        }
    }

    private fun scanVideos() {
        val list = mutableListOf<MediaItem>()
        val projection = arrayOf(
            MS.Video.Media._ID,
            MS.Video.Media.DISPLAY_NAME,
            MS.Video.Media.DATA,
            MS.Video.Media.DURATION,
            MS.Video.Media.WIDTH,
            MS.Video.Media.HEIGHT,
            MS.Video.Media.DATE_ADDED,
        )
        try {
            contentResolver.query(
                MS.Video.Media.EXTERNAL_CONTENT_URI,
                projection, null, null,
                "${MS.Video.Media.DATE_ADDED} " + if (sortDesc) "DESC" else "ASC"
            )?.use { cursor: Cursor ->
                while (cursor.moveToNext()) {
                    list.add(MediaItem(
                        id = cursor.getLong(0),
                        title = cursor.getString(1) ?: "",
                        path = cursor.getString(2) ?: "",
                        durationMs = cursor.getLong(3),
                        width = cursor.getInt(4),
                        height = cursor.getInt(5),
                        addedSec = cursor.getLong(6),
                    ))
                }
            }
        } catch (e: Exception) {
            status = "扫描失败：${e.message}"
            return
        }
        items = list
        status = "本地视频 ${list.size} 个"
    }

    private fun openPlayer(item: MediaItem) {
        // 237-C：海报点 → 详情页（详情内直接播放）；D12：与首页/搜索同一详情入口。
        startActivity(com.kivo.cinema.ui.detail.MovieDetailActivity.localIntent(this, item))
    }

    // ---- 231 私密区 ----

    private fun openPrivateZone() {
        // 39：门改为应用内独立数字密码（在 PrivateActivity 内），不再依赖设备生物特征，直达即可。
        startActivity(Intent(this, com.kivo.cinema.privatezone.PrivateActivity::class.java))
    }

    private fun togglePrivate(item: MediaItem) {
        val id = item.id.toString()
        if (privStore.contains(id)) {
            privStore.remove(id)
        } else {
            privStore.add(id)
        }
        privEpoch++
        menuFor = null
        status = "私密区标记已更新：${privStore.all().size} 个"
    }

    /** 主网格可见条目 = 全量 − 私密（231 设计稿 D3：标记后即时隐藏）。 */
    private fun visibleItems(): List<MediaItem> {
        val hidden = privStore.all()
        return if (hidden.isEmpty()) items
        else items.filterNot { hidden.contains(it.id.toString()) }
    }

    // ---- UI ----

    @Composable
    private fun MediaLibraryScreen() {
        // lib_01/lib_02 帧规格：网格全屏铺底，控制钮全部悬浮
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(Color(0xFFF2F2F7))
        ) {
            if (permissionGranted && items.isNotEmpty()) {
                PosterWall()
            } else if (permissionGranted) {
                Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                    Text("没有找到视频文件", fontSize = 15.sp, color = Color(0xFF8A8A8E))
                }
            }
            // 顶部：毛玻璃返回钮（左）+ 标题（中）
            Row(
                Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 13.dp, vertical = 10.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                GlassCircleButton(
                    modifier = Modifier.clickable { finish() }
                ) { Text("‹", fontSize = 22.sp, color = Color(0xFF1A1A1A)) }
                Spacer(Modifier.weight(1f))
                Text("本地影片", fontSize = 17.sp, fontWeight = FontWeight.Bold,
                    color = Color(0xFF1A1A1A))
                Spacer(Modifier.weight(1f))
                Spacer(Modifier.width(DesignTokens.circleButton.dp))
            }
            // 底部右：排序浮钮（真实功能：日期升/降切换）
            GlassCircleButton(
                modifier = Modifier
                    .align(Alignment.BottomEnd)
                    .padding(13.dp)
                    .clickable { sortDesc = !sortDesc; scanVideos() }
            ) { Text(if (sortDesc) "↓" else "↑", fontSize = 20.sp, color = Color(0xFF1A1A1A)) }
            // 底部左：状态行
            Text(status, fontSize = 13.sp, color = Color(0xFF8A8A8E),
                modifier = Modifier
                    .align(Alignment.BottomStart)
                    .padding(13.dp))
        }
        // 231：长按标记菜单
        menuFor?.let { target ->
            androidx.compose.material3.AlertDialog(
                onDismissRequest = { menuFor = null },
                title = { Text(target.title, fontSize = 16.sp, maxLines = 1) },
                text = { Text("将这条视频从私密区加入或移出？", fontSize = 13.sp) },
                confirmButton = {
                    Button(onClick = { togglePrivate(target) }) {
                        Text(if (privStore.contains(target.id.toString())) "移出私密区"
                             else "加入私密区", fontSize = 13.sp)
                    }
                },
                dismissButton = {
                    Text("取消", fontSize = 13.sp, color = Color(0xFF8A8A8E),
                        modifier = Modifier
                            .clickable { menuFor = null }
                            .padding(8.dp))
                }
            )
        }
    }

    @Composable
    private fun PosterWall() {
        // privEpoch 参与取值：增删标记后强制重算过滤（231 即时隐藏）
        val visible = remember(privEpoch, items) { visibleItems() }
        com.kivo.cinema.library.PosterWall(
            visible = visible,
            onOpen = { openPlayer(it) },
            onLongClick = { menuFor = it },
        )
    }
}

internal fun formatDate(addedSec: Long): String {
    val d = java.util.Calendar.getInstance()
    d.timeInMillis = addedSec * 1000L
    return "%04d-%02d-%02d".format(d.get(java.util.Calendar.YEAR),
        d.get(java.util.Calendar.MONTH) + 1, d.get(java.util.Calendar.DAY_OF_MONTH))
}
