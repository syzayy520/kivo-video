package com.kivo.cinema.ui.design.home.sections

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
import com.kivo.cinema.emby.EmbyAuthCache
import com.kivo.cinema.ui.design.TmdbImageCache
import com.kivo.cinema.library.HomeEntry
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/** 首页 Emby 续播海报的有界缓存（跨卡片复用，按字节限，不无界堆位图）。 */
/**
 * 首页「继续观看」里的 Emby 海报。
 *
 * 单一职责：只负责"把 Emby 条目的一张海报取来显示"。token 由 [EmbyAuthCache]
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
    var bitmap by remember(url) { mutableStateOf(url?.let { TmdbImageCache.get(it) }) }
    LaunchedEffect(url) {
        if (bitmap != null || url.isNullOrBlank()) return@LaunchedEffect
        val token = EmbyAuthCache.session(entry.embySourceId ?: "")?.second?.token
        bitmap = withContext(Dispatchers.IO) { TmdbImageCache.load(url, token) }
    }
    Box(modifier.clip(shape).background(Color(0xFFE8E6EF))) {
        val bmp = bitmap
        if (bmp != null) {
            Image(bmp.asImageBitmap(), contentDescription = null,
                contentScale = ContentScale.Crop, modifier = Modifier.fillMaxSize())
        }
    }
}
