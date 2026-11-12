package com.kivo.cinema.emby.aggregate.ui

import android.content.Intent
import android.media.ThumbnailUtils
import android.provider.MediaStore
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.ui.detail.DetailActivity

/**
 * 256 全局搜索本地卡：系统缩略图 + 本地徽标 + 时长角标 + 标题。
 * 点击复用本地库既有契约（DetailActivity kivo.media/kivo.durationMs），不新造播放入口。
 */
@Composable
fun LocalResultCard(item: MediaItem) {
    val context = LocalContext.current
    val bmp = remember(item.path) {
        runCatching {
            @Suppress("DEPRECATION")
            ThumbnailUtils.createVideoThumbnail(item.path, MediaStore.Video.Thumbnails.MINI_KIND)
        }.getOrNull()
    }
    Column(Modifier.clickable(interactionSource = remember { MutableInteractionSource() }, indication = null) {
        context.startActivity(Intent(context, DetailActivity::class.java)
            .putExtra("kivo.media", item.path)
            .putExtra("kivo.durationMs", item.durationMs))
    }) {
        Box(Modifier.fillMaxWidth().aspectRatio(2f / 3f)
            .clip(RoundedCornerShape(12.dp)).background(Color(0xFFE8E6EF))) {
            if (bmp != null) {
                Image(bmp.asImageBitmap(), item.title,
                    contentScale = ContentScale.Crop, modifier = Modifier.fillMaxSize())
            }
            Text("本地", fontSize = 10.sp, color = Color.White,
                modifier = Modifier.align(Alignment.TopStart).padding(6.dp)
                    .clip(RoundedCornerShape(6.dp)).background(Color.Black.copy(alpha = 0.55f))
                    .padding(horizontal = 6.dp, vertical = 2.dp))
            Text(item.formatDuration(), fontSize = 10.sp, color = Color.White,
                modifier = Modifier.align(Alignment.BottomEnd).padding(6.dp)
                    .clip(RoundedCornerShape(6.dp)).background(Color.Black.copy(alpha = 0.55f))
                    .padding(horizontal = 6.dp, vertical = 2.dp))
        }
        Spacer(Modifier.height(6.dp))
        Text(item.title, fontSize = 13.sp, fontWeight = FontWeight.Medium,
            maxLines = 1, overflow = TextOverflow.Ellipsis,
            color = MaterialTheme.colorScheme.onSurface)
    }
}