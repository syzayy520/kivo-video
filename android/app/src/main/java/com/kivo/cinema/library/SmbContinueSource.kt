package com.kivo.cinema.library

import com.kivo.cinema.stats.WatchStore

/**
 * 首页「继续观看」的 NAS(SMB) 侧来源。
 *
 * 单一职责：从 [WatchStore] 里筛出 `smb://` 键，转成 [HomeEntry]。
 * 不扫目录（那是 NasLibraryScanner 的活）、不碰 UI；只读续播记录，不写。
 *
 * 【为什么首页不要手机本地视频】用户裁决：手机里的视频多数不是可刮削的电影，
 * 混进首页会把版面拉丑。所以首页只聚合 NAS + Emby，手机 MediaStore 的续播
 * 继续留在「在看」页（那边本来就是本地进度管理）。
 */
object SmbContinueSource {

    fun recent(watch: WatchStore, limit: Int = ContinueWatching.DEFAULT_LIMIT): List<HomeEntry> =
        watch.recentKeys(limit).mapNotNull { key ->
            val parts = SmbContinueKey.parse(key) ?: return@mapNotNull null
            val progress = watch.progressOf(key) ?: return@mapNotNull null
            HomeEntry(
                key = key,
                title = MediaTitleParser.parse(parts.fileName).title,
                sourceLabel = "NAS",
                addedSec = 0L,
                positionMs = progress.positionMs,
                durationMs = progress.durationMs,
                kind = HomeEntry.Kind.Smb,
                lastPlayedMs = progress.updatedAtMs,
            )
        }
}
