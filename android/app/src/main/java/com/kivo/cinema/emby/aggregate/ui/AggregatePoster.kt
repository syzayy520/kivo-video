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
        // D371 取证：区分「源本来没有海报」与「取图失败」—— 这两种都是灰块，但责任方不同。
        if (url.isBlank()) {
            com.kivo.cinema.net.NetDiag.log("[agg-poster] blank ${source.record.name}")
            return@LaunchedEffect
        }
        if (bitmap != null) return@LaunchedEffect
        bitmap = withContext(Dispatchers.IO) {
            posterCache.get(url) ?: runCatching {
                val bytes = source.client.fetchBytes(url, source.auth.token)
                decodeImageCapped(bytes) ?: run {
                    // 拿到了字节却解不出图：格式异常/被网关替换成 HTML 等，必须留痕（不静默）。
                    com.kivo.cinema.net.NetDiag.log(
                        "[agg-poster] decode-null bytes=${bytes.size} ${url.takeLast(24)}",
                    )
                    null
                }
            }.onFailure {
                com.kivo.cinema.net.NetDiag.log(
                    "[agg-poster] fail ${it.javaClass.simpleName}: ${it.message} ${url.takeLast(24)}",
                )
            }.getOrNull()?.also { posterCache.put(url, it) }
        }
    }
    Box(modifier.clip(shape).background(Color(0xFFE8E6EF))) {
        val bmp = bitmap
        if (bmp != null) Image(bmp.asImageBitmap(), contentDescription = null,
            contentScale = ContentScale.Crop, modifier = Modifier.fillMaxSize())
    }
}