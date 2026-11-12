package com.kivo.cinema.emby

import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URI
import java.net.URL
import java.net.URLEncoder

/**
 * Emby REST 客户端（224 设计稿 D1-D3）：login / items / streamUrl。
 *
 * 协议骨架来自 Emby 官方 REST（服务器 4.8.0.62 实测钉死，见设计稿 §二）：
 * AuthenticateByName → AccessToken/User.Id；/Users/{uid}/Items 海报墙；
 * /Videos/{id}/stream?static=true 直连流（原生 HTTP Range → 220b 管线）。
 *
 * 单一职责：HTTP+JSON 封装——UI 在 EmbyActivity，解码在既有会话类。
 * 全同步阻塞，调用方自备线程。零第三方依赖（org.json 为平台内置）。
 */
class EmbyClient(serverInput: String) {

    /** Safe protocol status; never carry response bodies into UI or logs. */
    class HttpFailure(val code: Int) : Exception("Emby HTTP $code")

    /** Memory-only login result. */
    data class Auth(val token: String, val userId: String)

    internal val base: String
    internal val prefix: String

    init {
        // 归一化：无 scheme 补 https://；用户带了路径就用该路径作 API 前缀，否则 /emby
        var s = serverInput.trim()
        if (!s.startsWith("http://") && !s.startsWith("https://")) s = "https://$s"
        val uri = URI(s.trimEnd('/'))
        require(uri.scheme in listOf("http", "https") && !uri.host.isNullOrBlank() && uri.userInfo == null && uri.rawQuery == null && uri.rawFragment == null && uri.port in -1..65535)
        val path = uri.rawPath ?: ""
        base = "${uri.scheme}://${uri.host}${if (uri.port > 0) ":${uri.port}" else ""}"
        prefix = if (path.isNotBlank() && path != "/") path.trimEnd('/') else "/emby"
    }

    internal fun conn(url: String, method: String): HttpURLConnection {
        val c = URL(url).openConnection() as HttpURLConnection
        c.connectTimeout = 10000
        c.readTimeout = 20000
        c.requestMethod = method
        c.setRequestProperty("X-Emby-Authorization",
            "MediaBrowser Client=\"kivo\", Device=\"kivo-video\", " +
                "DeviceId=\"kivo-android\", Version=\"1.0\"")
        // 反代可能按 UA 拦（实测 python-urllib 被 403）；固定 UA 保证行为可复算。
        c.setRequestProperty("User-Agent", "kivo-video/1.0")
        return c
    }

    /** D1：AuthenticateByName。空密码 = Pw:""。失败只暴露 HTTP 状态码。 */
    fun login(username: String, password: String): Auth {
        val url = "$base$prefix/Users/AuthenticateByName"
        val c = conn(url, "POST")
        try {
            c.doOutput = true
            c.setRequestProperty("Content-Type", "application/json")
            c.outputStream.use {
                it.write(JSONObject().apply {
                    put("Username", username)
                    put("Pw", password)
                }.toString().toByteArray())
            }
            val code = c.responseCode
            if (code !in 200..299) throw HttpFailure(code)
            val body = c.inputStream.bufferedReader().use { it.readText() }
            val json = JSONObject(body)
            val token = json.getString("AccessToken")
            val userId = json.getJSONObject("User").getString("Id")
            return Auth(token, userId)
        } finally { c.disconnect() }
    }

    /** D2：海报墙条目（Movie，按加入时间倒序）。失败只暴露 HTTP 状态码。 */
    data class Page(val items: List<EmbyItem>, val total: Int?)

