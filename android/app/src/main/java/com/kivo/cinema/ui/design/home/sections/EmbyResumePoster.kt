package com.kivo.cinema.ui.design.home.sections

import android.graphics.Bitmap
import android.util.LruCache
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Shape
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.unit.dp
import com.kivo.cinema.emby.EmbyResumeSession
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.ui.design.decodeImageCapped
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/** 首页 Emby 续播海报的有界缓存（跨卡片复用，按字节限，不无界堆位图）。 */
private val embyResumeCache = object : LruCache<String, Bitmap>(16 * 1024 * 1024) {
    override fun sizeOf(key: String, value: Bitmap) = value.allocationByteCount
}

/**
 * 首页「继续观看」里的 Emby 海报。
 *
 * 单一职责：只负责"把 Emby 条目的一张海报取来显示"。token 由 [EmbyResumeSession]
 * 按 sourceId 复用（取数时已登录），本组件不碰凭据细节。
 * 加载中/失败给中性占位 —— 不放假图、不白块、不抛异常。
 */
@Composable
internal fun EmbyResumePoster(
    entry: HomeEntry,
    modifier: Modifier = Modifier,
    shape: Shape = RoundedCornerShape(12.dp),
) {
    val url = entry.posterUrl
    var bitmap by remember(url) { mutableStateOf(url?.let { embyResumeCache.get(it) }) }
    LaunchedEffect(url) {
        if (bitmap != null || url.isNullOrBlank()) return@LaunchedEffect
        bitmap = withContext(Dispatchers.IO) {
            embyResumeCache.get(url) ?: runCatching {
                EmbyResumeSession.fetchBytes(entry.embySourceId, url)?.let { decodeImageCapped(it) }
            }.getOrNull()?.also { embyResumeCache.put(url, it) }
        }
    }
    Box(modifier.clip(shape).background(Color(0xFFE8E6EF))) {
        val bmp = bitmap
        if (bmp != null) {
            Image(bmp.asImageBitmap(), contentDescription = null,
                contentScale = ContentScale.Crop, modifier = Modifier.fillMaxSize())
        }
    }
}
