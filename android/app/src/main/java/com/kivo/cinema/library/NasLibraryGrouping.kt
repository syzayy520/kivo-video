package com.kivo.cinema.library

/**
 * 片库索引 → "一部剧 / 一个单片"。**只做分组，不取数、不碰 UI。**
 *
 * ## 两级分组（缺一不可）
 * 1. **按目录**：NAS 上"一剧一目录（或一季一目录）、一集一文件"，目录是最可靠的自然边界；
 * 2. **按剧名跨目录合并**：多季剧在 NAS 上是 `剧集/某剧.S01.../`、`剧集/某剧.S02.../` 多个目录，
 *    只按目录聚合会让同一部剧在首页出现 4 张同名卡（真机：American Horror Story S01–S04）。
 *    所以第二级用 `(源, 共享, 父目录, 剧名)` 把各季并成一部剧，季号归到 [NasSeries.seasons]。
 *
 * ## 什么时候算"剧"
 * 目录里**能解析出集号的文件 ≥ 2** 才算。这条下限是必要的：
 * - 电影目录常带 1 个正片 + 若干花絮（花絮没有集号）⇒ 保持单片，不会被并成"2 集的剧"；
 * - 单集文件（剧集只下了一集）⇒ 保持单片，用户点开就是那一集。
 *
 * 集号认：中文 `第01集/第3话/第12期` → latin `S01E01/E05/EP01` → 日期 `20240101`（综艺）。
 */
object NasLibraryGrouping {

    sealed interface Item {
        val category: NasCategory

        data class Series(val series: NasSeries) : Item {
            override val category: NasCategory get() = series.category
        }

        data class Single(val video: NasVideo) : Item {
            override val category: NasCategory get() = video.category
        }
    }

    /** 聚合入口：输出顺序与"原列表首次出现"一致，避免首页行每次刷新都跳。 */
    fun group(videos: List<NasVideo>): List<Item> {
        val order = videos.withIndex().associate { it.value.path to it.index }
        val pending = ArrayList<Pair<Int, Item>>()
        val dirSeasons = ArrayList<DirSeason>()

        videos.groupBy { Triple(it.sourceId, it.share, it.dirPath) }.forEach { (_, files) ->
            val season = seasonOf(files)
            if (season != null) {
                dirSeasons += season
            } else {
                files.forEach { v -> pending += (order[v.path] ?: Int.MAX_VALUE) to Item.Single(v) }
            }
        }
        dirSeasons.groupBy { Quad(it.sourceId, it.share, it.rootDir, it.title) }.forEach { (_, list) ->
            val first = list.minOf { s -> s.episodes.minOf { order[it.path] ?: Int.MAX_VALUE } }
            pending += first to Item.Series(merge(list))
        }
        return pending.sortedBy { it.first }.map { it.second }
    }

    // ---- 第一级：目录 → 一季 ----

    private class DirSeason(
        val sourceId: String,
        val share: String,
        val rootDir: String,
        val title: String,
        val year: Int?,
        val seasonIndex: Int,
        val episodes: List<NasEpisode>,
        val posterPath: String?,
        val category: NasCategory,
        val latestAddedMs: Long,
    )

    private fun seasonOf(files: List<NasVideo>): DirSeason? {
        val tagged = files.mapNotNull { v -> episodeOf(v.fileName)?.let { v to it } }
        if (tagged.size < 2) return null

        val head = files.first()
        val dirParsed = MediaTitleParser.parse(head.dirName)
        // 标题优先用目录名（跨季合并靠它对齐）；目录名洗不出东西时退回文件名。
        val title = dirParsed.title.ifBlank { MediaTitleParser.parse(head.fileName).title }
        val seasonIndex = seasonIndexOf(dirParsed.episode, tagged.first().second)

        val episodes = tagged
            .map { (video, ep) ->
                NasEpisode(
                    path = video.path,
                    label = ep.label,
                    index = ep.index,
                    seasonIndex = seasonIndex,
                    size = video.size,
                    addedMs = video.addedMs,
                )
            }
            // 认不出集号（index=0）的排最后：宁可相信"没有编号"，也不要排到第 1 集前面。
            .sortedWith(compareBy({ if (it.index > 0) 0 else 1 }, { it.index }, { it.path }))

        return DirSeason(
            sourceId = head.sourceId,
            share = head.share,
            rootDir = head.dirPath.substringBeforeLast('/', ""),
            title = title,
            year = dirParsed.year ?: MediaTitleParser.parse(head.fileName).year,
            seasonIndex = seasonIndex,
            episodes = episodes,
            posterPath = files.firstNotNullOfOrNull { it.posterPath },
            category = head.category,
            latestAddedMs = files.maxOf { it.addedMs },
        )
    }

