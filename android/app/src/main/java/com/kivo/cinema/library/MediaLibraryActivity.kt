package com.kivo.cinema.library

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.database.Cursor
import android.net.Uri
import android.os.Bundle
import android.provider.MediaStore as MS
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
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
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
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
import androidx.compose.foundation.layout.height

/**
 * 本地媒体库（222 设计稿）：MediaStore 查询 + LazyVerticalGrid 海报墙。
 *
 * Rex 规格（§4）：卡片 2:3、3 列网格、标题 12sp 粗体 + 元信息 10sp 灰。
 * D1 MediaStore 查询：content://media/external/video → [MediaItem] 列表。
 * D2 缩略图：MediaStore.Video.Thumbnails（系统缓存）。
 * 点击 → PlayerComposeActivity（213 管线）。
 */
class MediaLibraryActivity : ComponentActivity() {

    private var items by mutableStateOf(listOf<MediaItem>())
    private var status by mutableStateOf("请求权限中…")
    private var permissionGranted by mutableStateOf(false)

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
        )
        try {
            contentResolver.query(
                MS.Video.Media.EXTERNAL_CONTENT_URI,
                projection, null, null,
                "${MS.Video.Media.DATE_ADDED} DESC"
            )?.use { cursor: Cursor ->
                while (cursor.moveToNext()) {
                    list.add(MediaItem(
                        id = cursor.getLong(0),
                        title = cursor.getString(1) ?: "",
                        path = cursor.getString(2) ?: "",
                        durationMs = cursor.getLong(3),
                        width = cursor.getInt(4),
                        height = cursor.getInt(5),
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
        val intent = Intent(this, com.kivo.cinema.ui.compose.PlayerComposeActivity::class.java)
            .putExtra("kivo.media", item.path)
        startActivity(intent)
    }

    private fun openSmb() {
        startActivity(Intent(this, com.kivo.cinema.smb.SmbActivity::class.java))
    }

    // ---- UI ----

    @Composable
    private fun MediaLibraryScreen() {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .background(Color(0xFFF2F2F7))
                .padding(12.dp)
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text("媒体库", fontSize = 28.sp, fontWeight = FontWeight.Bold,
                    color = Color(0xFF1A1A1A))
                Spacer(Modifier.weight(1f))
                Button(
                    onClick = { openSmb() },
                    colors = androidx.compose.material3.ButtonDefaults.buttonColors(
                        containerColor = Color(0xFF34C759)
                    ),
                    contentPadding = androidx.compose.foundation.layout.PaddingValues(
                        horizontal = 14.dp, vertical = 6.dp)
                ) { Text("SMB", fontSize = 13.sp) }
            }
            Spacer(Modifier.height(4.dp))
            Text(status, fontSize = 13.sp, color = Color(0xFF8A8A8E))
            Spacer(Modifier.height(8.dp))
            if (permissionGranted && items.isNotEmpty()) {
                PosterWall()
            } else if (permissionGranted) {
                Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                    Text("没有找到视频文件", fontSize = 15.sp, color = Color(0xFF8A8A8E))
                }
            }
        }
    }

    @Composable
    private fun PosterWall() {
        LazyVerticalGrid(
            columns = GridCells.Fixed(3),
            verticalArrangement = Arrangement.spacedBy(10.dp),
            horizontalArrangement = Arrangement.spacedBy(10.dp),
        ) {
            items(items) { item ->
                PosterCard(item) { openPlayer(item) }
            }
        }
    }

    @Composable
    private fun PosterCard(item: MediaItem, onClick: () -> Unit) {
        Column(
            modifier = Modifier.clickable { onClick() }
        ) {
            // 缩略图：MediaStore thumbnail
            val bitmap = android.media.ThumbnailUtils.createVideoThumbnail(
                item.path, MS.Video.Thumbnails.MINI_KIND
            )
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .aspectRatio(2f / 3f)   // Rex §4: 2:3
                    .clip(RoundedCornerShape(8.dp))
                    .background(Color(0xFFE0E0E0))
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
                        Text("🎬", fontSize = 32.sp)
                    }
                }
                // 时长角标
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
            Text(
                item.title,
                fontSize = 12.sp, fontWeight = FontWeight.Medium,
                color = Color(0xFF1A1A1A),
                maxLines = 1, overflow = TextOverflow.Ellipsis
            )
        }
    }
}
