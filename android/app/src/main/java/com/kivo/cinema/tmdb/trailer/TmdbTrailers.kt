package com.kivo.cinema.tmdb.trailer

import android.os.SystemClock
import org.json.JSONObject

data class TmdbTrailer(val youtubeId: String, val name: String) {
    val watchUrl: String get() = "https://www.youtube.com/watch?v=$youtubeId"
}

data class TrailerResult(val video: TmdbTrailer? = null, val reason: String? = null)

/** Provider metadata contains platform IDs, never a playable media-file URL. */
internal class TmdbTrailers(private val fetch: (String) -> Pair<JSONObject?, String?>) {
    private data class Entry(val at: Long, val result: TrailerResult)
    private val memo = HashMap<Pair<Boolean, Int>, Entry>()

    @Synchronized
    fun forItem(isTv: Boolean, id: Int): TrailerResult {
        val key = isTv to id
        val now = SystemClock.elapsedRealtime()
        memo[key]?.takeIf { now - it.at < 15 * 60_000L }?.let { return it.result }
        val kind = if (isTv) "tv" else "movie"
        // No language filter: many titles have no Chinese trailer but do have
        // an official original-language trailer. Rank language after type.
        val (json, error) = fetch("/$kind/$id/videos")
        if (json == null) return TrailerResult(reason = when (error) {
            "offline" -> "网络未连接，暂时显示海报"
            "timeout" -> "预告片加载超时，暂时显示海报"
            else -> "预告片信息加载失败，暂时显示海报"
        })
        val rows = json.optJSONArray("results")
        val options = (0 until (rows?.length() ?: 0)).map { rows!!.getJSONObject(it) }
            .filter { it.optString("site") == "YouTube" &&
                it.optString("key").matches(Regex("[A-Za-z0-9_-]{11}")) &&
                it.optString("type") in listOf("Trailer", "Teaser") }
        val chosen = options.sortedWith(
            compareByDescending<JSONObject> { it.optBoolean("official") }
                .thenBy { if (it.optString("type") == "Trailer") 0 else 1 }
                .thenBy { when (it.optString("iso_639_1")) { "zh" -> 0; "en" -> 1; else -> 2 } }
                .thenByDescending { it.optString("published_at") }
        ).firstOrNull()
        val result = if (chosen == null) TrailerResult(reason = "暂无可播放的预告片")
            else TrailerResult(TmdbTrailer(chosen.getString("key"), chosen.optString("name")))
        memo[key] = Entry(now, result)
        return result
    }
}
