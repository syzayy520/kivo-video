package com.kivo.cinema.search

import com.kivo.cinema.tmdb.TmdbSearch
import java.text.SimpleDateFormat
import java.util.Calendar
import java.util.Date
import java.util.Locale

/**
 * ① 热搜榜（family-tree §二）：TMDB trending/all/day 前 10 条。
 * NEW = 上映/首播日期距今 ≤30 天（真实日期推算）；名次升降无数据源，不实现（D3）。
 * 点击行为 = 把片名填入搜索框（不是直接进详情），与热搜的「搜这个词」语义一致。
 */
class HotSearchRepository(private val tmdb: TmdbSearch) {

    data class Board(val items: List<SearchEntry>, val error: String?)

    fun load(): Board {
        val data = tmdb.trendingDay()
        val items = data.items.take(10).map { item ->
            SearchEntry(
                key = "hot:${item.isTv}:${item.id}",
                title = item.title,
                year = item.year(),
                rating = item.rating,
                genres = item.genres,
                posterUrl = item.posterUrl,
                backdropUrl = item.backdropUrl,
                isTv = item.isTv,
                playable = false,
                sourceLabel = "TMDB · 未入库",
                quality = null,
                isNew = isRecent(item.date),
                tmdb = item,
            )
        }
        return Board(items, if (items.isEmpty()) data.errorKind?.let { SearchRepository.errorText(it) } else null)
    }

    private fun isRecent(date: String): Boolean {
        if (date.length < 10) return false
        val parsed = runCatching {
            SimpleDateFormat("yyyy-MM-dd", Locale.US).parse(date.take(10))
        }.getOrNull() ?: return false
        val cutoff = Calendar.getInstance().apply { add(Calendar.DAY_OF_MONTH, -30) }.time
        return !parsed.before(cutoff) && !parsed.after(Date())
    }
}