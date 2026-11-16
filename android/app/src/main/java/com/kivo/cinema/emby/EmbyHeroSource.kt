package com.kivo.cinema.emby

import org.json.JSONObject

/**
 * 317 · **服务器首页的数据源**：hero 候选 + 继续观看。
 *
 * 单一职责：只负责"首页要展示什么"，不碰详情/播放/收藏。
 * 连接与鉴权复用 [EmbyClient] 的 internal 出网口（与 [EmbyShows] 同一套口径）。
 *
 * ## 两个接口的选择依据
 * | 用途 | 接口 | 为什么是它 |
 * | --- | --- | --- |
 * | hero 候选 / 最近更新 | `Users/{uid}/Items?SortBy=DateCreated` | 服务器"刚入库"就是用户想看的"最新"；Emby 的 `Latest` 接口只给精选，覆盖面窄 |
 * | 继续观看 | `Users/{uid}/Items/Resume` | **Emby 原生**就带 `UserData.PlaybackPositionTicks`；自己拿列表再逐个查详情是 N 次请求 |
 *
 * ## 为什么 `Fields` 要显式列出
 * Emby 默认只回极简字段（Id/Name/Type）。`Overview` / `BackdropImageTags` /
 * `ChildCount` 不显式要就**完全不返回**（不是"返回空串"）—— 这一条真机踩过：
 * 少了 `BackdropImageTags` 就只能拿 480 宽海报铺屏，糊到不能看。
 */
class EmbyHeroSource(private val c: EmbyClient) {

    /** hero 与「最近更新」共用同一批字段。 */
    private val fields = listOf(
        "Overview", "ProductionYear", "Genres", "ProductionLocations",
        "PremiereDate", "CommunityRating", "ChildCount", "RecursiveItemCount",
        "DateCreated", "ImageTags", "BackdropImageTags", "RunTimeTicks",
    ).joinToString(",")

    /**
     * 最近入库的条目（hero 候选 = 前 [limit] 个）。
     *
     * @param parentId 媒体库 id；null = 整个服务器（跨库混合，Rex 的做法）
     */
    fun recent(auth: EmbyClient.Auth, parentId: String? = null, limit: Int = 12): List<EmbyHeroItem> {
        val parent = parentId?.let { "&ParentId=$it" } ?: ""
        val q = "?IncludeItemTypes=Movie,Series&Recursive=true$parent" +
            "&SortBy=DateCreated&SortOrder=Descending&Limit=$limit&Fields=$fields" +
            "&ImageTypeLimit=1&EnableImageTypes=Primary,Backdrop"
        return fetch("${c.base}${c.prefix}/Users/${auth.userId}/Items$q", auth)
    }

    /**
     * 继续观看（Emby 原生：带位置 + 总时长）。
     *
     * 【为什么用 Resume 而不是自己算】它已经把"看了但没看完、且位置 > 阈值"筛好了，
     * 排序也按最近观看 —— 自己拿播放记录算会和服务器口径不一致。
     */
    fun resume(auth: EmbyClient.Auth, limit: Int = 12): List<EmbyHeroItem> {
        val q = "?Limit=$limit&MediaTypes=Video&Fields=$fields" +
            "&ImageTypeLimit=1&EnableImageTypes=Primary,Backdrop"
        return fetch("${c.base}${c.prefix}/Users/${auth.userId}/Items/Resume$q", auth)
    }

    private fun fetch(url: String, auth: EmbyClient.Auth): List<EmbyHeroItem> {
        val conn = c.conn(url, "GET")
        try {
            conn.setRequestProperty("X-Emby-Token", auth.token)
            val code = conn.responseCode
            if (code !in 200..299) throw EmbyClient.HttpFailure(code)
            val arr = JSONObject(conn.inputStream.bufferedReader().use { it.readText() })
                .optJSONArray("Items") ?: return emptyList()
            val out = (0 until arr.length()).mapNotNull { i ->
                runCatching { parse(arr.getJSONObject(i)) }.getOrNull()
            }
            com.kivo.cinema.net.NetDiag.log("[emby] hero 数据 n=${out.size} url=${url.substringAfterLast("Items")}")
            return out
        } finally { conn.disconnect() }
    }

    private fun parse(o: JSONObject): EmbyHeroItem {
        val id = o.getString("Id")
        val isSeries = o.optString("Type") == "Series"
        fun strings(key: String): List<String> {
            val a = o.optJSONArray(key) ?: return emptyList()
            return (0 until a.length()).map { a.getString(it) }
        }
        // 大图：Backdrop 优先（横版天然铺屏），没有则退回 Primary（竖版）。
        val backdropTag = o.optJSONArray("BackdropImageTags")?.optString(0).orEmpty()
        val primaryTag = o.optJSONObject("ImageTags")?.optString("Primary").orEmpty()
        val heroUrl = when {
            backdropTag.isNotBlank() ->
                "${c.base}${c.prefix}/Items/$id/Images/Backdrop/0?maxWidth=1280&quality=88&tag=$backdropTag"
            primaryTag.isNotBlank() ->
                "${c.base}${c.prefix}/Items/$id/Images/Primary?maxWidth=1080&quality=88&tag=$primaryTag"
            else -> ""
        }
        val ud = o.optJSONObject("UserData")
        return EmbyHeroItem(
            id = id,
            name = o.optString("Name"),
            heroUrl = heroUrl,
            year = if (o.has("ProductionYear") && !o.isNull("ProductionYear")) o.getInt("ProductionYear") else null,
            countries = strings("ProductionLocations"),
            genres = strings("Genres"),
            premiereDate = o.optString("PremiereDate").take(10),
            // 剧集用 RecursiveItemCount（含所有季），没有则 ChildCount（单季时的集数）
            episodeCount = if (isSeries) {
                (if (o.has("RecursiveItemCount") && !o.isNull("RecursiveItemCount")) o.getInt("RecursiveItemCount") else null)
                    ?: (if (o.has("ChildCount") && !o.isNull("ChildCount")) o.getInt("ChildCount") else null)
            } else null,
            overview = o.optString("Overview"),
            rating = if (o.has("CommunityRating") && !o.isNull("CommunityRating")) o.getDouble("CommunityRating") else null,
            isSeries = isSeries,
            resumeMs = (ud?.optLong("PlaybackPositionTicks") ?: 0L) / 10_000L,
            runtimeMs = o.optLong("RunTimeTicks") / 10_000L,
        )
    }
}
