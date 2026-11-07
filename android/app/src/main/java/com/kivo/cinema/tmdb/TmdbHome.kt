package com.kivo.cinema.tmdb

import org.json.JSONArray
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL

/**
 * TMDB 首页数据域（237 方案，用户指定：首页除继续观看外全部 TMDB 数据）。
 * 节序（grid_01/grid_02/rex_browse_a 帧实测）：
 * hero(轮播) → 正在热映 → 正在播出 → 本周趋势 → 热门人物 → 播出平台 → 分类浏览。
 * 全部来自 TMDB 公开 API（/movie/now_playing、/tv/on_the_air、/trending/all/week、/person/popular）。
 */
class TmdbHome(private val client: TmdbClient) {

    data class Item(
        val title: String,
        val date: String,        // release/first_air date yyyy-MM-dd
        val overview: String,
        val genres: List<String>,
        val rating: Float,       // vote_average
        val posterUrl: String,   // w500
        val backdropUrl: String, // w780
        val isTv: Boolean,
    )

    companion object {
        const val IMG = "https://image.tmdb.org/t/p"
        fun poster(path: String) = "$IMG/w500$path"
        fun backdrop(path: String) = "$IMG/w780$path"
    }

    private fun auth(url: String): String =
        if (url.contains("?")) "$url&api_key=${client.apiKey()}"
        else "$url?api_key=${client.apiKey()}"

    private fun get(path: String): JSONObject? {
        if (client.apiKey().isEmpty()) return null
        return try {
            val conn = client.openConnection(auth("https://api.themoviedb.org/3$path"))
            conn.connectTimeout = 8000
            conn.readTimeout = 15000
            val code = conn.responseCode
            val body = (if (code in 200..299) conn.inputStream else conn.errorStream)
                ?.bufferedReader()?.readText() ?: ""
            if (code !in 200..299) null else JSONObject(body)
        } catch (e: Exception) {
            null
        }
    }

    private fun parseItems(results: JSONArray): List<Item> {
        val out = mutableListOf<Item>()
        for (i in 0 until results.length()) {
            val o = results.getJSONObject(i)
            val title = o.optString("title", o.optString("name", ""))
            val date = o.optString("release_date", o.optString("first_air_date", ""))
            val poster = o.optString("poster_path", "")
            val backdrop = o.optString("backdrop_path", "")
            if (title.isEmpty() || poster.isEmpty()) continue
            val rating = o.getDouble("vote_average").toFloat()
            if (rating <= 0f) continue
            out.add(Item(
                title = title,
                date = date,
                overview = o.optString("overview", ""),
                genres = emptyList(),   // genre_ids → 名称映射 v2（语言包）
                rating = rating,
                posterUrl = poster(poster),
                backdropUrl = if (backdrop.isNotEmpty()) backdrop(backdrop) else poster(poster),
                isTv = o.has("name"),
            ))
        }
        return out
    }

    /** 首页三节（网络调用，调用方备线程）。失败返回空表——UI 对应节隐藏。 */
    fun nowPlaying(): List<Item> =
        get("/movie/now_playing?language=zh-CN")?.optJSONArray("results")
            ?.let { parseItems(it) } ?: emptyList()

    fun onTheAir(): List<Item> =
        get("/tv/on_the_air?language=zh-CN")?.optJSONArray("results")
            ?.let { parseItems(it) } ?: emptyList()

    fun trendingWeek(): List<Item> =
        get("/trending/all/week?language=zh-CN")?.optJSONArray("results")
            ?.let { parseItems(it) } ?: emptyList()

    /** hero 候选：本周趋势前 11 个（grid_01 实测 ~11 圆点），须有 backdrop。 */
    fun heroCandidates(limit: Int = 11): List<Item> =
        trendingWeek().filter { it.backdropUrl.isNotBlank() }.take(limit)
}
