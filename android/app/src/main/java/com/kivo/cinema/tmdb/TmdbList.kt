package com.kivo.cinema.tmdb

import java.util.Calendar

/**
 * 首页 TMDB 榜单的**唯一路径表**：首页行与「查看全部」分页页共用同一条路径，保证两边内容一致、共享缓存。
 *
 * 单一职责：只描述「某个榜单去哪个接口取」，不做网络、不做解析（由 [TmdbHome.list] 负责）。
 */
enum class TmdbList(val isTv: Boolean, private val build: (lang: String) -> String) {
    NOW_PLAYING(false, { "/movie/now_playing?language=$it" }),
    ON_THE_AIR(true, { "/tv/on_the_air?language=$it" }),
    TOP_RATED_TV(true, { "/tv/top_rated?language=$it" }),
    /** 动画 16 + 日语原声，按人气。 */
    TODAY_ANIME(true, { "/discover/tv?language=$it&with_genres=16&with_original_language=ja&sort_by=popularity.desc" }),
    TOP_RATED_MOVIE(false, { "/movie/top_rated?language=$it" }),
    /** 华语原声电影，按人气；≥50 票过滤掉无人评分的冷门条目。 */
    CHINESE_MOVIE(false, {
        "/discover/movie?language=$it&with_original_language=zh&sort_by=popularity.desc&vote_count.gte=50"
    }),
    /** 近一年首播的韩语剧集，按人气。 */
    KOREAN_TV(true, {
        "/discover/tv?language=$it&with_original_language=ko&first_air_date.gte=${yearAgo()}&sort_by=popularity.desc"
    }),
    /** 近一年首播的日语剧集，按人气；排除动画（动画单独在「今日动漫」）。 */
    JAPANESE_TV(true, {
        "/discover/tv?language=$it&with_original_language=ja&without_genres=16" +
            "&first_air_date.gte=${yearAgo()}&sort_by=popularity.desc"
    });

    /** 第 [page] 页的完整路径；第 1 页不带 page 参数，与旧版首页请求同一个缓存键。 */
    fun path(page: Int = 1): String = build(TmdbLocale.tag()) + if (page > 1) "&page=$page" else ""
}

private fun yearAgo(): String {
    val cal = Calendar.getInstance()
    cal.add(Calendar.YEAR, -1)
    return "%04d-%02d-%02d".format(cal.get(Calendar.YEAR), cal.get(Calendar.MONTH) + 1, cal.get(Calendar.DAY_OF_MONTH))
}
