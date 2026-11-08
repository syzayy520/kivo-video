package com.kivo.cinema.ui.detail

import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.library.TitleMatch
import java.io.File

/**
 * 本地片源匹配（240 §9.2）。
 *
 * 唯一职责：把「本地视频列表 + TMDB 标题」组装成一个可展示的 [Matched]，
 * 并推导容量 / 码率 / 清晰度。不做查询（[com.kivo.cinema.library.LocalVideoSource]）、
 * 不做匹配算法（[TitleMatch]）、不碰 Compose。
 */
object MediaMatch {

    data class Matched(
        val item: MediaItem,
        val sizeBytes: Long,
        val bitrateBps: Long,
        val quality: String,     // 4K / 1080P / 720P / SD
        val resolution: String,  // 3840 × 2160
    )

    /** 找不到匹配或文件不可读时返回 null（调用方走空态）。 */
    fun find(items: List<MediaItem>, title: String): Matched? {
        val hit = items.firstOrNull { TitleMatch.matches(it.title, title) } ?: return null
        val size = runCatching { File(hit.path).length() }.getOrDefault(0L)
        if (size <= 0L) return null          // 文件不存在 → 视为无资源
        val sec = (hit.durationMs / 1000L).coerceAtLeast(1L)
        return Matched(
            item = hit,
            sizeBytes = size,
            bitrateBps = size * 8L / sec,
            quality = qualityOf(hit.height),
            resolution = "${hit.width} × ${hit.height}",
        )
    }

    private fun qualityOf(height: Int): String = when {
        height >= 2000 -> "4K"
        height >= 1000 -> "1080P"
        height >= 700 -> "720P"
        else -> "SD"
    }
}