    /** 季号：目录名里的 `S01/Season 2/第3季` 优先，其次文件集号带的季，最后按第 1 季。 */
    private fun seasonIndexOf(dirEpisode: String?, sample: Ep): Int {
        dirEpisode?.let { d ->
            SEASON_NUM.find(d)?.groupValues?.drop(1)
                ?.firstNotNullOfOrNull { it.toIntOrNull()?.takeIf { n -> n > 0 } }
                ?.let { return it }
        }
        return sample.season.takeIf { it > 0 } ?: 1
    }

    // ---- 第二级：多季 → 一部剧 ----

    private fun merge(list: List<DirSeason>): NasSeries {
        val head = list.first()
        // 同季目录重复出现（罕见，例如手工复制过一季）时按 path 去重，不让集列表出现重复集。
        val seasons = list
            .groupBy { it.seasonIndex }
            .map { (index, same) ->
                NasSeason(index, same.flatMap { it.episodes }.distinctBy { it.path })
            }
            .sortedBy { it.index }
        return NasSeries(
            sourceId = head.sourceId,
            share = head.share,
            rootDir = head.rootDir,
            title = head.title,
            year = head.year,
            seasons = seasons,
            posterPath = list.firstNotNullOfOrNull { it.posterPath },
            category = head.category,
            latestAddedMs = list.maxOf { it.latestAddedMs },
        )
    }

    private data class Quad(val a: String, val b: String, val c: String, val d: String)

    // ---- 集号识别 ----

    private class Ep(val label: String, val index: Int, val season: Int)

    /**
     * 从文件名认集号。依次认：中文（`第01集/第3期`）→ latin 季集（`S01E01`/`E05`）→
     * `EP01` → **日期**（综艺常用 `节目名.20240101`）。
     *
     * 认不出返回 null —— 「认不出」必须显式返回 null，不能伪造 index=1，
     * 否则电影目录里的"正片+花絮"会被并成一部两集的剧。
     */
    private fun episodeOf(fileName: String): Ep? {
        MediaTitleParser.cnEpisodeIndex(fileName)?.let { n ->
            return Ep("第%02d集".format(n), n, 0)
        }
        MediaTitleParser.parse(fileName).episode?.let { raw ->
            EPISODE_TAIL.find(raw)?.let { m ->
                val idx = m.groupValues[2].toIntOrNull() ?: return@let
                val season = m.groupValues[1].toIntOrNull() ?: 0
                return Ep(raw, idx, season)
            }
        }
        EP_FORM.find(fileName)?.groupValues?.getOrNull(1)?.toIntOrNull()?.let { n ->
            return Ep("EP%02d".format(n), n, 0)
        }
        // 综艺日期期号：`20240101` / `2024-01-01` / `2024.01.01`。
        // 必须校验月/日在合法区间——否则 `2024.1080`（年份+分辨率）会被误认成日期。
        DATE_FORM.find(fileName)?.let { m ->
            val y = m.groupValues[1].toIntOrNull()
            val mo = m.groupValues[2].toIntOrNull()
            val d = m.groupValues[3].toIntOrNull()
            if (y != null && mo != null && d != null && mo in 1..12 && d in 1..31) {
                val iso = "${m.groupValues[1]}-${m.groupValues[2]}-${m.groupValues[3]}"
                return Ep(iso, y * 10000 + mo * 100 + d, 0)
            }
        }
        return null
    }

    /** 从标准化集号里取季号与集号：`S01E01`→(1,1)、`E05`→(0,5)、`S02`→null（整季不算单集）。 */
    private val EPISODE_TAIL = Regex("""^(?:[Ss](\d{1,2}))?[Ee](\d{1,4})$""")

    /** `EP01` / `ep12`（动漫常见）。前后必须是分隔符，避免命中普通单词里的 ep。 */
    private val EP_FORM = Regex("""(?i)(?:^|[\s._\-])ep(\d{1,4})(?!\d)""")

    /** 8 位日期（`20240101` / `2024-01-01` / `2024.01.01`）；分组 1/2/3 = 年/月/日。 */
    private val DATE_FORM = Regex("""(20\d{2})[\-._]?(\d{2})[\-._]?(\d{2})(?!\d)""")

    /** 目录名里的季号：`S01` / `Season 2` / `第3季`。 */
    private val SEASON_NUM = Regex("""(?i)(?:^|[\s._\-])(?:s(\d{1,2})|season[ ._-]?(\d{1,2})|第\s*(\d{1,2})\s*季)""")
}
