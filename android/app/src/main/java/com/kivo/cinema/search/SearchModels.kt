package com.kivo.cinema.search

import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.library.MediaItem
import com.kivo.cinema.tmdb.TmdbHome

/**
 * 搜索页统一条目（family-tree §四）：本地（NAS / MediaStore）与 TMDB 结果同构。
 *
 * 三种打开载体最多一种非空：[homeEntry]（NAS 剧集/单片）、[mediaItem]（本机文件）、
 * [tmdb]（TMDB 详情）。可播判定只来自真实本地匹配，[sourceLabel] 与 [quality] 不编造。
 */
data class SearchEntry(
    val key: String,
    val title: String,
    val year: String,
    val rating: Float,
    val genres: List<String>,
    val posterUrl: String?,
    val backdropUrl: String?,
    val isTv: Boolean,
    val playable: Boolean,
    /** 来源签条原文：「NAS 可播」「本地可播」「TMDB · 未入库」。 */
    val sourceLabel: String,
    /** 「4K」/「HDR」/null：仅从本地文件名或真实分辨率推算，未入库一律 null。 */
    val quality: String?,
    /** 热搜 NEW 标记：上映/首播 ≤30 天（真实日期推算）。 */
    val isNew: Boolean = false,
    val tmdb: TmdbHome.Item? = null,
    val homeEntry: HomeEntry? = null,
    val mediaItem: MediaItem? = null,
    val emby: EmbyPlayableIndex.Entry? = null,
) {
    /** 结果行元信息：「电影 · 2024 · 动作 · ★ 7.1」口径由 UI 拼装，这里只给片段。 */
    val kindLabel: String get() = if (isTv) "剧集" else "电影"
}

/** 六分类 + 「全部」（③④ tabs 与 ① 分类卡共用同一份数据，顺序即展示顺序）。 */
enum class SearchCategory(
    val label: String,
    val english: String,
    /** discover 的 kind：movie / tv；All 为空（由仓储双路合并）。 */
    val discoverKind: String,
    /** 分类固有 genre 约束（与类型行的选择取 AND）。 */
    val baseGenres: String,
    val originalLanguage: String? = null,
) {
    All("全部", "", "", ""),
    Series("剧集", "SERIES", "tv", ""),
    Movie("电影", "MOVIES", "movie", ""),
    Anime("动漫", "ANIME", "tv", "16", "ja"),
    Variety("综艺", "VARIETY", "tv", "10764"),
    Concert("演唱会", "CONCERT", "movie", "10402"),
    Documentary("纪录片", "DOCUMENTARY", "movie", "99");

    companion object {
        /** ① 分类卡的六个（不含 All）。 */
        val cards: List<SearchCategory> = entries.filter { it != All }
    }
}

/** ③ 类型行（label → TMDB genre id；id=0 表示「全部」）。 */
val SEARCH_GENRES = listOf(
    0 to "全部", 28 to "动作", 80 to "犯罪", 18 to "剧情",
    9648 to "悬疑", 878 to "科幻", 35 to "喜剧", 27 to "恐怖",
)

/** ③ 地区行（label → with_origin_country；空串 = 全部）。 */
val SEARCH_REGIONS = listOf(
    "" to "全部", "CN" to "大陆", "HK" to "香港", "US" to "美国",
    "KR" to "韩国", "JP" to "日本", "GB" to "英国",
)

/** ③ 年份行：具体年 / 2010s / 更早（2010s 与更早展开为日期区间，见 BrowseRepository）。 */
val SEARCH_YEARS = listOf("全部", "2026", "2025", "2024", "2023", "2010s", "更早")

/** ③ 排序：TMDB discover 没有「最近更新」口径，给真实存在的三档（family-tree D 偏差）。 */
val SEARCH_SORTS = listOf(
    "popularity.desc" to "人气最高",
    "primary_release_date.desc" to "最新上映",
    "vote_average.desc" to "评分最高",
)

/** 当前筛选条件（③④ 与吸顶摘要共用）。 */
data class BrowseFilter(
    val genreId: Int = 0,
    val region: String = "",
    val year: String = "全部",
    val sortIndex: Int = 0,
    val onlyPlayable: Boolean = false,
) {
    val genreLabel: String get() = SEARCH_GENRES.firstOrNull { it.first == genreId }?.second ?: "全部"
    val regionLabel: String get() = SEARCH_REGIONS.firstOrNull { it.first == region }?.second ?: "全部"
    val sortLabel: String get() = SEARCH_SORTS[sortIndex].second
}

/** 年份相容：双方都有年份且不同即否决；任一方缺失不否决（缺失方由标题包含匹配兜底）。 */
fun yearCompatible(entryYear: Int?, tmdbYear: String): Boolean {
    val y = tmdbYear.toIntOrNull() ?: return true
    return entryYear == null || entryYear == 0 || entryYear == y
}

/** 从文件名/真实分辨率推算画质角标；推算不出就 null（不显示），不编造。 */
fun qualityBadgeOf(fileName: String, width: Int): String? {
    val name = fileName.lowercase()
    return when {
        "hdr" in name -> "HDR"
        width >= 3840 || "2160p" in name || "4k" in name -> "4K"
        else -> null
    }
}