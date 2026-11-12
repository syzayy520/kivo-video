package com.kivo.cinema.emby

import org.json.JSONObject

/**
 * 255：服务器自有分类（/Users/{uid}/Views）。首页把每个 View 渲染成一条横排
 * （电视-韩国 / 电影-华语 …），CollectionType 决定钻取网格是电影还是剧。
 * 单一职责：分类目录 HTTP+JSON；连接/鉴权复用 [EmbyClient] 的 internal 出网口。
 */
class EmbyCatalog(private val c: EmbyClient) {

    data class View(
        val id: String, val name: String, val collectionType: String,
        val parentId: String? = null, val genres: String? = null,
    )

    fun views(auth: EmbyClient.Auth): List<View> {
        val conn = c.conn("${c.base}${c.prefix}/Users/${auth.userId}/Views", "GET")
        try {
            conn.setRequestProperty("X-Emby-Token", auth.token)
            val code = conn.responseCode
            if (code !in 200..299) throw EmbyClient.HttpFailure(code)
            val arr = JSONObject(conn.inputStream.bufferedReader().use { it.readText() })
                .getJSONArray("Items")
            return (0 until arr.length()).map {
                val o = arr.getJSONObject(it)
                View(o.getString("Id"), o.getString("Name"), o.optString("CollectionType"),
                    parentId = o.getString("Id"))
            }
        } finally { conn.disconnect() }
    }

    /** 255：服务器未建"综艺"视图时按类型补一行（Series+真人秀 genre，实测有内容才返回）。 */
    fun varietyView(auth: EmbyClient.Auth): View? = runCatching {
        val page = c.items(auth, limit = 1, types = "Series", genres = "真人秀")
        if ((page.total ?: page.items.size) > 0)
            View("genre-variety", "综艺", "tvshows", parentId = null, genres = "真人秀")
        else null
    }.getOrNull()
}