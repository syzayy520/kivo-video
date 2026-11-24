package com.kivo.cinema.subtitle.online

import com.kivo.cinema.tmdb.TmdbClient
import org.json.JSONObject
import java.io.IOException
import java.net.HttpURLConnection
import java.net.URL
import java.net.URLEncoder

/**
 * OpenSubtitles 官方 REST API v1 客户端（字段以真实响应为准，见 research.md）。
 * 搜索用 [TmdbClient.get]（匿名可搜，已实测）；登录/下载需要 Api-Key 与 Bearer（用户设置里填）。
 */
class OpenSubtitlesClient(private val tmdb: TmdbClient, private val prefs: OnlineSubtitlePrefs) {

    fun search(
        query: String,
        languages: String = "",
        season: Int? = null,
        episode: Int? = null,
        page: Int = 1,
    ): List<OnlineSubtitle> {
        val url = buildString {
            append("https://api.opensubtitles.com/api/v1/subtitles?query=")
            append(URLEncoder.encode(query, "UTF-8"))
            if (languages.isNotBlank()) append("&languages=$languages")
            season?.let { append("&season=$it") }
            episode?.let { append("&episode=$it") }
            append("&page=$page")
        }
        val body = httpGet(tmdb, url) ?: throw IOException("OpenSubtitles 搜索失败（网络或服务异常）")
        val data = JSONObject(body).optJSONArray("data") ?: return emptyList()
        return (0 until data.length()).mapNotNull { i ->
            runCatching { parse(data.getJSONObject(i)) }.getOrNull()
        }
    }

    private fun parse(item: JSONObject): OnlineSubtitle {
        val a = item.getJSONObject("attributes")
        val files = a.optJSONArray("files")
        val fileId = files?.optJSONObject(0)?.optLong("file_id") ?: 0L
        val language = a.optString("language")
        return OnlineSubtitle(
            source = SubSource.OpenSubtitles,
            id = item.optString("id"),
            title = a.optString("release").ifBlank { a.optString("slug") },
            languageCode = language,
            languageLabel = languageLabelOf(language),
            format = "SRT",
            downloads = a.optInt("download_count"),
            ratingUp = a.optInt("ratings").toInt().coerceAtLeast(0),
            ratingDown = 0,
            uploader = a.optJSONObject("uploader")?.optString("name") ?: "",
            releaseName = a.optString("release"),
            pageUrl = a.optString("url"),
            fileId = fileId,
            trusted = a.optBoolean("from_trusted"),
        )
    }

    /** 登录（设置里账号密码）：成功返回 Bearer token 并落盘。 */
    fun login(username: String, password: String): String {
        val body = post(
            "https://api.opensubtitles.com/api/v1/login",
            JSONObject().put("username", username).put("password", password).toString(),
            bearer = null,
        )
        val token = JSONObject(body).optString("token")
        if (token.isBlank()) throw IOException("OpenSubtitles 登录失败：${body.take(160)}")
        prefs.bearerToken = token
        return token
    }

    /** 取下载直链：POST /download {file_id} → JSON.link（真实文件地址）。 */
    fun downloadLink(fileId: Long): String {
        val token = prefs.bearerToken
        val body = post(
            "https://api.opensubtitles.com/api/v1/download",
            JSONObject().put("file_id", fileId).toString(),
            bearer = token.ifBlank { null },
        )
        val link = JSONObject(body).optString("link")
        if (link.isBlank()) throw IOException("OpenSubtitles 下载需要登录账号（设置 → 播放体验 → 字幕里登录）")
        return link
    }

    private fun post(url: String, jsonBody: String, bearer: String?): String {
        val conn = URL(url).openConnection() as HttpURLConnection
        try {
            conn.requestMethod = "POST"
            conn.connectTimeout = 20_000
            conn.readTimeout = 30_000
            conn.setRequestProperty("Content-Type", "application/json")
            conn.setRequestProperty("Accept", "application/json")
            if (prefs.apiKey.isNotBlank()) conn.setRequestProperty("Api-Key", prefs.apiKey)
            bearer?.let { conn.setRequestProperty("Authorization", "Bearer $it") }
            conn.doOutput = true
            conn.outputStream.use { it.write(jsonBody.toByteArray()) }
            val code = conn.responseCode
            val text = (if (code in 200..299) conn.inputStream else conn.errorStream)
                .readBytes().decodeToString()
            if (code !in 200..299) throw IOException("OpenSubtitles HTTP $code：${text.take(160)}")
            return text
        } finally {
            conn.disconnect()
        }
    }
}