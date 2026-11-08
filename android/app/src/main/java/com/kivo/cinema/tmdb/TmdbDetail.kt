package com.kivo.cinema.tmdb

import org.json.JSONObject

/**
 * 影片详情数据域（240 §1）：一次 append_to_response 拉详情+预告+演职员+相似+图片。
 * 全部 TMDB 公开 API，key/代理复用 TmdbClient 既有链路；网络失败返回 null（G9 由 UI 呈现）。
 */
class TmdbDetail(private val client: TmdbClient) {

    data class Details(
        val id: Int,
        val title: String,
        val date: String,          // yyyy-MM-dd，空=未定档
        val genres: List<String>,  // 中文名（详情接口自带 zh-CN 名称）
        val overview: String,
        val rating: Float,
        val posterUrl: String,     // w780（详情幕布用）
        val isTv: Boolean,
        val region: String?,       // 制作地区首国（「英国」）
        val episodes: Int?,        // 剧集集数（电影为 null）
        val trailerKey: String?,   // YouTube key（官方 Trailer，zh 优先）
        val cast: List<CastMember>,
        val similar: List<SimilarItem>,
    )

    data class CastMember(val name: String, val role: String, val profileUrl: String?)
    data class SimilarItem(val id: Int, val title: String, val date: String,
                           val rating: Float, val posterUrl: String)

    fun details(isTv: Boolean, id: Int): Details? {
        val kind = if (isTv) "tv" else "movie"
        val json = get("/$kind/$id?language=zh-CN&" +
            "append_to_response=videos,credits,similar,images" +
            "&include_image_language=zh,en,null").first ?: return null
        val title = json.optString("title", json.optString("name", ""))
        if (title.isEmpty()) return null
        val genres = json.optJSONArray("genres")?.let { arr ->
            (0 until arr.length()).map { arr.getJSONObject(it).optString("name") }
                .filter { it.isNotEmpty() }
        } ?: emptyList()
        val date = json.optString("release_date", json.optString("first_air_date", ""))
        val region = json.optJSONArray("production_countries")?.optJSONObject(0)
            ?.optString("name")?.takeIf { it.isNotEmpty() }
            ?: json.optJSONArray("origin_country")?.optString(0)?.takeIf { it.isNotEmpty() }
        val videos = json.optJSONObject("videos")?.optJSONArray("results")
        var trailerKey: String? = null
        if (videos != null) {
            var best = Int.MAX_VALUE
            for (i in 0 until videos.length()) {
                val v = videos.getJSONObject(i)
                if (v.optString("site") != "YouTube") continue
                val type = v.optString("type")
                val rank = when {
                    type == "Trailer" && v.optBoolean("official") -> 0
                    type == "Trailer" -> 1
                    type == "Teaser" -> 2
                    else -> 3
                }
                if (rank < best && v.optString("key").isNotEmpty()) {
                    best = rank; trailerKey = v.optString("key")
                }
            }
        }
        val cast = mutableListOf<CastMember>()
        json.optJSONObject("credits")?.optJSONArray("cast")?.let { arr ->
            for (i in 0 until arr.length().coerceAtMost(10)) {
                val o = arr.getJSONObject(i)
                val name = o.optString("name")
                if (name.isEmpty()) continue
                val profile = o.optString("profile_path").takeIf { it.isNotEmpty() }
                cast.add(CastMember(name, o.optString("character", o.optString("job", "")),
                    profile?.let { "${TmdbHome.IMG}/w185$it" }))
            }
        }
        val similar = mutableListOf<SimilarItem>()
        json.optJSONObject("similar")?.optJSONArray("results")?.let { arr ->
            for (i in 0 until arr.length()) {
                val o = arr.getJSONObject(i)
                val t = o.optString("title", o.optString("name", ""))
                val poster = o.optString("poster_path")
                if (t.isEmpty() || poster.isEmpty()) continue
                similar.add(SimilarItem(o.getInt("id"), t,
                    o.optString("release_date", o.optString("first_air_date", "")),
                    o.getDouble("vote_average").toFloat().takeIf { it > 0f } ?: 0f,
                    "${TmdbHome.IMG}/w500$poster"))
            }
        }
        val backdrop = json.optString("backdrop_path").takeIf { it.isNotEmpty() }
        return Details(
            id = id,
            title = title,
            date = date,
            genres = genres,
            overview = json.optString("overview", ""),
            rating = json.getDouble("vote_average").toFloat(),
            posterUrl = json.optString("poster_path").takeIf { it.isNotEmpty() }
                ?.let { "${TmdbHome.IMG}/w780$it" } ?: "",
            isTv = isTv,
            region = region,
            episodes = if (isTv) json.optInt("number_of_episodes").takeIf { it > 0 } else null,
            trailerKey = trailerKey,
            cast = cast,
            similar = similar,
        ).let { d ->
            // 幕布原色使用；backdrop 缺失时回退海报（w780 已在 posterUrl）
            if (backdrop != null) d.copy(posterUrl = "${TmdbHome.IMG}/w780$backdrop") else d
        }
    }

    private fun get(path: String): Pair<JSONObject?, String?> {
        if (client.apiKey().isEmpty()) return null to "offline"
        return try {
            val url = "https://api.themoviedb.org/3$path" +
                (if (path.contains("?")) "&api_key=${client.apiKey()}"
                 else "?api_key=${client.apiKey()}")
            val conn = client.openConnection(url)
            conn.connectTimeout = 8000
            conn.readTimeout = 15000
            val code = conn.responseCode
            val body = (if (code in 200..299) conn.inputStream else conn.errorStream)
                ?.bufferedReader()?.readText() ?: ""
            if (code !in 200..299) null to "server" else JSONObject(body) to null
        } catch (e: Exception) {
            null to "lost"
        }
    }
}
