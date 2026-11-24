package com.kivo.cinema.subtitle.online

import com.kivo.cinema.tmdb.TmdbClient
import org.json.JSONObject
import java.io.IOException
import java.net.HttpURLConnection
import java.net.URL
import java.net.URLEncoder

/**
 * SubHD（射手网）网页客户端：搜索/详情解析/准备下载。
 * 实拉确认：搜索页给 /d/<影视id>，详情页给 /a/<字幕id> 与语言/格式/下载量；
 * 下载用 POST /api/sub/prepare-download {sid} → /down/<sid>（时效链接，Cloudflare 可能拦截，
 * 拦截时由上层给可见原因，不静默降级）。
 */
class SubHdClient(private val tmdb: TmdbClient) {

    private val hosts = listOf("https://subhd.tv", "https://subhd.me")

    fun search(query: String, year: Int? = null): List<OnlineSubtitle> {
        val ids = mediaIds(query, year)
            ?: throw IOException("SubHD 搜索失败（网络不可达或站点被墙，请配置代理后重试）")
        if (ids.isEmpty()) throw IOException("SubHD 没有找到相关影视，换个片名试试")
        val results = ids.map { (id, _) ->
            runCatching { subtitlesOf(id) }
                .getOrElse { if (it is IOException) throw it else emptyList() }
        }
        val all = results.flatten()
        if (all.isEmpty()) throw IOException("SubHD 搜索到了影视但未能解析出字幕（网络不可达或站点被墙）")
        return all
    }

    private fun mediaIds(query: String, year: Int?): List<Pair<String, Int?>>? {
        val body = get("/search/${URLEncoder.encode(query, "UTF-8")}") ?: return null
        val out = LinkedHashSet<String>()
        Regex("/d/(\\d+)").findAll(body).forEach { out += it.groupValues[1] }
        return out.take(3).map { it to year }
    }

    private fun subtitlesOf(mediaId: String): List<OnlineSubtitle> {
        val body = get("/d/$mediaId") ?: return emptyList()
        val results = mutableListOf<OnlineSubtitle>()
        Regex("<a href=\"(/a/[A-Za-z0-9]+)\">(.*?)</a>").findAll(body).forEach { m ->
            val path = m.groupValues[1]
            val rawTitle = m.groupValues[2].replace(Regex("<[^>]+>"), "").trim()
            if (rawTitle.isBlank()) return@forEach
            val tail = body.substring(m.range.last + 1).take(420)
            val format = Regex("\\b(SRT|ASS|SUP)\\b", RegexOption.IGNORE_CASE).find(tail)?.value?.uppercase() ?: "未知"
            val downloads = Regex("(\\d{2,})").find(tail.replace(Regex("\\s+"), " "))?.value?.toIntOrNull() ?: 0
            val language = when {
                "双语" in tail -> "双语"
                "简体" in tail -> "简体"
                "繁体" in tail -> "繁体"
                "英语" in tail -> "英语"
                else -> "其他"
            }
            results += OnlineSubtitle(
                source = SubSource.SubHd,
                id = path.removePrefix("/a/"),
                title = rawTitle,
                languageCode = language,
                languageLabel = language,
                format = format,
                downloads = downloads,
                ratingUp = 0,
                ratingDown = 0,
                uploader = "",
                releaseName = rawTitle,
                pageUrl = "$activeHost$path",
            )
        }
        return results
    }

    /** 准备下载：返回 /down/<sid> 的完整 URL（时效；Cloudflare 拦截时上层给可见原因）。 */
    fun prepareDownload(subtitle: OnlineSubtitle): String {
        val sid = subtitle.id
        val body = post("$activeHost/api/sub/prepare-download", "{\"sid\":\"$sid\"}")
        val path = JSONObject(body).optString("url")
        if (path.isBlank()) throw IOException("SubHD 准备下载失败：${body.take(160)}")
        return "$activeHost$path"
    }

    @Volatile private var activeHost = hosts[0]

    private fun get(path: String): String? {
        hosts.forEach { host ->
            val body = httpGet(tmdb, "$host$path")
            if (body != null) {
                activeHost = host
                return body
            }
        }
        return null
    }

    private fun post(url: String, jsonBody: String): String {
        val conn = URL(url).openConnection() as HttpURLConnection
        try {
            conn.requestMethod = "POST"
            conn.connectTimeout = 20_000
            conn.readTimeout = 30_000
            conn.setRequestProperty("Content-Type", "application/json")
            conn.setRequestProperty("User-Agent", "Mozilla/5.0")
            conn.doOutput = true
            conn.outputStream.use { it.write(jsonBody.toByteArray()) }
            val code = conn.responseCode
            val text = (if (code in 200..299) conn.inputStream else conn.errorStream)
                .readBytes().decodeToString()
            if (code !in 200..299) throw IOException("SubHD HTTP $code")
            return text
        } finally {
            conn.disconnect()
        }
    }
}