    fun items(auth: Auth, start: Int = 0, search: String = "", sort: Int = 0, limit: Int = 60,
              parentId: String? = null, types: String = "Movie", genres: String? = null): Page {
        require(start >= 0 && limit in 1..100 && sort in 0..2)
        val order = listOf("DateCreated", "SortName", "ProductionYear")[sort]
        val parent = parentId?.let { "&ParentId=$it" } ?: ""
        val genre = genres?.let { "&Genres=${URLEncoder.encode(it, "UTF-8")}" } ?: ""
        val q = "?IncludeItemTypes=$types&Recursive=true$parent$genre&SortBy=$order,Id" +
            "&SortOrder=${if (sort == 1) "Ascending" else "Descending"}&StartIndex=$start" +
            "&Limit=$limit&Fields=ProductionYear&ImageTypeLimit=1&EnableTotalRecordCount=true" +
            "&SearchTerm=${URLEncoder.encode(search.trim(), "UTF-8")}"
        val url = "$base$prefix/Users/${auth.userId}/Items$q"
        val c = conn(url, "GET")
        try {
            c.setRequestProperty("X-Emby-Token", auth.token)
            val code = c.responseCode
            if (code !in 200..299) throw HttpFailure(code)
            val body = c.inputStream.bufferedReader().use { it.readText() }
            val json = JSONObject(body)
            val arr = json.getJSONArray("Items")
            val out = mutableListOf<EmbyItem>()
            for (i in 0 until arr.length()) out += parseItem(arr.getJSONObject(i))
            return Page(out, if (json.has("TotalRecordCount") && !json.isNull("TotalRecordCount"))
                json.getInt("TotalRecordCount").coerceAtLeast(0) else null)
        } finally { c.disconnect() }
    }

    data class Counts(val movies: Int, val series: Int)

    /** Availability only; authentication and count permissions are separate capabilities. */
    fun isReachable(): Boolean = runCatching {
        val c = conn("$base$prefix/System/Info/Public", "GET")
        c.connectTimeout = 3000
        c.readTimeout = 3000
        try { c.responseCode in 200..499 } finally { c.disconnect() }
    }.getOrDefault(false)

    /** Counts visible to this user; SeriesCount counts shows, not individual episodes. */
    fun counts(auth: Auth): Counts {
        val c = conn("$base$prefix/Items/Counts?UserId=${URLEncoder.encode(auth.userId, "UTF-8")}", "GET")
        try {
            c.setRequestProperty("X-Emby-Token", auth.token)
            val code = c.responseCode
            if (code !in 200..299) throw HttpFailure(code)
            val json = JSONObject(c.inputStream.bufferedReader().use { it.readText() })
            return Counts(json.getInt("MovieCount"), json.getInt("SeriesCount")).also {
                require(it.movies >= 0 && it.series >= 0)
            }
        } finally { c.disconnect() }
    }

    /** D3：直连流 URL（static=true）。token 走 X-Emby-Token 头（254），永不进 URL/日志。 */
    fun streamUrl(auth: Auth, itemId: String): String =
        "$base$prefix/Videos/$itemId/stream?static=true"

    /** 海报/详情图字节下载（位图解码在调用侧）。token 可选：匿名图不带头。 */
    fun fetchBytes(url: String, token: String? = null): ByteArray {
        val c = conn(url, "GET")
        try {
            token?.let { c.setRequestProperty("X-Emby-Token", it) }
            val code = c.responseCode
            if (code !in 200..299) throw HttpFailure(code)
            return c.inputStream.use { it.readBytes() }
        } finally { c.disconnect() }
    }

    internal fun parseItem(o: JSONObject): EmbyItem {
        val tag = o.optJSONObject("ImageTags")?.optString("Primary") ?: ""
        val year = if (o.has("ProductionYear") && !o.isNull("ProductionYear"))
            o.getInt("ProductionYear") else null
        val poster = if (tag.isNotBlank())
            "$base$prefix/Items/${o.getString("Id")}/Images/Primary" +
                "?maxWidth=320&quality=75&tag=$tag" else ""
        return EmbyItem(o.getString("Id"), o.getString("Name"), year, poster,
            o.optString("Type") == "Series")
    }

