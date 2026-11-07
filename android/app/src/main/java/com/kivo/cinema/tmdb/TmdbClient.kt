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

    data class Rating(val source: String, val score: Float)   // source="TMDB"

    private val keyFile = File(context.filesDir, "tmdb_key.txt")
    private val cacheFile = File(context.filesDir, "tmdb_cache.json")
    private val cache = HashMap<String, Rating>()
    private var cacheLoaded = false

    @Synchronized
    fun apiKey(): String = runCatching {
        if (keyFile.exists()) keyFile.readText().trim() else ""
    }.getOrDefault("")

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

    /** 同步阻塞；调用方备线程。失败返回 null（徽章行隐藏）。 */
    @Synchronized
    fun ratingFor(title: String, year: Int?): Rating? {
        if (apiKey().isEmpty()) return null
        loadCache()
        cache[cacheKey(title, year)]?.let { return it }
        return try {
            val q = URLEncoder.encode(title, "UTF-8")
            val yq = year?.let { "&year=$it" } ?: ""
            val url = "https://api.themoviedb.org/3/search/movie?api_key=${apiKey()}" +
                "&language=zh-CN&query=$q$yq"
            val conn = URL(url).openConnection() as HttpURLConnection
            conn.connectTimeout = 8000
            conn.readTimeout = 15000
            val code = conn.responseCode
            val body = (if (code in 200..299) conn.inputStream else conn.errorStream)
                ?.bufferedReader()?.readText() ?: ""
            if (code !in 200..299) return null
            val results = JSONObject(body).getJSONArray("results")
            if (results.length() == 0) return null
            val first = results.getJSONObject(0)
            val score = first.getDouble("vote_average").toFloat()
            if (score <= 0f) return null   // 无评分条目不显示
            val r = Rating("TMDB", score)
            cache[cacheKey(title, year)] = r
            saveCache()
            r
        } catch (e: Exception) {
            null
        }
    }
}
