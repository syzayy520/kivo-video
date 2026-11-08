package com.kivo.cinema.tmdb

import org.json.JSONArray
import org.json.JSONObject
import android.os.SystemClock
import com.kivo.cinema.tmdb.trailer.TmdbTrailers

/**
 * TMDB 首页数据域（239 §2；节序 = IMG_0136 全链实测）：
 * hero(趋势 10 颗) → 继续观看(本地) → 正在热映 → 正在播出 → 本周趋势(宽卡)
 * → 近一月·热门综艺 → 今日动漫。
 * 全部来自 TMDB 公开 API（now_playing / on_the_air / trending/all/week /
 * discover/tv 综艺近一月 + 今日动漫）。
 */
class TmdbHome(private val client: TmdbClient) {

    data class Item(
        val id: Int,
        val title: String,
        val date: String,        // release/first_air date yyyy-MM-dd
        val overview: String,
        val genres: List<String>,   // genre_ids → 中文名（GENRE_ZH 映射）
        val rating: Float,          // vote_average
        val posterUrl: String,      // w500
        val posterHeroUrl: String,  // w780（hero 全宽，1080px 屏不过度上采样）
        val backdropUrl: String,    // w780
        val isTv: Boolean,
    ) {
        /** 海报节年份标签（E9：「2026」）。 */
        fun year(): String = if (date.length >= 4) date.take(4) else date

        /** 宽卡标签（E8/hero_32：「2026 · 恐怖 / 科幻」）。 */
        fun tagLine(): String {
            val y = year()
            return if (genres.isEmpty()) y else "$y · " + genres.take(2).joinToString(" / ")
        }

        /** hero meta 行（帧实证：「奇幻 动作 · 2026-11-05」）。 */
        val metaLine: String =
            (if (genres.isEmpty()) "" else genres.take(2).joinToString(" ") + " · ") + date
    }

    companion object {
        const val IMG = "https://image.tmdb.org/t/p"
        fun poster(path: String) = "$IMG/w500$path"
        fun backdrop(path: String) = "$IMG/w780$path"

        /** TMDB genre_id → 中文名（/genre/movie/list&/tv/list zh-CN 全量，离线内嵌）。 */
        val GENRE_ZH = mapOf(
            28 to "动作", 12 to "冒险", 16 to "动画", 35 to "喜剧", 80 to "犯罪",
            99 to "纪录", 18 to "剧情", 10751 to "家庭", 14 to "奇幻", 36 to "历史",
            27 to "恐怖", 10402 to "音乐", 9648 to "悬疑", 10749 to "爱情",
            878 to "科幻", 10770 to "电视电影", 53 to "惊悚", 10752 to "战争",
            37 to "西部", 10759 to "动作冒险", 10762 to "儿童", 10763 to "新闻",
            10764 to "真人秀", 10765 to "科幻奇幻", 10766 to "肥皂剧", 10767 to "脱口秀",
            10768 to "战争政治",
        )

    }

    // Movie and TV IDs occupy different namespaces. Only successful metadata is cached.
    private data class ImageEntry(val at: Long, val json: JSONObject)
    private val imageMemo = HashMap<Pair<Boolean, Int>, ImageEntry>()
    private val trailers = TmdbTrailers(::get)

    fun trailerFor(item: Item) = trailers.forItem(item.isTv, item.id)

    @Synchronized
    private fun images(item: Item): JSONObject? {
        val key = item.isTv to item.id
        val now = SystemClock.elapsedRealtime()
        imageMemo[key]?.takeIf { now - it.at < 15 * 60_000L }?.let { return it.json }
        val kind = if (item.isTv) "tv" else "movie"
        val result = get("/$kind/${item.id}/images").first ?: return null
        imageMemo[key] = ImageEntry(now, result)
        return result
    }

    /** Prefer a neutral portrait so the title can live in its own foreground layer. */
    fun topVotedPosterUrl(item: Item): String? {
        val path = com.kivo.cinema.tmdb.artwork.ArtworkSelection.portrait(images(item))
        return path?.let { "$IMG/w780$it" }
    }

