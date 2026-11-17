package com.kivo.cinema.ui.profile.thumbs

import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.produceState
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.unit.dp
import com.kivo.cinema.evidence.IoTrace
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.design.decodeImageFileCapped
import com.kivo.cinema.ui.profile.model.WatchEntry
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * 列表条目的封面槽（待看票根卡 16:9、在看海报 2:3 共用，避免两份重复解码实现）。
 *
 * 数据口径：[WatchEntry.posterPath] 是 [VideoThumbnails] 落盘的**本地缓存 jpg 绝对路径**
 * （PROF-02 冻结语义），本槽只解码本地文件；http 或空值一律退回 [DesignTokens.groupedFill]
 * 占位 —— 不引入图片加载依赖，也不画假封面。
 *
 * D11：解码在 `produceState + Dispatchers.IO` 中完成，主线程只收 [ImageBitmap]；
 * [maxDim] 仍按卡片实际像素上限裁剪，避免大图占用内存。
 */
@Composable
fun EntryCover(entry: WatchEntry, aspectRatio: Float, maxDim: Int = DefaultMaxDim) {
    val cover by produceState<ImageBitmap?>(null, entry.posterPath, maxDim) {
        value = withContext(Dispatchers.IO) {
            IoTrace.measure("profile.entryCover") { decodeCover(entry.posterPath, maxDim) }
        }
    }
    Box(
        Modifier
            .fillMaxWidth()
            .aspectRatio(aspectRatio)
            .clip(RoundedCornerShape(CoverCorner))
            .background(DesignTokens.groupedFill),
    ) {
        val coverValue = cover
        if (coverValue != null) {
            Image(
                bitmap = coverValue,
                contentDescription = entry.title,
                contentScale = ContentScale.Crop,
                modifier = Modifier.fillMaxSize(),
            )
        }
    }
}

private fun decodeCover(path: String?, maxDim: Int): ImageBitmap? {
    if (path.isNullOrBlank()) return null
    return runCatching { decodeImageFileCapped(path, maxDim)?.asImageBitmap() }.getOrNull()
}

// 封面圆角与解码上限：卡片最大边 178dp，440dpi 下 482px，取 480 留 1px 余量。
private val CoverCorner = 10.dp
private const val DefaultMaxDim = 480