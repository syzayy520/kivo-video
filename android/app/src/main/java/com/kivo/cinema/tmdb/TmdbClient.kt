package com.kivo.cinema.tmdb

import android.content.Context
import org.json.JSONObject
import java.io.File
import java.net.HttpURLConnection
import java.net.URLEncoder
import java.net.URL

/**
 * TMDB 评分源（237 方案 F2，零第三方）：搜索片名+年份 → vote_average（TMDB 评分）。
 *
 * Key 管理：filesDir/tmdb_key.txt（用户在 TMDB 注册免费获取后放入；空=功能整体隐藏，G9 一致）。
 * 评分缓存：filesDir/tmdb_cache.json（titleKey→rating，命中即不走网）。
 * 失败语义：无 key/无网/无匹配 → 返回 null，徽章行隐藏——不显示 0 分（设计稿 §4）。
 */
class TmdbClient(context: Context) {

    companion object {
        // TMDB's API hostname; keep it shared by home, details and rating requests.
        const val API_BASE = "https://api.tmdb.org/3"
        @Volatile private var shared: TmdbClient? = null
        fun setShared(c: TmdbClient) { shared = c }
        fun staticOpen(urlStr: String): HttpURLConnection =
            shared?.openConnection(urlStr)
                ?: URL(urlStr).openConnection() as HttpURLConnection

        fun staticBitmap(urlStr: String): android.graphics.Bitmap? = try {
            val conn = staticOpen(urlStr)
            if (conn.responseCode in 200..299) {
                android.graphics.BitmapFactory.decodeStream(conn.inputStream)
            } else null
        } catch (e: Exception) { null }
    }

    data class Rating(val source: String, val score: Float)   // source="TMDB"

    private val keyFile = File(context.filesDir, "tmdb_key.txt")
    private val cacheFile = File(context.filesDir, "tmdb_cache.json")
    private val cache = HashMap<String, Rating>()
    private var cacheLoaded = false

    @Synchronized
    fun apiKey(): String = runCatching {
        if (keyFile.exists()) keyFile.readText().trim() else ""
    }.getOrDefault("")

    /** 代理（可选）：filesDir/tmdb_proxy.txt 写 "host:port"（SOCKS）。
     *  仅 TMDB 域走代理——SMB/Emby 等内网源不受影响。开发期可用 adb reverse。 */
    private val proxyFile = File(context.filesDir, "tmdb_proxy.txt")

    @Synchronized
    fun proxyAddr(): String? = runCatching {
        if (proxyFile.exists()) proxyFile.readText().trim().takeIf { it.isNotBlank() } else null
    }.getOrNull()

    /** 统一出网口：有代理则 SOCKS，否则直连。 */
    fun openConnection(urlStr: String): HttpURLConnection {
        val url = URL(urlStr)
        val proxied = proxyAddr()?.let { addr ->
            val host = addr.substringBeforeLast(':')
            val port = addr.substringAfterLast(':').toIntOrNull() ?: 10808
            java.net.Proxy(java.net.Proxy.Type.SOCKS,
                java.net.InetSocketAddress(host, port))
        }
        return (proxied?.let { url.openConnection(it) } ?: url.openConnection())
            as HttpURLConnection
    }

    @Synchronized
    private fun loadCache() {
        if (cacheLoaded) return
        cacheLoaded = true
        runCatching {
            if (cacheFile.exists()) {
                val o = JSONObject(cacheFile.readText())
                o.keys().forEach { k ->
                    cache[k] = Rating("TMDB", o.getDouble(k).toFloat())
                }
            }
        }
    }

    @Synchronized
    private fun saveCache() {
        val o = JSONObject()
        cache.forEach { (k, v) -> o.put(k, v.score.toDouble()) }
        cacheFile.writeText(o.toString())
    }

    private fun cacheKey(title: String, year: Int?) = "$title#$year"

    /** search/movie 或 search/tv 首条命中：评分与详情智能填充共用同一次请求逻辑。无 key/无网/无匹配 → null。 */
    private fun searchFirst(title: String, year: Int?, series: Boolean = false): JSONObject? {
        if (apiKey().isEmpty()) return null
        return try {
            val q = URLEncoder.encode(title, "UTF-8")
            val yq = year?.let {
                if (series) "&first_air_date_year=$it" else "&year=$it"
            } ?: ""
            val url = "$API_BASE/search/${if (series) "tv" else "movie"}?api_key=${apiKey()}" +
                "&language=zh-CN&query=$q$yq"
            val conn = openConnection(url)
            conn.connectTimeout = 8000
            conn.readTimeout = 15000
            val code = conn.responseCode
            val body = (if (code in 200..299) conn.inputStream else conn.errorStream)
                ?.bufferedReader()?.readText() ?: ""
            if (code !in 200..299) return null
            val results = JSONObject(body).getJSONArray("results")
            if (results.length() == 0) null else results.getJSONObject(0)
        } catch (e: Exception) {
            null
        }
    }

    /** 254 D2：片名+年份 → TMDB id（详情智能填充入口）。无匹配 → null（调用方走 Emby 文本兜底）。 */
    fun searchId(title: String, year: Int?, series: Boolean = false): Int? =
        searchFirst(title, year, series)?.optInt("id")?.takeIf { it > 0 }

    /** 255：集剧照兜底（库缩略图优先）。episode_number → still URL；无 key/无网/无图 → 空 map。 */
    fun seasonStills(tvId: Int, season: Int): Map<Int, String> {
        if (apiKey().isEmpty()) return emptyMap()
        return try {
            val conn = openConnection(
                "$API_BASE/tv/$tvId/season/$season?api_key=${apiKey()}&language=zh-CN")
            conn.connectTimeout = 8000
            conn.readTimeout = 15000
            if (conn.responseCode !in 200..299) return emptyMap()
            val arr = JSONObject(conn.inputStream.bufferedReader().use { it.readText() })
                .getJSONArray("episodes")
            (0 until arr.length()).mapNotNull {
                val o = arr.getJSONObject(it)
                val still = o.optString("still_path")
                if (still.isBlank()) null
                else o.optInt("episode_number") to "${TmdbHome.IMG}/w300$still"
            }.toMap()
        } catch (e: Exception) {
            emptyMap()
        }
    }

    /** 同步阻塞；调用方备线程。失败返回 null（徽章行隐藏）。 */
    @Synchronized
    fun ratingFor(title: String, year: Int?): Rating? {
        if (apiKey().isEmpty()) return null
        loadCache()
        cache[cacheKey(title, year)]?.let { return it }
        val first = searchFirst(title, year) ?: return null
        val score = first.optDouble("vote_average", 0.0).toFloat()
        if (score <= 0f) return null   // 无评分条目不显示
        val r = Rating("TMDB", score)
        cache[cacheKey(title, year)] = r
        saveCache()
        return r
    }
}
