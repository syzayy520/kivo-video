package com.kivo.cinema.mpv

import com.kivo.cinema.net.ByteSource
import java.io.BufferedOutputStream
import java.io.BufferedReader
import java.io.IOException
import java.net.InetAddress
import java.net.ServerSocket
import java.net.Socket
import java.util.concurrent.atomic.AtomicLong

/**
 * 253-P2：本地 HTTP range 代理 —— **只做一件事**：把一个 [ByteSource] 变成
 * mpv 能读的 HTTP URL。
 *
 * ## 为什么需要它
 * mpv（也就是 SenPlayer / Rex 用的那颗内核）通过 FFmpeg 的 `http` 协议读网络资源。
 * 我们自己的 SMB 读层（块窗口 / 预读 / 断网恢复 / 读画像）比 FFmpeg 的 libsmbclient
 * 强得多，但**协议层没法直接塞进 mpv** —— FFmpeg 的协议表是编译期静态的。
 *
 * 所以走这条中间路：
 * ```
 * mpv ──http://127.0.0.1:<port>/<id>──▶ 本代理 ──readAt()──▶ SmbByteSource ──▶ NAS
 * ```
 * 好处：我们的 SMB 层**一点不改**地继续生效（块窗口、预读、断网恢复全在）。
 *
 * ## 协议实现范围
 * 只实现 mpv/FFmpeg 真正会用的部分：
 *  · `HEAD` → 返回 `Content-Length`（FFmpeg 用它拿文件大小）
 *  · `GET` + `Range: bytes=N-` / `bytes=N-M` → 返回 206 + 那一段
 *  · 不支持 Range → 返回 200 + 全量（FFmpeg 偶尔会这么问）
 *  · 其它方法 → 405
 *
 * ## 为什么只监听回环
 * 只绑 `127.0.0.1`，不暴露到局域网 —— 免得变成一个可被外部访问的文件服务。
 *
 * ## 线程模型
 * 每个连接一个线程（mpv 的 demux 线程会串行发请求，连接数很小）。
 * 读取时对 [ByteSource] 加锁 —— 它明确声明"不保证线程安全"。
 */
