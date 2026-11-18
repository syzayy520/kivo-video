package com.kivo.cinema.emby

import org.json.JSONObject

/**
 * 255：剧集的季与集（/Shows/{id}/Seasons、/Shows/{id}/Episodes?SeasonId=）。
 * 剧详情"第 N 季 ⌄ + 集缩略图横排"的数据源；字段集经服务器 4.8 实测钉死。
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
        // D366：季表 6h 缓存（写操作会让该 series 失效），二次进详情不再重打源站。
        val body = c.cachedBody(
            "${c.base}${c.prefix}/Shows/$seriesId/Seasons?UserId=${auth.userId}",
            auth.token, EmbyResponseCache.TTL_EPISODES,
        )
        val arr = JSONObject(body).getJSONArray("Items")
        val list = (0 until arr.length()).map {
            val o = arr.getJSONObject(it)
            Season(o.getString("Id"), o.getString("Name"), o.optInt("IndexNumber"))
        }.sortedBy { it.index }
        com.kivo.cinema.net.NetDiag.log(
            "[emby] seasons series=$seriesId n=${list.size} " +
                list.take(4).joinToString { "S${it.index}:${it.name}#${it.id.take(8)}" },
        )
        return list
    }

    fun episodes(auth: EmbyClient.Auth, seriesId: String, seasonId: String): List<Episode> {
        // D366：集表同样 6h 缓存 —— 剧集详情 2.38s 里 1.59s 花在四季并发拉集上。
        val url = "${c.base}${c.prefix}/Shows/$seriesId/Episodes?UserId=${auth.userId}" +
            "&SeasonId=$seasonId&Fields=RunTimeTicks,Overview,PremiereDate&EnableTotalRecordCount=false"
        val body = c.cachedBody(url, auth.token, EmbyResponseCache.TTL_EPISODES)
        val arr = JSONObject(body).getJSONArray("Items")
        com.kivo.cinema.net.NetDiag.log(
            "[emby] episodes series=$seriesId season=$seasonId n=${arr.length()}",
        )
        return parseEpisodes(arr).sortedBy { it.index }
    }

    /**
     * 全剧集**一次请求**（不带 SeasonId）。
     *
     * 【为什么】逐季拉集时 N 季就是 N 次往返（真机 47 集单季剧实测 0.4–2.4 s）。
     * Emby 该端点不带 `SeasonId` 时返回整剧的集（含 `ParentIndexNumber` = 季号），
     * 一次拿全即可按季分组。**是否分页截断由调用方校验**：拿到的季不全就退回逐季请求。
     */
    fun allEpisodes(auth: EmbyClient.Auth, seriesId: String): List<Episode> {
        val url = "${c.base}${c.prefix}/Shows/$seriesId/Episodes?UserId=${auth.userId}" +
            "&Fields=RunTimeTicks,Overview,PremiereDate&EnableTotalRecordCount=false"
        val body = c.cachedBody(url, auth.token, EmbyResponseCache.TTL_EPISODES)
        val arr = JSONObject(body).getJSONArray("Items")
        val eps = parseEpisodes(arr)
        com.kivo.cinema.net.NetDiag.log(
            "[emby] all-episodes series=$seriesId n=${eps.size} " +
                "seasons=${eps.map { it.seasonIndex }.distinct().sorted()}",
        )
        return eps
    }

    private fun parseEpisodes(arr: org.json.JSONArray): List<Episode> = (0 until arr.length()).map {
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
    }
}