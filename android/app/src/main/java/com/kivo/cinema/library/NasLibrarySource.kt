package com.kivo.cinema.library

/**
 * NAS 片库索引 → 首页分类行。
 *
 * 单一职责：把索引用**分类**切成一节一节的（电影/剧集/综艺/动漫/演唱会/纪录片），
 * 每节给出该分类的前 N 条 [HomeEntry]。不读磁盘、不扫描、不碰 UI。
 *
 * 空分类与"设置里关掉的分类"都不出现在结果里——首页不画空行。
 */
object NasLibrarySource {

    data class Section(val category: NasCategory, val entries: List<HomeEntry>)

    /** 单个分类的条目。剧集已按"一部剧一条"聚合，不再是"一集一张卡"。 */
    fun entries(videos: List<NasVideo>, category: NasCategory, limit: Int = 12): List<HomeEntry> =
        NasLibraryGrouping.group(videos)
            .asSequence()
            .filter { it.category == category }
            .take(limit)
            .map { it.toHomeEntry() }
            .toList()

    private fun NasLibraryGrouping.Item.toHomeEntry(): HomeEntry = when (this) {
        is NasLibraryGrouping.Item.Single -> {
            val v = video
            val parsed = MediaTitleParser.parse(v.fileName)
            HomeEntry(
                key = v.watchKey,
                title = parsed.title,
                sourceLabel = "NAS",
                addedSec = if (v.addedMs > 0) v.addedMs / 1000 else 0L,
                positionMs = 0L,
                durationMs = 0L,
                kind = HomeEntry.Kind.Smb,
                lastPlayedMs = v.addedMs,
                // 库内同目录封面：有就优先用，没有由 UI 兜底 TMDB。
                smbPosterPath = v.posterPath,
                year = parsed.year,
                searchAlias = parsed.english,
                episodeLabel = parsed.episode,
            )
        }
        is NasLibraryGrouping.Item.Series -> {
            val s = series
            HomeEntry(
                // 剧用 series 键（`smb-series://`）：点击分流到"详情页 + 选集"，不是直接播第 1 集。
                key = s.key,
                title = s.title,
                sourceLabel = "NAS",
                addedSec = if (s.latestAddedMs > 0) s.latestAddedMs / 1000 else 0L,
                positionMs = 0L,
                durationMs = 0L,
                kind = HomeEntry.Kind.Smb,
                lastPlayedMs = s.latestAddedMs,
                smbPosterPath = s.posterPath,
                year = s.year,
                searchAlias = null,
                episodeLabel = s.countLabel,
            )
        }
    }

    /**
     * 按 [NasCategory.ordered] 顺序切节；[visible] 决定哪些分类参与（设置开关）。
     * 空分类自动略过。
     */
    fun sections(
        videos: List<NasVideo>,
        limit: Int = 12,
        visible: (NasCategory) -> Boolean,
    ): List<Section> =
        NasCategory.ordered
            .filter(visible)
            .mapNotNull { c ->
                val items = entries(videos, c, limit)
                if (items.isEmpty()) null else Section(c, items)
            }
}
