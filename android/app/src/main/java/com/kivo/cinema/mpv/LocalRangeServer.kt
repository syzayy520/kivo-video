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

    /**
     * 拼出 mpv 可以直接用的 URL。
     *
     * 【为什么带这一串 query】
     * FFmpeg 的 http 协议把 URL 的 query 参数直接喂给 `av_opt_set_dict`，
     * 所以这里可以就地打开它的重连能力：
     *  · `reconnect=1` —— 连接在 EOF 前断开时自动重连；
     *  · `reconnect_at_eof=1` —— 读到一半流断掉时也重连（断网最常见的情形）；
     *  · `reconnect_streamed=1` —— 允许对流式源重连；
     *  · `reconnect_on_network_error=1` —— TCP 层错误（不是 HTTP 状态码）也重连；
     *  · `reconnect_on_http_error=503` —— 收到 503 时重连。
     *    代理在 SMB 读失败时返回的就是 503（见 respondGet 的说明）；
     *  · `reconnect_delay_max=5` —— 退避最多 5 秒；
     *  · `rw_timeout=150` —— 单次读超时 150 秒。
     *    【为什么这么大】代理在断网时会**挂着等**（见 readWithRetry），
     *    这段时间内核这边一个字节都收不到 —— 超时太短内核就放弃了。
     *    150 秒与代理的 READ_RETRY_WINDOW_MS(120s) 对齐并留了余量。
     *
     * 没有这些，mpv 在断网后**不会自己接上**（真机实测：网络 29 秒恢复，
     * 位置一直停在 34.66 秒不动）。
     */
    fun urlFor(id: String): String =
        "$baseUrl/$id?reconnect=1&reconnect_at_eof=1&reconnect_streamed=1" +
            "&reconnect_on_network_error=1&reconnect_on_http_error=503" +
            "&reconnect_delay_max=5&rw_timeout=150"

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

        // 【为什么先读第一块再发响应头】
        // 断网时 SMB 读会失败。如果先发 `200 OK` + Content-Length 再去读，
        // 中途失败就只能**直接断连** —— FFmpeg 看到的是"网络错误"，
        // 而它的 http 协议默认**不会**在网络错误后重连（reconnect_on_network_error=0）。
        //
        // 先读第一块再发头：读失败 ⇒ 返回 **503 Service Unavailable**，
        // FFmpeg 收到明确的 HTTP 状态码，只要 URL 上带
        // `reconnect_on_http_error=503` 就会按退避重试。
        // 这条路径比"断连重连"可靠得多（断连在 TCP 层，FFmpeg 分不清是
        // "服务端挂了"还是"网络抖了"）。
        val chunk = ByteArray(256 * 1024)
        val firstWant = minOf(chunk.size.toLong(), length).toInt()
        val firstGot = readWithRetry(source, start, chunk, 0, firstWant)
        if (firstGot <= 0) {
            // 读不出来且已过重试窗口：按"暂时不可用"处理，让内核按退避重试整个请求。
            writeSimple(out, 503, "Service Unavailable")
            return
        }

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
        out.write(chunk, 0, firstGot)
        out.flush()
        var offset = start + firstGot
        while (offset <= end) {
            val want = minOf(chunk.size.toLong(), end - offset + 1).toInt()
            val got = readWithRetry(source, offset, chunk, 0, want)
            if (got <= 0) break
            out.write(chunk, 0, got)
            out.flush()
            offset += got
        }
    }

    /**
     * 读一段，失败时**持续重试**直到成功或超过重试窗口。
     *
     * 【为什么必须在这里重试，而不是把失败直接告诉内核】
     * `SmbByteSource.readAt` 在离线时**抛异常**（一次性的）——
     * 它的退避与"等网络回来"的逻辑在 [com.kivo.cinema.smb.SmbRetryGate] 里，
     * 由**调用方循环调用**才能生效。旧内核正是这么做的（fd 回调线程持续重试），
     * 所以它的断网恢复只要 3.4 秒。
     *
     * 如果代理试一次就返回 503，FFmpeg 那边看到的是"这个文件读完了"或
     * "服务端不可用"，demuxer 会直接判定 EOF 并放弃 —— 真机实测：
     *   `[30.572] HTTP error 503` → `[30.572] mkv: EOF reached.` → 位置永远停住。
     *
     * 这里循环重试 = 复刻旧内核的语义：连接一直挂着，等网络回来后接着供数，
     * 播放位置**不停**。只有超过窗口（默认 120 秒）才放弃并返回 503，
     * 让内核有机会走它自己的重连逻辑。
     *
     * @return 读到的字节数；0 表示"到文件尾"或"重试窗口内没成功"。
     */
    private fun readWithRetry(
        source: ByteSource,
        offset: Long,
        buf: ByteArray,
        off: Int,
        len: Int,
    ): Int {
        val deadline = System.currentTimeMillis() + READ_RETRY_WINDOW_MS
        var attempt = 0
        while (true) {
            try {
                val got = synchronized(source) { source.readAt(offset, buf, off, len) }
                if (got > 0) return got
                // 读到文件尾（0 或 -1）是**正常结束**，不该重试。
                return 0
            } catch (e: Exception) {
                attempt += 1
                if (System.currentTimeMillis() >= deadline || closed.get() != 0L) {
                    return 0
                }
                // 退避由 SMB 层自己管（gate 的指数退避），这里只做短睡眠避免空转。
                try {
                    Thread.sleep(200)
                } catch (interrupted: InterruptedException) {
                    Thread.currentThread().interrupt()
                    return 0
                }
            }
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
         * 读失败后的重试窗口（毫秒）。
         *
         * 为什么是 120 秒：断网时内核必须**等**网络回来才能继续。
         * 窗口太短 → 代理放弃并回 503 → 内核判定 EOF → 位置永久停住；
         * 窗口够长 → 网络一恢复就接着供数，播放位置**全程不停**。
         * 旧内核的断网恢复是 3.4 秒（fd 线程持续重试），本窗口是它的兜底上限。
         */
        const val READ_RETRY_WINDOW_MS = 120_000L

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
