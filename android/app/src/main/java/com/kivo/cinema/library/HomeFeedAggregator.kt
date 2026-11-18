package com.kivo.cinema.library

import com.kivo.cinema.stats.WatchStore

/**
 * 首页「继续观看」的多源聚合（本地文件 / SMB / Emby）。
 *
 * 单一职责：把各源**已取好**的条目合并成一份有序列表 —— 排序（最近观看优先）、
 * 按 [HomeEntry.key] 去重、截断到上限。不取数、不碰 UI。
 *
 * 排序口径：`lastPlayedMs` 降序。本地有真实观看时刻；Emby 无时间戳（记 0）⇒
 * 稳定排在本地之后——这是刻意的，不做"伪造时间"去抢本地位置。
 */
object HomeFeedAggregator {

    fun merge(
        local: List<Pair<MediaItem, WatchStore.Progress>>,
        emby: List<HomeEntry>,
        limit: Int = ContinueWatching.DEFAULT_LIMIT,
    ): List<HomeEntry> =
        (local.map { (item, p) -> item.toHomeEntry(p) } + emby)
            .sortedByDescending { it.lastPlayedMs }
            .distinctBy { it.key }
            .take(limit)

    private fun MediaItem.toHomeEntry(p: WatchStore.Progress): HomeEntry {
        val smb = path.startsWith(SMB_PREFIX)
        return HomeEntry(
            key = ContinueWatching.normPath(path),
            title = title,
            sourceLabel = if (smb) "SMB" else "本地",
            addedSec = addedSec,
            positionMs = p.positionMs,
            durationMs = if (p.durationMs > 0) p.durationMs else durationMs,
            kind = if (smb) HomeEntry.Kind.Smb else HomeEntry.Kind.Local,
            mediaItem = this,
            lastPlayedMs = p.updatedAtMs,
        )
    }

    private const val SMB_PREFIX = "smb://"
}
