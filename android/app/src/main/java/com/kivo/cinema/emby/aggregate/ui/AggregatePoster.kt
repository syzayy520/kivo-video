package com.kivo.cinema.emby.aggregate.ui

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
import com.kivo.cinema.emby.aggregate.AggregateSource
import com.kivo.cinema.ui.design.decodeImageCapped
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/** 聚合海报共享有界缓存：跨结果卡/类型卡复用，按字节限制，不无界堆位图。 */
private val posterCache = object : LruCache<String, Bitmap>(24 * 1024 * 1024) {
    override fun sizeOf(key: String, value: Bitmap) = value.allocationByteCount
}

/**
 * 256 聚合海报：用**结果所属源**的 client+token 拉图（不是单源会话），解码封顶复用既有 capped 解码。
 * 单一职责：聚合场景的海报加载与显示；加载中/无图给中性占位，不放假图。
 */
@Composable
fun AggregatePoster(
    source: AggregateSource, url: String, modifier: Modifier = Modifier,
    shape: Shape = RoundedCornerShape(12.dp),
) {
    var bitmap by remember(url) { mutableStateOf(posterCache.get(url)) }
    LaunchedEffect(url) {
        if (bitmap != null || url.isBlank()) return@LaunchedEffect
        bitmap = withContext(Dispatchers.IO) {
            posterCache.get(url) ?: runCatching {
                decodeImageCapped(source.client.fetchBytes(url, source.auth.token))
            }.getOrNull()?.also { posterCache.put(url, it) }
        }
    }
    Box(modifier.clip(shape).background(Color(0xFFE8E6EF))) {
        val bmp = bitmap
        if (bmp != null) Image(bmp.asImageBitmap(), contentDescription = null,
            contentScale = ContentScale.Crop, modifier = Modifier.fillMaxSize())
    }
}