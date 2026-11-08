package com.kivo.cinema.emby

import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URI
import java.net.URL

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

    private val base: String
    private val prefix: String

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

    private fun conn(url: String, method: String): HttpURLConnection {
        val c = URL(url).openConnection() as HttpURLConnection
        c.connectTimeout = 10000
        c.readTimeout = 20000
        c.requestMethod = method
        c.setRequestProperty("X-Emby-Authorization",
            "MediaBrowser Client=\"kivo\", Device=\"kivo-video\", " +
                "DeviceId=\"kivo-android\", Version=\"1.0\"")
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
    fun items(auth: Auth, limit: Int = 60): List<EmbyItem> {
        val q = "?IncludeItemTypes=Movie&Recursive=true&SortBy=DateCreated" +
            "&SortOrder=Descending&Limit=$limit&Fields=ProductionYear&ImageTypeLimit=1"
        val url = "$base$prefix/Users/${auth.userId}/Items$q"
        val c = conn(url, "GET")
        try {
            c.setRequestProperty("X-Emby-Token", auth.token)
            val code = c.responseCode
            if (code !in 200..299) throw HttpFailure(code)
            val body = c.inputStream.bufferedReader().use { it.readText() }
            val arr = JSONObject(body).getJSONArray("Items")
            val out = mutableListOf<EmbyItem>()
            for (i in 0 until arr.length()) {
                val o = arr.getJSONObject(i)
                val tag = o.optJSONObject("ImageTags")?.optString("Primary") ?: ""
                val year = if (o.has("ProductionYear") && !o.isNull("ProductionYear"))
                    o.getInt("ProductionYear") else null
                val poster = if (tag.isNotBlank())
                    "$base$prefix/Items/${o.getString("Id")}/Images/Primary" +
                        "?maxWidth=320&quality=75&tag=$tag" else ""
                out += EmbyItem(
                    id = o.getString("Id"),
                    name = o.getString("Name"),
                    year = year,
                    posterUrl = poster,
                )
            }
            return out
        } finally { c.disconnect() }
    }

    /** D3：直连流 URL（static=true + api_key）——Emby 原生 HTTP Range，喂 220b 会话。 */
    fun streamUrl(auth: Auth, itemId: String): String =
        "$base$prefix/Videos/$itemId/stream?static=true&api_key=${auth.token}"

    /** 海报字节下载（位图解码在 Activity 侧）。 */
    fun fetchBytes(url: String): ByteArray {
        val c = conn(url, "GET")
        try {
            val code = c.responseCode
            if (code !in 200..299) throw HttpFailure(code)
            return c.inputStream.use { it.readBytes() }
        } finally { c.disconnect() }
    }
}