class LocalRangeServer private constructor(
    private val socket: ServerSocket,
    private val sources: MutableMap<String, ByteSource>,
) : AutoCloseable {

    /** 每个注册的源给一个短 id，URL 里用它，避免把路径/凭据暴露在 URL 里。 */
    private val idSeq = AtomicLong(1)
    private val closed = AtomicLong(0)

    @Volatile
    private var running = true

    private val acceptor = Thread({ acceptLoop() }, "kivo-range-acceptor").apply {
        isDaemon = true
        start()
    }

    /**
     * 代理的基址，例如 `http://127.0.0.1:52341`。
     *
     * 【为什么固定 IPv4 而不是 getLoopbackAddress()】
     * `getLoopbackAddress()` 在这台设备上返回 `::1`（IPv6），拼出来的 URL 是
     * `http://::1:38205/1` —— **IPv6 字面量必须加方括号**（`[::1]:38205`）才合法，
     * 而 FFmpeg 的 http 协议不会替我们加括号，于是直接
     * `Failed to open http://::1:38205/1`（真机实测）。
     * 用 `127.0.0.1` 就没有这个坑：它是 IPv4 字面量，任何解析器都认。
     * 服务器仍然**只绑回环**（不暴露到局域网），只是用 IPv4 表达回环。
     */
    val baseUrl: String = "http://127.0.0.1:${socket.localPort}"

    /**
     * 注册一个源，返回它在 URL 里的 id。
     *
     * @param name 只用于诊断（不出现在 URL 里）。
     */
    fun register(source: ByteSource, name: String = ""): String {
        val id = idSeq.getAndIncrement().toString()
        synchronized(sources) { sources[id] = source }
        return id
    }

    /** 拼出 mpv 可以直接用的 URL。 */
    fun urlFor(id: String): String = "$baseUrl/$id"

    /** 注销并关闭一个源。 */
    fun release(id: String) {
        val source = synchronized(sources) { sources.remove(id) }
        source?.close()
    }

    private fun acceptLoop() {
        while (running && !socket.isClosed) {
            val client = try {
                socket.accept()
            } catch (e: IOException) {
                if (running) continue else break
            }
            Thread({ serve(client) }, "kivo-range-conn").apply { isDaemon = true }.start()
        }
    }

    private fun serve(client: Socket) {
        client.use { conn ->
            try {
                conn.soTimeout = 30_000
                val input = conn.getInputStream().bufferedReader()
                val output = BufferedOutputStream(conn.getOutputStream())
                val requestLine = input.readLine() ?: return
                val parts = requestLine.split(' ')
                if (parts.size < 2) return
                val method = parts[0].uppercase()
                val path = parts[1]

                // 读完请求头（FFmpeg 会带 Range / User-Agent 等）。
                var rangeHeader: String? = null
                while (true) {
                    val line = input.readLine() ?: break
                    if (line.isEmpty()) break
                    val lower = line.lowercase()
                    if (lower.startsWith("range:")) {
                        rangeHeader = line.substringAfter(':').trim()
                    }
                }

                val id = path.trim('/').substringBefore('?')
                val source = synchronized(sources) { sources[id] }
                if (source == null) {
                    writeSimple(output, 404, "Not Found")
                    return
                }

                when (method) {
                    "HEAD" -> respondHead(output, source)
                    "GET" -> respondGet(output, input, source, rangeHeader)
                    else -> writeSimple(output, 405, "Method Not Allowed")
                }
            } catch (e: IOException) {
                // 客户端断开是正常现象（mpv 换源/seek 时会断开旧连接）。
            }
        }
    }

    private fun respondHead(out: BufferedOutputStream, source: ByteSource) {
        val size = source.size()
        out.write(
            (
                "HTTP/1.1 200 OK\r\n" +
                    "Accept-Ranges: bytes\r\n" +
                    "Content-Length: $size\r\n" +
                    "Content-Type: video/x-matroska\r\n" +
                    "Connection: close\r\n\r\n"
                ).toByteArray()
        )
        out.flush()
    }

    private fun respondGet(
        out: BufferedOutputStream,
        input: BufferedReader,
        source: ByteSource,
        rangeHeader: String?,
    ) {
        val size = source.size()
        var start = 0L
        var end = size - 1
        var partial = false

        if (rangeHeader != null && rangeHeader.startsWith("bytes=")) {
            val spec = rangeHeader.removePrefix("bytes=").trim()
            val dash = spec.indexOf('-')
            if (dash > 0) {
                partial = true
                val from = spec.substring(0, dash).trim()
                start = if (from.isEmpty()) 0 else from.toLong()
                val to = spec.substring(dash + 1).trim()
                if (to.isNotEmpty()) {
                    end = to.toLong().coerceAtMost(size - 1)
                }
            }
        }
        if (start >= size || start > end) {
            writeSimple(out, 416, "Range Not Satisfiable")
            return
        }

        val length = end - start + 1
        val header = StringBuilder()
        header.append("HTTP/1.1 ").append(if (partial) "206 Partial Content" else "200 OK").append("\r\n")
        header.append("Accept-Ranges: bytes\r\n")
        header.append("Content-Length: ").append(length).append("\r\n")
        if (partial) {
            header.append("Content-Range: bytes ").append(start).append('-').append(end).append('/').append(size).append("\r\n")
        }
        header.append("Content-Type: video/x-matroska\r\n")
        header.append("Connection: close\r\n\r\n")
        out.write(header.toString().toByteArray())
        out.flush()

        // 分块读，边读边写 —— 一次不把整个区间塞进内存。
        val chunk = ByteArray(256 * 1024)
        var offset = start
        while (offset <= end) {
            val want = minOf(chunk.size.toLong(), end - offset + 1).toInt()
            val got = synchronized(source) { source.readAt(offset, chunk, 0, want) }
            if (got <= 0) break
            out.write(chunk, 0, got)
            out.flush()
            offset += got
        }
    }

    private fun writeSimple(out: BufferedOutputStream, code: Int, message: String) {
        val body = message.toByteArray()
        out.write(
            (
                "HTTP/1.1 $code $message\r\n" +
                    "Content-Length: ${body.size}\r\n" +
                    "Connection: close\r\n\r\n"
                ).toByteArray()
        )
        out.write(body)
        out.flush()
    }

    override fun close() {
        if (closed.getAndIncrement() != 0L) return
        running = false
        try {
            socket.close()
        } catch (e: IOException) {
            // 忽略：关闭时的异常不影响正确性。
        }
        acceptor.interrupt()
        val all = synchronized(sources) {
            val copy = sources.values.toList()
            sources.clear()
            copy
        }
        all.forEach { runCatching { it.close() } }
    }

    companion object {
        /**
         * 起一个代理。
         *
         * @param backlog 监听队列长度。
         */
        fun start(backlog: Int = 8): LocalRangeServer {
            // 只绑 IPv4 回环：见 baseUrl 的说明（IPv6 字面量在 URL 里要加方括号，
            // FFmpeg 不会替我们加）。
            val socket = ServerSocket(0, backlog, InetAddress.getByName("127.0.0.1"))
            socket.reuseAddress = true
            return LocalRangeServer(socket, mutableMapOf())
        }
    }
}