    /** Prefer localized logos, then the variant with the strongest voting support. */
    fun logoUrlFor(item: Item): String? {
        val logos = images(item)?.optJSONArray("logos") ?: return null
        val options = (0 until logos.length()).map { logos.getJSONObject(it) }
            .filter { it.optString("file_path").endsWith(".png", ignoreCase = true) }
        fun languageRank(o: JSONObject) = when (o.optString("iso_639_1")) {
            "zh" -> 0; "", "null" -> 1; "en" -> 2; else -> 3
        }
        val chosen = options.sortedWith(compareBy<JSONObject> { languageRank(it) }
            .thenByDescending { it.optInt("vote_count") }
            .thenByDescending { it.optDouble("vote_average", 0.0) }).firstOrNull()
        return chosen?.optString("file_path")?.let { "$IMG/w500$it" }
    }

    private fun auth(url: String): String =
        if (url.contains("?")) "$url&api_key=${client.apiKey()}"
        else "$url?api_key=${client.apiKey()}"

    data class HomeData(val items: List<Item>, val errorKind: String?)

    /** 错误五类（home.error.network.* 原文键）——按异常类型映射。 */
    private fun kindOf(e: Exception): String = when (e) {
        is java.net.SocketTimeoutException -> "timeout"
        is java.net.UnknownHostException -> "offline"
        is javax.net.ssl.SSLException -> "secure"
        is java.net.ConnectException -> "server"
        else -> "lost"
    }

    private fun get(path: String): Pair<JSONObject?, String?> {
        if (client.apiKey().isEmpty()) return null to "offline"
        return try {
            val conn = client.openConnection(auth("https://api.themoviedb.org/3$path"))
            conn.connectTimeout = 8000
            conn.readTimeout = 15000
            try {
                val code = conn.responseCode
                val body = (if (code in 200..299) conn.inputStream else conn.errorStream)
                    ?.bufferedReader()?.use { it.readText() } ?: ""
                if (code !in 200..299) null to "server" else JSONObject(body) to null
            } finally { conn.disconnect() }
        } catch (e: Exception) {
            null to kindOf(e)
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
            val genres = mutableListOf<String>()
            val gids = o.optJSONArray("genre_ids")
            if (gids != null) {
                for (g in 0 until gids.length()) {
                    GENRE_ZH[gids.getInt(g)]?.let { if (it !in genres) genres.add(it) }
                }
            }
            out.add(Item(
                id = o.getInt("id"),
                title = title,
                date = date,
                overview = o.optString("overview", ""),
                genres = genres,
                rating = rating,
                posterUrl = poster(poster),
                posterHeroUrl = "$IMG/w780$poster",
                backdropUrl = if (backdrop.isNotEmpty()) backdrop(backdrop) else poster(poster),
                isTv = o.has("name"),
            ))
        }
        return out
    }

    private fun items(path: String): HomeData =
        get(path).let { (j, err) ->
            HomeData(j?.optJSONArray("results")?.let { parseItems(it) } ?: emptyList(), err)
        }

    fun nowPlaying(): HomeData = items("/movie/now_playing?language=zh-CN")
    fun onTheAir(): HomeData = items("/tv/on_the_air?language=zh-CN")
    fun trendingWeek(): HomeData = items("/trending/all/week?language=zh-CN")

    /** 近一月·热门综艺（homeBuiltIn.popular.variety.past.month；真人秀 10764）。 */
    fun varietyPastMonth(): HomeData {
        val cal = java.util.Calendar.getInstance()
        cal.add(java.util.Calendar.DAY_OF_MONTH, -30)
        val from = "%04d-%02d-%02d".format(
            cal.get(java.util.Calendar.YEAR),
            cal.get(java.util.Calendar.MONTH) + 1,
            cal.get(java.util.Calendar.DAY_OF_MONTH))
        return items("/discover/tv?language=zh-CN&with_genres=10764" +
            "&first_air_date.gte=$from&sort_by=popularity.desc")
    }

    /** 今日动漫（homeBuiltIn.today.s.anime；动画 16 + 日语原声，按人气）。 */
    fun todayAnime(): HomeData =
        items("/discover/tv?language=zh-CN&with_genres=16" +
            "&with_original_language=ja&sort_by=popularity.desc")

    /** hero 候选：本周趋势前 10 颗（t22/t18/0082 圆点实测均 10）。 */
    fun heroCandidates(limit: Int = 10): List<Item> =
        trendingWeek().items.take(limit)
}
