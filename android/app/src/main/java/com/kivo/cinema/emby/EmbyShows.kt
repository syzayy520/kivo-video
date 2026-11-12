package com.kivo.cinema.emby

import org.json.JSONObject

/**
 * 255：剧集的季与集（/Shows/{id}/Seasons、/Shows/{id}/Episodes?SeasonId=）。
 * REX 剧详情"第 N 季 ⌄ + 集缩略图横排"的数据源；字段集经服务器 4.8 实测钉死。
 * 单一职责：季集 HTTP+JSON；连接/鉴权复用 [EmbyClient] 的 internal 出网口。
 */
class EmbyShows(private val c: EmbyClient) {

    data class Season(val id: String, val name: String, val index: Int)

    data class Episode(
        val id: String, val name: String, val index: Int, val seasonIndex: Int,
        val runtimeMinutes: Long, val thumbUrl: String,
        val played: Boolean, val positionTicks: Long,
        val airDate: String, val overview: String,
    )

    fun seasons(auth: EmbyClient.Auth, seriesId: String): List<Season> {
        val conn = c.conn("${c.base}${c.prefix}/Shows/$seriesId/Seasons?UserId=${auth.userId}", "GET")
        try {
            conn.setRequestProperty("X-Emby-Token", auth.token)
            val code = conn.responseCode
            if (code !in 200..299) throw EmbyClient.HttpFailure(code)
            val arr = JSONObject(conn.inputStream.bufferedReader().use { it.readText() })
                .getJSONArray("Items")
            return (0 until arr.length()).map {
                val o = arr.getJSONObject(it)
                Season(o.getString("Id"), o.getString("Name"), o.optInt("IndexNumber"))
            }.sortedBy { it.index }
        } finally { conn.disconnect() }
    }

    fun episodes(auth: EmbyClient.Auth, seriesId: String, seasonId: String): List<Episode> {
        val url = "${c.base}${c.prefix}/Shows/$seriesId/Episodes?UserId=${auth.userId}" +
            "&SeasonId=$seasonId&Fields=RunTimeTicks,Overview,PremiereDate&EnableTotalRecordCount=false"
        val conn = c.conn(url, "GET")
        try {
            conn.setRequestProperty("X-Emby-Token", auth.token)
            val code = conn.responseCode
            if (code !in 200..299) throw EmbyClient.HttpFailure(code)
            val arr = JSONObject(conn.inputStream.bufferedReader().use { it.readText() })
                .getJSONArray("Items")
            return (0 until arr.length()).map {
                val o = arr.getJSONObject(it)
                val tag = o.optJSONObject("ImageTags")?.optString("Primary") ?: ""
                val ud = o.optJSONObject("UserData")
                Episode(
                    o.getString("Id"), o.getString("Name"), o.optInt("IndexNumber"),
                    o.optInt("ParentIndexNumber"), o.optLong("RunTimeTicks") / 600_000_000L,
                    if (tag.isNotBlank()) "${c.base}${c.prefix}/Items/${o.getString("Id")}" +
                        "/Images/Primary?maxWidth=780&quality=90&tag=$tag" else "",
                    ud?.optBoolean("Played") ?: false, ud?.optLong("PlaybackPositionTicks") ?: 0L,
                    o.optString("PremiereDate").take(10), o.optString("Overview"),
                )
            }.sortedBy { it.index }
        } finally { conn.disconnect() }
    }
}