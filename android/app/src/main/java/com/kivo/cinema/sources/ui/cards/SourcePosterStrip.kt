package com.kivo.cinema.sources.ui.cards

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
import com.kivo.cinema.emby.EmbyItem
import com.kivo.cinema.ui.design.TmdbImageCache
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * 资源库服务器卡的海报条：最近影片横向缩略图，共享有界位图缓存。
 * 单一职责：拉图 + 显示 + 占位；取数/revision 在 SourceSummaries，不在此。
 * 【D369】原来自带一个只有内存的 LruCache ⇒ 每次冷启动都要把每源 6 张海报重下；
 * 改用 [TmdbImageCache]（内存 + 磁盘 + 传输失败重试，且支持 X-Emby-Token），删掉重复实现。
 * 加载失败给中性占位，不放假图，不阻塞卡片其余交互。
 */
@Composable
internal fun SourcePosterStrip(
    token: String?, posters: List<EmbyItem>,
    posterW: Dp = 42.dp, posterH: Dp = 60.dp,
) {
    if (posters.isEmpty()) return
    Row {
        posters.forEachIndexed { idx, item ->
            if (idx > 0) Spacer(Modifier.width(6.dp))
            SourcePosterThumb(token, item.posterUrl, posterW, posterH)
        }
    }
}

@Composable
private fun SourcePosterThumb(token: String?, url: String, w: Dp, h: Dp) {
    var bitmap by remember(url) { mutableStateOf(TmdbImageCache.get(url)) }
    LaunchedEffect(url) {
        if (bitmap != null || url.isBlank()) return@LaunchedEffect
        bitmap = withContext(Dispatchers.IO) { TmdbImageCache.load(url, token) }
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
