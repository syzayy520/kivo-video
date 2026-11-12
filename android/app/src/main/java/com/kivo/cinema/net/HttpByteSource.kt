package com.kivo.cinema.net

import java.net.HttpURLConnection
import java.net.URL

/**
 * HTTP Range 随机读源（254 D5）：Emby 直连流喂 mpv 内核 / 预览帧。
 *
 * token 走 X-Emby-Token 头，**永不进 URL**——mpv 日志里只有 localhost 代理地址。
 * 每次读独立连接、流读完关闭（不 disconnect）→ 复用 JVM keep-alive 池；
 * 代理层对 readAt 已串行化，这里无共享可变状态（size 除外，加锁）。
 * 服务器无视 Range（回 200）= 契约不成立，抛 reason，不静默降级。
 */
class HttpByteSource(private val url: String, private val token: String? = null) : ByteSource {

    @Volatile private var total: Long = -1

    @Synchronized
    override fun size(): Long {
        if (total >= 0) return total
        val c = open(0, 0)
        try {
            val code = c.responseCode
            checkAuth(code)
            val t = c.getHeaderField("Content-Range")?.substringAfterLast('/')?.toLongOrNull()
            if (code == 206 && t != null && t > 0) {
                total = t
                return t
            }
            throw IllegalStateException("服务器不支持 Range，无法播放")
        } finally {
            runCatching { c.inputStream?.close() }
            c.disconnect()
        }
    }

    override fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int {
        if (len == 0 || offset < 0) return 0
        val size = size()
        if (offset >= size) return -1
        val want = minOf(len.toLong(), size - offset).toInt()
        // 空 206（offset<size 却读不到字节）不是真 EOF：冷启主+预览双实例并发读时偶发瞬态空响应，
        // 直接报 -1 会让代理断连、FFmpeg 判 header EOF（257 首开瞬败）。有界重开连接重试。
        var attempt = 0
        while (true) {
            val got = readOnce(offset, want, buf, off)
            if (got != 0) return got
            attempt += 1
            if (attempt > MAX_EMPTY_RETRY) return -1
        }
    }

    private fun readOnce(offset: Long, want: Int, buf: ByteArray, off: Int): Int {
        val c = open(offset, offset + want - 1)
        var ok = false
        try {
            val code = c.responseCode
            checkAuth(code)
            if (code != 206) throw IllegalStateException("服务器不支持 Range（HTTP $code），无法播放")
            val stream = c.inputStream
            var got = 0
            while (got < want) {
                val n = stream.read(buf, off + got, want - got)
                if (n < 0) break
                got += n
            }
            stream.close()
            ok = true
            return got
        } finally {
            if (!ok) c.disconnect()
        }
    }

    override fun close() {
        // 无持久资源：连接随流关闭归池。
    }

    private fun checkAuth(code: Int) {
        when (code) {
            401, 403 -> throw IllegalStateException("服务器拒绝了访问（HTTP $code），请检查账号或权限")
            404 -> throw IllegalStateException("服务器上没有找到这个视频（HTTP 404）")
        }
    }

    private fun open(from: Long, to: Long): HttpURLConnection {
        val c = URL(url).openConnection() as HttpURLConnection
        c.connectTimeout = 10_000
        c.readTimeout = 20_000
        c.setRequestProperty("Range", "bytes=$from-$to")
        c.setRequestProperty("User-Agent", "kivo-video/1.0")
        token?.let { c.setRequestProperty("X-Emby-Token", it) }
        return c
    }

    companion object {
        private const val MAX_EMPTY_RETRY = 2
    }
}