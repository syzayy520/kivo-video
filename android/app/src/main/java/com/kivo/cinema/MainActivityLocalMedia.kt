package com.kivo.cinema

import android.content.Context
import android.database.Cursor
import android.media.ThumbnailUtils
import android.provider.MediaStore as MS
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.unit.sp
import com.kivo.cinema.library.MediaItem

/**
 * 本地媒体索引：MediaStore 扫描 + 缩略图缓存。
 * 唯一职责 = 把设备上的本地视频变成 [MediaItem] 列表与海报块；不含路由与播放。
 * 缩略图缓存：ThumbnailUtils 逐条 MINI_KIND + HashMap——v1 明示。
 */
internal class LocalMediaIndex(private val context: Context) {

    private val thumbCache = HashMap<Long, android.graphics.Bitmap?>()

    /** 扫描结果：媒体库不可读时 [visibleCount] 为 null（首页据此显示未知）。 */
    class Scan(val items: List<MediaItem>, val visibleCount: Int?)

    /** 扫全量本地视频，并算出去掉私域库后的可见数。 */
    fun scan(): Scan {
        val list = mutableListOf<MediaItem>()
        var readable = false
        val projection = arrayOf(
            MS.Video.Media._ID, MS.Video.Media.DISPLAY_NAME, MS.Video.Media.DATA,
            MS.Video.Media.DURATION, MS.Video.Media.WIDTH, MS.Video.Media.HEIGHT,
            MS.Video.Media.DATE_ADDED,
        )
        runCatching {
            context.contentResolver.query(
                MS.Video.Media.EXTERNAL_CONTENT_URI, projection, null, null,
                "${MS.Video.Media.DATE_ADDED} DESC"
            )?.use { c: Cursor ->
                readable = true
                while (c.moveToNext()) {
                    list.add(MediaItem(
                        id = c.getLong(0), title = c.getString(1) ?: "",
                        path = c.getString(2) ?: "", durationMs = c.getLong(3),
                        width = c.getInt(4), height = c.getInt(5),
                        addedSec = c.getLong(6),
                    ))
                }
            }
        }
        val privateIds = com.kivo.cinema.privatezone.PrivateStore(context).all()
        val visible = if (readable) list.count { it.id.toString() !in privateIds } else null
        return Scan(list, visible)
    }

    /** 海报块：命中缓存直接给图，取不到就落占位方块（不静默留白）。 */
    @Composable
    fun posterBlock(item: MediaItem): @Composable () -> Unit {
        val bmp = remember(item.id) {
            thumbCache.getOrPut(item.id) {
                runCatching {
                    ThumbnailUtils.createVideoThumbnail(item.path,
                        MS.Video.Thumbnails.MINI_KIND)
                }.getOrNull()
            }
        }
        return {
            if (bmp != null) {
                Image(bmp.asImageBitmap(), item.title,
                    contentScale = ContentScale.Crop, modifier = Modifier.fillMaxSize())
            } else {
                Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                    Text("🎬", fontSize = 26.sp)
                }
            }
        }
    }
}

/** 扫描结果落到 Activity 状态：本地视频列表 + 首页可见数。 */
internal fun MainActivity.scanLocalVideos() {
    val scan = localMedia.scan()
    items = scan.items
    shellState.localVideoCount = scan.visibleCount
}
