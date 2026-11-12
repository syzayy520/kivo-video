package com.kivo.cinema.privatezone

import android.content.Intent
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
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.*
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
import com.kivo.cinema.privacy.PrivacyLockFlow
import com.kivo.cinema.privacy.PrivacyLockMode
import com.kivo.cinema.privacy.PrivacyPassword
import com.kivo.cinema.sources.access.SourceAccessSession

/**
 * 私密区海报墙（39 卡重构）：应用内独立数字密码解锁后只渲染 private.json 里的条目。
 * 门在入口（读 SourceAccessSession，与隐藏源共用一次解锁），播放层无感知（kivo.media 照常）。
 * 网格规格 = 222 媒体库同款（2:3 卡 3 列）。已解锁挂 FLAG_SECURE，防截图 / 最近任务泄露。
 */
class PrivateActivity : ComponentActivity() {

    private val store by lazy { PrivateStore(this) }
    private val password by lazy { PrivacyPassword(this) }
    private var items by mutableStateOf(listOf<com.kivo.cinema.library.MediaItem>())
    private var menuFor by mutableStateOf<com.kivo.cinema.library.MediaItem?>(null)
    private var showChange by mutableStateOf(false)
    private var status by mutableStateOf("")

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        SourceAccessSession.install(application)
        setContent { PrivateScreen() }
    }

    override fun onResume() {
        super.onResume()
        if (SourceAccessSession.unlocked) scanPrivateVideos()
    }

    private fun scanPrivateVideos() {
        val wanted = store.all()
        val list = mutableListOf<com.kivo.cinema.library.MediaItem>()
        if (wanted.isNotEmpty()) {
            val projection = arrayOf(
                MS.Video.Media._ID, MS.Video.Media.DISPLAY_NAME, MS.Video.Media.DATA,
                MS.Video.Media.DURATION, MS.Video.Media.WIDTH, MS.Video.Media.HEIGHT,
            )
            runCatching {
                contentResolver.query(
                    MS.Video.Media.EXTERNAL_CONTENT_URI, projection, null, null,
                    "${MS.Video.Media.DATE_ADDED} DESC"
                )?.use { cursor: Cursor ->
                    while (cursor.moveToNext()) {
                        val id = cursor.getLong(0).toString()
                        if (wanted.contains(id)) {
                            list.add(com.kivo.cinema.library.MediaItem(
                                id = cursor.getLong(0), title = cursor.getString(1) ?: "",
                                path = cursor.getString(2) ?: "", durationMs = cursor.getLong(3),
                                width = cursor.getInt(4), height = cursor.getInt(5),
                            ))
                        }
                    }
                }
            }
        }
        items = list
        status = "已解锁：${list.size} 个私密条目"
    }

    private fun openPlayer(item: com.kivo.cinema.library.MediaItem) {
        startActivity(
            Intent(this, com.kivo.cinema.ui.compose.PlayerComposeActivity::class.java)
                .putExtra("kivo.media", item.path))
    }

    @Composable
    private fun PrivateScreen() {
        val unlocked = SourceAccessSession.unlocked
        SideEffect {
            if (unlocked) window.addFlags(android.view.WindowManager.LayoutParams.FLAG_SECURE)
            else window.clearFlags(android.view.WindowManager.LayoutParams.FLAG_SECURE)
        }
        MaterialTheme(colorScheme = darkColorScheme(
            primary = Color(0xFF34C759), background = Color(0xFF1C1C1E),
            surface = Color(0xFF2C2C2E), onSurface = Color.White, onSurfaceVariant = Color(0xFF98989E),
        )) {
            val bg = Modifier.background(MaterialTheme.colorScheme.background)
            when {
                !unlocked -> PrivacyLockFlow(password,
                    mode = if (password.isSet) PrivacyLockMode.UNLOCK else PrivacyLockMode.SETUP,
                    onDone = { SourceAccessSession.unlock(); scanPrivateVideos() },
                    onCancel = { finish() }, modifier = bg)
                showChange -> PrivacyLockFlow(password, PrivacyLockMode.CHANGE,
                    onDone = { showChange = false; SourceAccessSession.lock() },
                    onCancel = { showChange = false }, modifier = bg)
                else -> PrivateWall()
            }
        }
    }

    @Composable
    private fun PrivateWall() {
        Column(Modifier.fillMaxSize().background(MaterialTheme.colorScheme.background).padding(12.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text("私密区", fontSize = 28.sp, fontWeight = FontWeight.Bold, color = Color.White)
                Spacer(Modifier.weight(1f))
                Text("修改密码", color = Color(0xFF34C759), fontSize = 13.sp,
                    modifier = Modifier.clickable { showChange = true }.padding(6.dp))
                Text("锁定", color = Color(0xFF34C759), fontSize = 13.sp,
                    modifier = Modifier.clickable { SourceAccessSession.lock() }.padding(6.dp))
            }
            Spacer(Modifier.height(4.dp))
            Text(status, fontSize = 13.sp, color = Color(0xFF98989E))
            Spacer(Modifier.height(8.dp))
            if (items.isNotEmpty()) {
                LazyVerticalGrid(columns = GridCells.Fixed(3),
                    verticalArrangement = Arrangement.spacedBy(10.dp),
                    horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                    items(items) { item ->
                        PosterCard(item, onLongClick = { menuFor = item }, onClick = { openPlayer(item) })
                    }
                }
            } else {
                Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                    Text("私密区为空——在媒体库长按视频可加入", fontSize = 14.sp, color = Color(0xFF98989E))
                }
            }
        }
        menuFor?.let { target ->
            AlertDialog(
                onDismissRequest = { menuFor = null },
                title = { Text(target.title, fontSize = 16.sp, maxLines = 1) },
                text = { Text("将这条视频移出私密区？（移出后回到普通媒体库）", fontSize = 13.sp) },
                confirmButton = {
                    Button(onClick = {
                        store.remove(target.id.toString()); scanPrivateVideos(); menuFor = null
                    }) { Text("移出私密区", fontSize = 13.sp) }
                },
                dismissButton = {
                    Text("取消", fontSize = 13.sp, color = Color(0xFF98989E),
                        modifier = Modifier.clickable { menuFor = null }.padding(8.dp))
                },
            )
        }
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
