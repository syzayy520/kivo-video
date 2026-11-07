package com.kivo.cinema.dav

import java.io.BufferedReader
import java.io.InputStreamReader
import java.net.HttpURLConnection
import java.net.URL
import com.kivo.cinema.smb.SmbSource
import javax.net.ssl.HttpsURLConnection

/**
 * WebDAV 源（221 设计稿 D1）：实现 [SmbSource.ByteSource] 同一接口——
 * LocalRangeProxy / NetworkPlaybackSession 直接复用，与 SMB 通道完全同构。
 *
 * 协议：PROPFIND(Depth:1) 浏览 + GET(Range) 随机读——java.net.HttpURLConnection
 * 内置实现，无第三方依赖。错误纪律（G9）：[DavException] 带阶段与目标。
 *
 * 线程模型：一个 [WebDavSource] 一个连接上下文；readAt 由播放线程串行调。
 */
class WebDavSource(private val config: WebDavConfig) {

    class DavException(stage: String, target: String, cause: Throwable? = null) :
        Exception("[$stage] $target: ${cause?.message ?: "unknown"}", cause)

    data class Entry(val name: String, val isDirectory: Boolean, val size: Long)

    /** Base URL 到当前路径的拼接（不带尾斜杠）。 */
    private fun baseUrl(): String {
        val pathPart = config.path.trim('/')
        return if (pathPart.isEmpty()) {
            "http://${config.host}:${config.port}/${config.share}"
        } else {
            "http://${config.host}:${config.port}/${config.share}/$pathPart"
        }
    }

    private fun openConnection(url: String, method: String): HttpURLConnection {
        val conn = URL(url).openConnection() as HttpURLConnection
        conn.requestMethod = method
        conn.connectTimeout = 8000
        conn.readTimeout = 15000
        if (config.username.isNotEmpty()) {
            val cred = android.util.Base64.encodeToString(
                "${config.username}:${config.password}".toByteArray(),
                android.util.Base64.NO_WRAP
            )
            conn.setRequestProperty("Authorization", "Basic $cred")
        }
        return conn
    }

    /** 浏览目录（PROPFIND Depth:1）→ 解析 XML multistatus。 */
    fun list(subPath: String = config.path): List<Entry> {
        val dirUrl = buildString {
            append(baseUrl())
            val sub = subPath.trim('/')
            if (sub.isNotEmpty()) append("/$sub")
            append("/")
        }
        try {
            val conn = openConnection(dirUrl, "PROPFIND")
            conn.setRequestProperty("Depth", "1")
            conn.setRequestProperty("Content-Type", "application/xml")
            conn.doOutput = true
            val propBody = """<?xml version="1.0" encoding="utf-8"?>
                |<D:propfind xmlns:D="DAV:"><D:prop>
                |<D:resourcetype/><D:getcontentlength/>
                |</D:prop></D:propfind>
            """.trimMargin()
            conn.outputStream.write(propBody.toByteArray())
            val code = conn.responseCode
            if (code != 207) {
                throw DavException("browse", dirUrl, Exception("HTTP $code"))
            }
            val body = conn.inputStream.bufferedReader().readText()
            conn.disconnect()
            return parseMultistatus(body, dirUrl)
        } catch (e: DavException) {
            throw e
        } catch (e: Exception) {
            throw DavException("browse", dirUrl, e)
        }
    }

    /** 解析 DAV:multistatus XML → 条目列表（跳过 self）。 */
    private fun parseMultistatus(xml: String, selfUrl: String): List<Entry> {
        val out = mutableListOf<Entry>()
        // 简易 XML 解析（不引第三方库——M1 最简策略）
        val responseRegex = Regex("<D:response>(.*?)</D:response>", RegexOption.DOT_MATCHES_ALL)
        val hrefRegex = Regex("<D:href>(.*?)</D:href>")
        val typeRegex = Regex("<D:collection\\s*/?>")
        val sizeRegex = Regex("<D:getcontentlength>(\\d+)</D:getcontentlength>")
        val selfNorm = selfUrl.trimEnd('/')

        for (match in responseRegex.findAll(xml)) {
            val block = match.groupValues[1]
            val href = hrefRegex.find(block)?.groupValues[1]
                ?: continue
            val decoded = java.net.URLDecoder.decode(href, "UTF-8")
            // 跳过 self（URL 解码后对比尾段）
            if (decoded.trimEnd('/') == selfNorm.trimEnd('/')) continue
            val isDir = typeRegex.containsMatchIn(block)
            val size = sizeRegex.find(block)?.groupValues[1]?.toLong() ?: 0
            val name = decoded.trimEnd('/').substringAfterLast('/')
            if (name.isNotEmpty()) {
                out.add(Entry(name, isDir, size))
            }
        }
        return out.sortedWith(compareByDescending<Entry> { it.isDirectory }.thenBy { it.name.lowercase() })
    }

    /** 打开文件为 ByteSource（HEAD 取大小 + GET Range 随机读）。 */
    fun openForRead(filePath: String): HttpByteSource {
        val fileUrl = buildString {
            append(baseUrl())
            val sub = filePath.trim('/')
            if (sub.isNotEmpty()) append("/$sub")
        }
        // HEAD 取大小
        val headConn = openConnection(fileUrl, "HEAD")
        val code = headConn.responseCode
        if (code != 200) {
            headConn.disconnect()
            throw DavException("open", fileUrl, Exception("HTTP $code"))
        }
        val size = headConn.getContentLengthLong()
        headConn.disconnect()
        return HttpByteSource(fileUrl, size)
    }

    /** HTTP Range 随机读源（实现 ByteSource 接口）。 */
    inner class HttpByteSource(private val fileUrl: String, private val sizeBytes: Long) :
        SmbSource.ByteSource {
        override fun size(): Long = sizeBytes

        override fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int {
            try {
                val conn = openConnection(fileUrl, "GET")
                conn.setRequestProperty("Range", "bytes=$offset-${offset + len - 1}")
                val code = conn.responseCode
                if (code == 416) {
                    conn.disconnect()
                    return -1  // EOF
                }
                if (code != 200 && code != 206) {
                    conn.disconnect()
                    throw DavException("read", "$fileUrl@$offset", Exception("HTTP $code"))
                }
                val stream = conn.inputStream
                var total = 0
                while (total < len) {
                    val n = stream.read(buf, off + total, len - total)
                    if (n < 0) break
                    total += n
                }
                conn.disconnect()
                return if (total == 0) -1 else total
            } catch (e: DavException) {
                throw e
            } catch (e: Exception) {
                throw DavException("read", "$fileUrl@$offset", e)
            }
        }

        override fun close() { /* HttpURLConnection 无需显式关闭 */ }
    }
}

/**
 * WebDAV 连接描述（221）：与 [SmbConfig] 同构。
 */
data class WebDavConfig(
    val host: String,
    val port: Int = 8080,
    val share: String = "",
    val path: String = "",
    val username: String = "",
    val password: String = ""
) {
    fun toJson(): String {
        val o = org.json.JSONObject()
        o.put("host", host)
        o.put("port", port)
        o.put("share", share)
        o.put("path", path)
        o.put("username", username)
        o.put("password", password)
        return o.toString()
    }

    companion object {
        fun fromJson(s: String): WebDavConfig? = runCatching {
            val o = org.json.JSONObject(s)
            WebDavConfig(
                host = o.getString("host"),
                port = o.optInt("port", 8080),
                share = o.optString("share", ""),
                path = o.optString("path", ""),
                username = o.optString("username", ""),
                password = o.optString("password", "")
            )
        }.getOrNull()
    }
}
