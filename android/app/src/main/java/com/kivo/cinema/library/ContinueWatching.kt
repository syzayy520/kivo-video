package com.kivo.cinema.library

import android.content.Context
import com.kivo.cinema.stats.WatchStore

/**
 * 续播记录 × 本地片源的唯一对齐入口。
 *
 * 首页「继续观看」（[com.kivo.cinema.shellState.continueItems]）、发现页、在看覆盖层与
 * 「我的」看板都从这里取数：同一 key、同一过滤规则 ⇒ 同一页面上的「在看数」不会与覆盖层互相矛盾。
 *
 * 只对齐得上的记录返回（无续播位 / 无对应 [MediaItem] 的一律丢弃）——这是「不把 URL 当片名」的根：
 * 网络源 key（Emby 的 `stream?…`）永远无法与本地片源对齐，因此不会进入 UI。
 *
 * 磁盘与 MediaStore 读取是阻塞调用：请在上 `Dispatchers.IO` 调用。
 */
object ContinueWatching {

    /** 最近 N 条；与首页/覆盖层一致，看板不要再各自扩大。 */
    const val DEFAULT_LIMIT = 10

    /** `/sdcard` 与 `/storage/emulated/0` 是同一文件的两个别名，统一后比较。 */
    private const val SDCARD_PREFIX = "/sdcard/"
    private const val EMULATED_PREFIX = "/storage/emulated/0/"

    fun normPath(p: String): String =
        if (p.startsWith(SDCARD_PREFIX)) EMULATED_PREFIX + p.removePrefix(SDCARD_PREFIX) else p

    /** 现查 MediaStore 再对齐（无现成片源列表时的入口）。 */
    fun aligned(
        context: Context,
        watch: WatchStore,
        limit: Int = DEFAULT_LIMIT,
    ): List<Pair<MediaItem, WatchStore.Progress>> =
        aligned(LocalVideoSource.query(context), watch, limit)

    /** 用调用方已有的片源列表对齐，避免重复查 MediaStore。 */
    fun aligned(
        videos: List<MediaItem>,
        watch: WatchStore,
        limit: Int = DEFAULT_LIMIT,
    ): List<Pair<MediaItem, WatchStore.Progress>> {
        if (videos.isEmpty()) return emptyList()
        val byPath = videos.associateBy { normPath(it.path) }
        return watch.recentKeys(limit).mapNotNull { key ->
            val progress = watch.progressOf(key) ?: return@mapNotNull null
            val item = byPath[normPath(key)] ?: return@mapNotNull null
            item to progress
        }
    }
}