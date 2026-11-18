package com.kivo.cinema.library

/**
 * NAS 片库索引 → 首页条目。
 *
 * 单一职责：把 [NasVideo] 映射成 [HomeEntry]（片名解析 + 来源标签），不读磁盘、不扫描、不碰 UI。
 * 语义与「继续观看」不同：这里没有播放进度，是"库里有什么"（P3 的「本地片库」行）。
 */
object NasLibrarySource {

    fun entries(videos: List<NasVideo>, limit: Int = 12): List<HomeEntry> =
        videos.take(limit).map { v ->
            HomeEntry(
                key = v.watchKey,
                title = MediaTitleParser.parse(v.fileName).title,
                sourceLabel = "NAS",
                addedSec = if (v.addedMs > 0) v.addedMs / 1000 else 0L,
                positionMs = 0L,
                durationMs = 0L,
                kind = HomeEntry.Kind.Smb,
                lastPlayedMs = v.addedMs,
            )
        }
}
