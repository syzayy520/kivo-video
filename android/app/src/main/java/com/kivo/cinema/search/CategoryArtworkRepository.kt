package com.kivo.cinema.search

import com.kivo.cinema.tmdb.TmdbHome

/**
 * ① 六分类卡剧照（family-tree §一）：每分类取 discover 人气首条，
 * 再请求 `/{kind}/{id}/images?include_image_language=null` 选**无文字剧照**（w780）；
 * 无文字剧照缺失时回退该条目的 backdrop（真实图，不用色块伪造）。
 * 全部走 TmdbHome 的 12h 缓存；同步阻塞，调用方在 IO 线程执行。
 */
class CategoryArtworkRepository(private val home: TmdbHome) {

    fun load(): Map<SearchCategory, String> = SearchCategory.cards.mapNotNull { category ->
        artworkFor(category)?.let { category to it }
    }.toMap()

    private fun artworkFor(category: SearchCategory): String? {
        val first = home.pagedItems(BrowsePaths.discover(category, BrowseFilter(), 1))
            .items.firstOrNull() ?: return null
        val kind = if (first.isTv) "tv" else "movie"
        val images = home.get("/$kind/${first.id}/images?include_image_language=null").first
        val textless = images?.optJSONArray("backdrops")?.let { arr ->
            (0 until arr.length()).firstNotNullOfOrNull { i ->
                TmdbHome.imgPath(arr.optJSONObject(i), "file_path")
            }
        }
        return textless?.let { "${TmdbHome.IMG}/w780$it" } ?: first.backdropUrl
    }
}