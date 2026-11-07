package com.kivo.cinema.privatezone

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.database.Cursor
import android.os.Bundle
import android.provider.MediaStore as MS
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 私密区海报墙（231 设计稿 D4）：BiometricPrompt 解锁后只渲染 private.json 里的条目。
 *
 * 门在入口（每次进入都验证，PrivateGate），播放层无感知（kivo.media 照常）。
 * 网格规格 = 222 媒体库同款（2:3 卡 3 列）。
 */
class PrivateActivity : ComponentActivity() {

    private val store by lazy { PrivateStore(this) }
    private val gate = PrivateGate()

    private var unlocked by mutableStateOf(false)
    private var status by mutableStateOf("验证指纹以进入私密区")
    private var items by mutableStateOf(listOf<com.kivo.cinema.library.MediaItem>())
    private var menuFor by mutableStateOf<com.kivo.cinema.library.MediaItem?>(null)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent { PrivateScreen() }
    }

    override fun onResume() {
        super.onResume()
        // AC1：每次进入都验证（返回后再进也要重新过门）
        if (!unlocked) authenticate()
    }

    private fun authenticate() {
        if (!gate.available(this)) {
            status = "设备未录入生物特征——无法进入私密区"
            return
        }
        gate.authenticate(this, "私密区") { result ->
            when (result) {
                is PrivateGate.Result.Success -> {
                    unlocked = true
                    scanPrivateVideos()
                    status = "已解锁：${items.size} 个私密条目"
                }
                is PrivateGate.Result.Failed -> {
                    status = "未进入私密区：${result.reason}"
                }
            }
        }
    }

    private fun scanPrivateVideos() {
        val wanted = store.all()
        if (wanted.isEmpty()) {
            items = emptyList()   // 移出最后一条后网格要清空（AC4 往返）
            return
        }
        val list = mutableListOf<com.kivo.cinema.library.MediaItem>()
        val projection = arrayOf(
            MS.Video.Media._ID,
            MS.Video.Media.DISPLAY_NAME,
            MS.Video.Media.DATA,
            MS.Video.Media.DURATION,
            MS.Video.Media.WIDTH,
            MS.Video.Media.HEIGHT,
        )
        runCatching {
            contentResolver.query(
                MS.Video.Media.EXTERNAL_CONTENT_URI,
                projection, null, null,
                "${MS.Video.Media.DATE_ADDED} DESC"
            )?.use { cursor: Cursor ->
                while (cursor.moveToNext()) {
                    val id = cursor.getLong(0).toString()
                    if (wanted.contains(id)) {
                        list.add(com.kivo.cinema.library.MediaItem(
                            id = cursor.getLong(0),
                            title = cursor.getString(1) ?: "",
                            path = cursor.getString(2) ?: "",
                            durationMs = cursor.getLong(3),
                            width = cursor.getInt(4),
                            height = cursor.getInt(5),
                        ))
                    }
                }
            }
        }
        items = list
    }

    private fun openPlayer(item: com.kivo.cinema.library.MediaItem) {
        startActivity(
            Intent(this, com.kivo.cinema.ui.compose.PlayerComposeActivity::class.java)
                .putExtra("kivo.media", item.path))
    }

    @Composable
    private fun PrivateScreen() {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .background(Color(0xFF1C1C1E))   // 私密空间用暗色（与播放器 chrome 一致）
                .padding(12.dp)
        ) {
            Row {
                Text("私密区", fontSize = 28.sp, fontWeight = FontWeight.Bold,
                    color = Color.White)
                Spacer(Modifier.weight(1f))
                Text("重新验证", color = Color(0xFF34C759), fontSize = 13.sp,
                    modifier = Modifier
                        .clickable {
                            unlocked = false
                            authenticate()
                        }
                        .padding(6.dp))
            }
            Spacer(Modifier.height(4.dp))
            Text(status, fontSize = 13.sp, color = Color(0xFF98989E))
            Spacer(Modifier.height(8.dp))
            if (unlocked && items.isNotEmpty()) {
                LazyVerticalGrid(
                    columns = GridCells.Fixed(3),
                    verticalArrangement = Arrangement.spacedBy(10.dp),
                    horizontalArrangement = Arrangement.spacedBy(10.dp),
                ) {
                    items(items) { item ->
                        PosterCard(item,
                            onLongClick = { menuFor = item },
                            onClick = { openPlayer(item) })
                    }
                }
            } else if (unlocked) {
                Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                    Text("私密区为空——在媒体库长按视频可加入",
                        fontSize = 14.sp, color = Color(0xFF98989E))
                }
            }
        }
        // 长按移出（AC4：加入→移出→主网格恢复的唯一路径）
        menuFor?.let { target ->
            androidx.compose.material3.AlertDialog(
                onDismissRequest = { menuFor = null },
                title = { Text(target.title, fontSize = 16.sp, maxLines = 1) },
                text = { Text("将这条视频移出私密区？（移出后回到普通媒体库）",
                    fontSize = 13.sp) },
                confirmButton = {
                    androidx.compose.material3.Button(onClick = {
                        store.remove(target.id.toString())
                        refreshPrivate()
                        menuFor = null
                    }) { Text("移出私密区", fontSize = 13.sp) }
                },
                dismissButton = {
                    Text("取消", fontSize = 13.sp, color = Color(0xFF98989E),
                        modifier = Modifier
                            .clickable { menuFor = null }
                            .padding(8.dp))
                }
            )
        }
    }

    private fun refreshPrivate() {
        scanPrivateVideos()
        status = "已解锁：${items.size} 个私密条目"
    }

    @OptIn(ExperimentalFoundationApi::class)
    @Composable
    private fun PosterCard(
        item: com.kivo.cinema.library.MediaItem,
        onLongClick: () -> Unit,
        onClick: () -> Unit) {
        Column(modifier = Modifier.combinedClickable(
            onClick = onClick, onLongClick = onLongClick)) {
            val bitmap = android.media.ThumbnailUtils.createVideoThumbnail(
                item.path, MS.Video.Thumbnails.MINI_KIND)
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .aspectRatio(2f / 3f)
                    .clip(RoundedCornerShape(8.dp))
                    .background(Color(0xFF2C2C2E))
            ) {
                if (bitmap != null) {
                    Image(
                        bitmap = bitmap.asImageBitmap(),
                        contentDescription = item.title,
                        contentScale = ContentScale.Crop,
                        modifier = Modifier.fillMaxSize()
                    )
                } else {
                    Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                        Text("🔒", fontSize = 30.sp)
                    }
                }
                Text(
                    item.formatDuration(),
                    fontSize = 10.sp,
                    color = Color.White,
                    modifier = Modifier
                        .align(Alignment.BottomEnd)
                        .padding(6.dp)
                        .background(Color.Black.copy(alpha = 0.6f),
                            RoundedCornerShape(4.dp))
                        .padding(horizontal = 4.dp, vertical = 1.dp)
                )
            }
            Spacer(Modifier.height(4.dp))
            Text(item.title, fontSize = 12.sp, fontWeight = FontWeight.Medium,
                color = Color.White, maxLines = 1, overflow = TextOverflow.Ellipsis)
        }
    }
}
