package com.kivo.cinema.tmdb

import java.net.URLEncoder

/**
 * 搜索页 TMDB 端点（family-tree §四）：search/multi 与 trending/day。
 * 单一职责：拼路径；请求、12h 缓存、错误分类、条目解析全部复用 [TmdbHome.pagedItems]。
 */
class TmdbSearch(private val home: TmdbHome) {

    fun multi(query: String, page: Int = 1): TmdbHome.PagedHomeData {
        val q = URLEncoder.encode(query, "UTF-8")
        return home.pagedItems(
            "/search/multi?language=${TmdbLocale.tag()}&include_adult=false&query=$q&page=$page")
    }

    /** 热搜榜数据源：今日趋势（名次升降无数据源，不推算）。 */
    fun trendingDay(): TmdbHome.PagedHomeData =
        home.pagedItems("/trending/all/day?language=${TmdbLocale.tag()}")
}