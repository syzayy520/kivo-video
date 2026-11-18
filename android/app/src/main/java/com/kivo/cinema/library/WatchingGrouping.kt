package com.kivo.cinema.library

/**
 * 「在看」列表的一行。
 *
 * ## 为什么要聚合
 * 观看记录是**按集**存的，用户连看 5 集同一部剧就会有 5 条记录，
 * 在看页会被同一部剧刷满。这里把"同一部剧的多个集"收成一行，代表 = 最近看的那一集。
 *
 * ## 判据（与片库保持一致）
 * 复用 [NasLibraryGrouping] 的口径：`(源, 共享, 父目录, 剧名)` 相同即同一部剧 ——
 * 这样多季分目录（`某剧.S01/`、`某剧.S02/`）也能合成一行。
 * 认不出集号的（电影、单集文件）各自独立成行，绝不被误并。
 *
 * 只做分组，不取数、不碰 UI。
 */
data class WatchingRow(
    /** 展示片名（剧名或电影名）。 */
    val title: String,
    /** 代表条目：最近看的那一集/那部片。点击播它。 */
    val latest: HomeEntry,
    /** 同剧的其它集（含代表，最近在前）；单片时只有 1 条。 */
    val episodes: List<HomeEntry>,
    /** 代表条目的集号（`S02E03` / `第03集`）；认不出为 null。 */
    val episodeLabel: String?,
    /** 来源短标签（NAS / 本地 / Emby）。 */
    val sourceLabel: String,
) {
    val progress: Float get() = latest.progress
    val positionMs: Long get() = latest.positionMs
    val durationMs: Long get() = latest.durationMs
    val entryCount: Int get() = episodes.size
    val isSeries: Boolean get() = episodes.size > 1
}

object WatchingGrouping {

    /** 输入按最近观看倒序的条目，输出同序的行；代表条目取该剧最近看的一集。 */
    fun rows(items: List<HomeEntry>): List<WatchingRow> {
        val groups = LinkedHashMap<String, MutableList<HomeEntry>>()
        val singles = ArrayList<HomeEntry>()
        for (e in items) {
            val g = groupOf(e)
            if (g == null) singles += e else groups.getOrPut(g.key) { mutableListOf() } += e
        }
        val out = ArrayList<WatchingRow>(groups.size + singles.size)
        groups.forEach { (_, list) ->
            // 输入已按 updatedAt 倒序 ⇒ 组内第一个就是最近看的。
            val latest = list.first()
            out += WatchingRow(
                title = titleOf(latest),
                latest = latest,
                episodes = list,
                episodeLabel = filePathOf(latest)?.let { episodeOf(it) },
                sourceLabel = latest.sourceLabel,
            )
        }
        singles.forEach { e ->
            out += WatchingRow(
                title = e.title,
                latest = e,
                episodes = listOf(e),
                episodeLabel = null,
                sourceLabel = e.sourceLabel,
            )
        }
        return out.sortedByDescending { it.latest.lastPlayedMs }
    }

    private class Group(val key: String)

    /**
     * 同一部剧的分组键；**认不出集号就返回 null**（保持单片）。
     *
     * Emby 条目不从 key 解析（键是 `emby:源:条目`，拿不到目录），也不参与聚合 ——
     * 宁可让 Emby 各自一行，也不要把不同剧错误并到一起。
     */
    private fun groupOf(entry: HomeEntry): Group? {
        val path = filePathOf(entry) ?: return null
        if (episodeOf(path) == null) return null
        val fileName = path.substringAfterLast('/')
        val title = MediaTitleParser.parse(fileName).title
        if (title.isBlank()) return null
        // 跨季合并：父目录 + 剧名（与片库聚合同口径）。源前缀区分不同共享/设备。
        val root = path.substringBeforeLast('/', "").substringBeforeLast('/', "")
        return Group("${sourcePrefixOf(entry)}|$root|$title")
    }

    /** 共享内/设备内文件路径；Emby 无（返回 null）。 */
    private fun filePathOf(entry: HomeEntry): String? = when (entry.kind) {
        HomeEntry.Kind.Smb -> SmbContinueKey.parse(entry.key)?.path
        HomeEntry.Kind.Local -> entry.mediaItem?.path ?: entry.key.takeIf { it.startsWith("/") }
        HomeEntry.Kind.Emby -> null
    }

    private fun sourcePrefixOf(entry: HomeEntry): String = when (entry.kind) {
        HomeEntry.Kind.Smb -> SmbContinueKey.parse(entry.key)
            ?.let { "${it.sourceId}|${it.share}" } ?: "smb"
        HomeEntry.Kind.Local -> "local"
        HomeEntry.Kind.Emby -> "emby"
    }

    private fun titleOf(entry: HomeEntry): String {
        val parsed = filePathOf(entry)?.substringAfterLast('/')
            ?.let { MediaTitleParser.parse(it).title }
        return parsed?.takeIf { it.isNotBlank() } ?: entry.title
    }

    /** 集号：中文 `第03集` 优先，其次 latin `S01E01`；认不出 null（与片库同一套判据）。 */
    private fun episodeOf(path: String): String? {
        val fileName = path.substringAfterLast('/')
        MediaTitleParser.cnEpisodeIndex(fileName)?.let { return "第%02d集".format(it) }
        return MediaTitleParser.parse(fileName).episode
    }
}