    /** 254 D2：详情单请求。字段集经服务器 4.8 实测（设计稿 §二），UserData 随带。 */
    fun details(auth: Auth, itemId: String): EmbyDetails {
        val fields = "Overview,People,Genres,Studios,MediaSources,MediaStreams," +
            "ProductionLocations,PremiereDate,CommunityRating,RunTimeTicks,ImageTags"
        val c = conn("$base$prefix/Users/${auth.userId}/Items/$itemId?Fields=$fields", "GET")
        try {
            c.setRequestProperty("X-Emby-Token", auth.token)
            val code = c.responseCode
            if (code !in 200..299) throw HttpFailure(code)
            val o = JSONObject(c.inputStream.bufferedReader().use { it.readText() })
            fun image(type: String, tag: String, width: Int) = if (tag.isNotBlank())
                "$base$prefix/Items/$itemId/Images/$type?maxWidth=$width&quality=80&tag=$tag" else ""
            fun strings(key: String): List<String> {
                val a = o.optJSONArray(key) ?: return emptyList()
                return (0 until a.length()).map { a.getString(it) }
            }
            val people = mutableListOf<EmbyPerson>()
            val pa = o.optJSONArray("People")
            if (pa != null) for (i in 0 until pa.length()) {
                val p = pa.getJSONObject(i)
                val pid = p.optString("Id")
                val ptag = p.optString("PrimaryImageTag")
                people += EmbyPerson(pid, p.optString("Name"), p.optString("Role"), p.optString("Type"),
                    if (pid.isNotBlank() && ptag.isNotBlank())
                        "$base$prefix/Items/$pid/Images/Primary?maxWidth=160&quality=80&tag=$ptag" else "")
            }
            val ms = o.optJSONArray("MediaSources")?.optJSONObject(0)
            val streams = ms?.optJSONArray("MediaStreams")
            fun titles(type: String): List<String> {
                if (streams == null) return emptyList()
                return (0 until streams.length()).map { streams.getJSONObject(it) }
                    .filter { it.optString("Type") == type }
                    .map { it.optString("DisplayTitle").ifBlank { it.optString("Codec") } }
            }
            val videoObj = streams?.let { arr ->
                (0 until arr.length()).map { arr.getJSONObject(it) }
                    .firstOrNull { it.optString("Type") == "Video" }
            }
            val ud = o.optJSONObject("UserData")
            return EmbyDetails(
                id = itemId,
                name = o.optString("Name"),
                year = if (o.has("ProductionYear") && !o.isNull("ProductionYear")) o.getInt("ProductionYear") else null,
                overview = o.optString("Overview"),
                premiereDate = o.optString("PremiereDate").take(10),
                countries = strings("ProductionLocations"),
                runtimeMinutes = o.optLong("RunTimeTicks") / 600_000_000L,
                genres = strings("Genres"),
                rating = if (o.has("CommunityRating") && !o.isNull("CommunityRating")) o.getDouble("CommunityRating") else null,
                studios = (0 until (o.optJSONArray("Studios")?.length() ?: 0))
                    .map { o.optJSONArray("Studios").getJSONObject(it).optString("Name") },
                people = people,
                media = if (ms != null) EmbyMediaInfo(ms.optString("Container"), ms.optLong("Size"),
                    ms.optLong("Bitrate"), videoObj?.optInt("Width") ?: 0, videoObj?.optInt("Height") ?: 0,
                    titles("Video"), titles("Audio"), titles("Subtitle")) else null,
                posterUrl = image("Primary", o.optJSONObject("ImageTags")?.optString("Primary").orEmpty(), 780),
                backdropUrl = image("Backdrop", o.optJSONArray("BackdropImageTags")?.optString(0).orEmpty(), 1280),
                logoUrl = image("Logo", o.optJSONObject("ImageTags")?.optString("Logo").orEmpty(), 800),
                isFavorite = ud?.optBoolean("IsFavorite") ?: false,
                played = ud?.optBoolean("Played") ?: false,
            )
        } finally { c.disconnect() }
    }

    /** 254 D4：收藏/已看是服务端事实；false = 失败，调用方回滚并给 reason。 */
    fun setFavorite(auth: Auth, itemId: String, on: Boolean): Boolean =
        userDataWrite(auth, "FavoriteItems", itemId, on)

    fun setPlayed(auth: Auth, itemId: String, on: Boolean): Boolean =
        userDataWrite(auth, "PlayedItems", itemId, on)

    private fun userDataWrite(auth: Auth, kind: String, itemId: String, on: Boolean): Boolean {
        val c = conn("$base$prefix/Users/${auth.userId}/$kind/$itemId", if (on) "POST" else "DELETE")
        try {
            c.setRequestProperty("X-Emby-Token", auth.token)
            return c.responseCode in 200..299
        } finally { c.disconnect() }
    }

    /** 254 D10：相似作品横排。 */
    fun similar(auth: Auth, itemId: String, limit: Int = 12): List<EmbyItem> {
        val c = conn("$base$prefix/Items/$itemId/Similar?UserId=${auth.userId}&Limit=$limit", "GET")
        try {
            c.setRequestProperty("X-Emby-Token", auth.token)
            val code = c.responseCode
            if (code !in 200..299) throw HttpFailure(code)
            val arr = JSONObject(c.inputStream.bufferedReader().use { it.readText() }).getJSONArray("Items")
            return (0 until arr.length()).map { parseItem(arr.getJSONObject(it)) }
        } finally { c.disconnect() }
    }
}
