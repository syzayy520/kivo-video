package com.kivo.cinema.mpv

import com.kivo.cinema.net.ByteSource
import com.kivo.cinema.net.NetDiag
import java.io.BufferedOutputStream
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
 * mpv 内核通过 FFmpeg 的 `http` 协议读网络资源。
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
    /** 范围语义（给多长 / 怎么算 Range）：见 [LocalRangeResponder]。 */
    private val responder: LocalRangeResponder,
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
     *    代理在 SMB 读失败时返回的就是 503（见 [LocalRangeResponder] 的说明）；
     *  · `reconnect_delay_max=5` —— 退避最多 5 秒；
     *  · `rw_timeout=150` —— 单次读超时 150 秒。
     *    【为什么这么大】代理在断网时会**挂着等**（见 readWithRetry），
     *    这段时间内核这边一个字节都收不到 —— 超时太短内核就放弃了。
     *    150 秒与代理的 READ_RETRY_WINDOW_MS(120s) 对齐并留了余量。
     *
     * 没有这些，mpv 在断网后**不会自己接上**（真机实测：网络 29 秒恢复，
     * 位置一直停在 34.66 秒不动）。
     *
     * 【为什么预览要 [reconnect] = false】
     * `reconnect_at_eof=1` 的意思是"读到这一段的末尾就**再连一次接着往下读**"。
     * 预览内核的每一跳都被我们截了上限（见 [LocalRangeResponder]），
     * 一旦允许它在末尾重连，它就会**接着往下拉** —— 截上限就白设了。
     * 关掉之后：这一段到头 = 干净的文件尾；下一次 seek 由内核的
     * `http_seek` 重开连接（带新的 Range），照样能定位到新位置。
     */
    fun urlFor(id: String, reconnect: Boolean = true): String {
        val atEof = if (reconnect) 1 else 0
        return "$baseUrl/$id?reconnect=1&reconnect_at_eof=$atEof&reconnect_streamed=1" +
            "&reconnect_on_network_error=1&reconnect_on_http_error=503" +
            "&reconnect_delay_max=5&rw_timeout=150"
    }

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
                    responder.respondSimple(output, 404, "Not Found")
                    return
                }

                when (method) {
                    "HEAD" -> responder.respondHead(output, source.size())
                    "GET" -> responder.respondGet(output, source, rangeHeader)
                    else -> responder.respondSimple(output, 405, "Method Not Allowed")
                }
            } catch (e: Exception) {
                // 客户端断开是正常现象（mpv 换源/seek 时会断开旧连接）。
                // 这里捕 Exception 而不是 IOException：代理线程里任何一个异常
                // 都不许拖垮播放器进程（未捕获异常会让 Android 杀掉整个进程）。
                NetDiag.log("PROXY conn fail: ${e.javaClass.simpleName}: ${e.message}")
            }
        }
    }

    override fun close() {
        if (closed.getAndIncrement() != 0L) return
        running = false
        // 让正在重试的读立刻放弃（否则连接线程会挂满 120 秒的重试窗口）。
        responder.markClosed()
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
         * @param maxRangeBytes 单次响应上限（字节）；0 = 不限。
         *        播放主链路传 0（要的是连续流）；预览实例传 4 MiB ——
         *        见 [LocalRangeResponder] 的类注释（273 v12 的 32 MiB 浪费）。
         */
        fun start(backlog: Int = 8, maxRangeBytes: Long = 0L): LocalRangeServer {
            // 只绑 IPv4 回环：见 baseUrl 的说明（IPv6 字面量在 URL 里要加方括号，
            // FFmpeg 不会替我们加）。
            val socket = ServerSocket(0, backlog, InetAddress.getByName("127.0.0.1"))
            socket.reuseAddress = true
            return LocalRangeServer(socket, mutableMapOf(), LocalRangeResponder(maxRangeBytes))
        }
    }
}
