package com.kivo.cinema.sources.ui.cards

import android.graphics.Bitmap
import android.util.LruCache
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
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
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import com.kivo.cinema.emby.EmbyClient
import com.kivo.cinema.emby.EmbyItem
import com.kivo.cinema.ui.design.decodeImageCapped
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * 资源库服务器卡的海报条：最近影片横向缩略图，共享有界位图缓存。
 * 单一职责：拉图 + 显示 + 占位；取数Revision/缓存策略在 SourceSummaries，不在此。
 * 加载失败给中性占位，不放假图，不阻塞卡片其余交互。
 */
private val sourcePosterCache = object : LruCache<String, Bitmap>(12 * 1024 * 1024) {
    override fun sizeOf(key: String, value: Bitmap) = value.allocationByteCount
}

@Composable
internal fun SourcePosterStrip(
    address: String, token: String?, posters: List<EmbyItem>,
    posterW: Dp = 42.dp, posterH: Dp = 60.dp,
) {
    if (posters.isEmpty()) return
    Row {
        posters.forEachIndexed { idx, item ->
            if (idx > 0) Spacer(Modifier.width(6.dp))
            SourcePosterThumb(address, token, item.posterUrl, posterW, posterH)
        }
    }
}

@Composable
private fun SourcePosterThumb(
    address: String, token: String?, url: String, w: Dp, h: Dp,
) {
    var bitmap by remember(url) { mutableStateOf(sourcePosterCache.get(url)) }
    LaunchedEffect(url) {
        if (bitmap != null || url.isBlank()) return@LaunchedEffect
        bitmap = withContext(Dispatchers.IO) {
            sourcePosterCache.get(url) ?: runCatching {
                decodeImageCapped(EmbyClient(address).fetchBytes(url, token), maxDim = 480)
            }.getOrNull()?.also { sourcePosterCache.put(url, it) }
        }
    }
    val shape = RoundedCornerShape(7.dp)
    androidx.compose.foundation.layout.Box(
        Modifier.size(w, h).clip(shape).background(Color(0xFFE8E6EF)),
    ) {
        bitmap?.let { bmp ->
            Image(bmp.asImageBitmap(), contentDescription = null,
                contentScale = ContentScale.Crop, modifier = Modifier.size(w, h))
        }
    }
}
