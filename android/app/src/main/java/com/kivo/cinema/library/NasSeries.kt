package com.kivo.cinema.library

/**
 * NAS 片库里的**一部剧**（同剧名跨季目录合并而成）。
 *
 * ## 为什么必须跨目录合并
 * 索引按文件落盘，NAS 上"一剧多季"就是"多个季目录"：
 * `剧集/American.Horror.Story.S01.../`、`...S02.../`、`...S03.../`、`...S04.../`。
 * 只按目录聚合的话，同一部剧会在首页出现 4 张同名卡（各标 12/13/13/13 集），
 * 用户看到的就是"重复的剧"。所以聚合键是 **(源, 共享, 父目录, 剧名)**，季归到 [seasons] 里。
 *
 * 只装数据与派生值，不做 IO / 不碰 UI；聚合规则在 [NasLibraryGrouping]。
 */
data class NasSeries(
    val sourceId: String,
    val share: String,
    /** 剧所在父目录（共享内路径，如 `剧集`）；跨季合并时它是唯一相同的层级。 */
    val rootDir: String,
    val title: String,
    val year: Int?,
    /** 按季号升序；每季内部按集号升序。 */
    val seasons: List<NasSeason>,
    /** 同目录封面（共享内路径）；null = 库里没图，上层兜底 TMDB。 */
    val posterPath: String?,
    val category: NasCategory,
    /** 该剧最后一次有新文件的时间（用于"最近添加"排序）。 */
    val latestAddedMs: Long,
) {
    val count: Int get() = seasons.sumOf { it.episodes.size }

    val seasonCount: Int get() = seasons.size

    /** 代表集：默认播第 1 季第 1 集（点卡片进详情，详情里再选集）。 */
    val firstEpisode: NasEpisode
        get() = seasons.firstOrNull()?.episodes?.firstOrNull()
            ?: NasEpisode("", "", 0, 1, 0L, 0L)

    /** 卡片副标题：单季 `共 6 集`；多季 `4 季 · 共 51 集`。 */
    val countLabel: String
        get() = if (seasons.size <= 1) "共 $count 集" else "$seasonCount 季 · 共 $count 集"

    /**
     * 剧的稳定键：`smb-series://<源>/<共享>/<父目录>|<剧名>`。
     *
     * **前缀必须与单集的 `smb://` 区分**——点击时靠它分流到"详情页（多季选集）"还是"详情页（单片）"。
     * 剧名放在 `|` 之后：标题可能含空格/中文，但不含 `|`，解析无歧义。
     */
    val key: String get() = "$KEY_PREFIX$sourceId/$share/$rootDir|$title"

    companion object {
        const val KEY_PREFIX = "smb-series://"

        fun isSeriesKey(key: String): Boolean = key.startsWith(KEY_PREFIX)

        /** 剧键的坐标；格式不对返回 null。 */
        data class Coords(val sourceId: String, val share: String, val rootDir: String, val title: String)

        fun parseKey(key: String): Coords? {
            if (!isSeriesKey(key)) return null
            val body = key.removePrefix(KEY_PREFIX)
            val bar = body.indexOf('|')
            if (bar <= 0) return null
            val title = body.substring(bar + 1)
            val parts = body.substring(0, bar).split('/', limit = 3)
            if (parts.size < 2) return null
            val sourceId = parts[0]
            val share = parts[1]
            if (sourceId.isBlank() || share.isBlank() || title.isBlank()) return null
            return Coords(sourceId, share, parts.getOrElse(2) { "" }, title)
        }
    }
}

/** 一季（季号 + 该季的集）。 */
data class NasSeason(val index: Int, val episodes: List<NasEpisode>)

/** 剧里的一集。只保留详情页选集/播放需要的字段。 */
data class NasEpisode(
    /** 共享内文件路径（[NasVideo.path]）。 */
    val path: String,
    /** 展示用集号：`第01集` / `E01` / 日期；认不出时回落文件名。 */
    val label: String,
    /** 集号（季内）；认不出为 0。 */
    val index: Int,
    val seasonIndex: Int,
    val size: Long,
    val addedMs: Long,
) {
    val fileName: String get() = path.substringAfterLast('/')
}
