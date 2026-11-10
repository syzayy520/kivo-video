package com.kivo.cinema.net

import java.io.BufferedOutputStream
import java.io.BufferedReader
import java.io.InputStreamReader
import java.net.InetAddress
import java.net.ServerSocket
import java.net.Socket
import java.util.concurrent.atomic.AtomicBoolean

/**
 * 本地 HTTP/1.1 Range 代理（220 设计稿 D2）——SenPlayer 架构对位
 * （实证：libMLibKit.so 内嵌 HTTP/1.1 + Content-Range + Accept-Ranges）。
 *
 * 职责：把 [ByteSource]（随机读字节源）暴露成 http://127.0.0.1:port/<token>
 * 只服务本机回环；支持 GET/HEAD + Range: bytes=x-y（含开放区间 x- / -y）+
 * Content-Range/Accept-Ranges/206/416。FFmpeg 与 MediaExtractor 都是标准 HTTP 客户端。
 *
 * 243 起**只服务于 DLNA 投屏**：渲染端只认 HTTP URL，必须有一个真 HTTP 服务。
 * SMB / WebDAV 播放走 `net.RangeFd` 直接出 fd，不再经过这里。
 *
 * 线程模型：accept 循环一线程 + 每连接一线程（播放器通常 1-2 个并发请求）；
 * ByteSource 由调用方持有并保证串行（SMB 连接非线程安全）——代理内用锁串行化。
 * 生命周期：[start] 后 [close] 停止；连接处理循环见 [running]。
 */
class LocalRangeProxy(
    private val source: ByteSource,
    private val bindAddress: String = "127.0.0.1",   // 230 投屏：绑 0.0.0.0 给渲染器取流
) {
    private var server: ServerSocket? = null
    private val running = AtomicBoolean(false)
    private var acceptThread: Thread? = null
    private val lock = Any()   // ByteSource 串行化（SMB 连接非线程安全）

    /** 绑定端口（回环随机 / 指定地址随机）；返回端口号。 */
    fun start(): Int {
        val ss = ServerSocket(0, 8, InetAddress.getByName(bindAddress))
        server = ss
        running.set(true)
        acceptThread = Thread({
            while (running.get()) {
                try {
                    val sock = ss.accept()
                    Thread { handleConnection(sock) }.apply { isDaemon = true }.start()
                } catch (e: Exception) {
                    if (running.get()) continue
                }
            }
        }, "LocalRangeProxy-accept").apply { isDaemon = true; start() }
        return ss.localPort
    }

    fun close() {
        running.set(false)
        runCatching { server?.close() }
        runCatching { source.close() }
    }

    private fun handleConnection(sock: Socket) {
        try {
            sock.tcpNoDelay = true
            val reader = BufferedReader(InputStreamReader(sock.getInputStream()))
            val out = BufferedOutputStream(sock.getOutputStream(), 64 * 1024)
            var keepAlive = true
            while (keepAlive && running.get()) {
                val requestLine = reader.readLine() ?: break
                if (requestLine.isBlank()) continue
                // 读完请求头
                var contentLength = 0L
                var range: String? = null
                var method = ""
                while (true) {
                    val line = reader.readLine() ?: return
                    if (line.isEmpty()) break
                    val lower = line.lowercase()
                    if (lower.startsWith("range:")) range = line.substring(6).trim()
                    if (lower.startsWith("content-length:")) contentLength = line.substring(15).trim().toLong()
                }
                method = requestLine.substringBefore(" ").uppercase()
                if (contentLength > 0) {
                    // 有 body 的请求（本代理不该收到），丢弃
                    var n = contentLength
                    val skip = ByteArray(4096)
                    while (n > 0) {
                        val r = sock.getInputStream().read(skip, 0, minOf(skip.size.toLong(), n).toInt())
                        if (r < 0) return
                        n -= r
                    }
                }

                val fileSize = synchronized(lock) { source.size() }
                // 解析 Range: bytes=a-b / a- / -suffix
                var start = 0L
                var endIncl = fileSize - 1
                var isRange = false
                var badRange = false
                if (range != null && range.startsWith("bytes=")) {
                    isRange = true
                    val spec = range.substring(6).trim()
                    val dash = spec.indexOf('-')
                    if (dash < 0) {
                        badRange = true
                    } else if (dash == 0) {
                        // -suffix：最后 N 字节
                        val suffix = spec.substring(1).toLongOrNull() ?: 0L
                        if (suffix <= 0) badRange = true
                        else {
                            start = (fileSize - suffix).coerceAtLeast(0)
                            endIncl = fileSize - 1
                        }
                    } else {
                        start = spec.substring(0, dash).toLongOrNull() ?: -1L
                        val endStr = spec.substring(dash + 1)
                        if (start < 0) badRange = true
                        else if (endStr.isEmpty()) {
                            endIncl = fileSize - 1
                        } else {
                            endIncl = endStr.toLongOrNull() ?: -1L
                            if (endIncl < start) badRange = true
                        }
                    }
                    if (!badRange && (fileSize == 0L || start >= fileSize)) badRange = true
                    if (badRange) {
                        out.write("HTTP/1.1 416 Range Not Satisfiable\r\n".toByteArray())
                        out.write("Content-Range: bytes */$fileSize\r\n".toByteArray())
                        out.write("Content-Length: 0\r\n\r\n".toByteArray())
                        out.flush()
                        continue
                    }
                }

                val total = endIncl - start + 1
                val head = StringBuilder()
                head.append(if (isRange) "HTTP/1.1 206 Partial Content\r\n" else "HTTP/1.1 200 OK\r\n")
                head.append("Accept-Ranges: bytes\r\n")
                head.append("Content-Type: application/octet-stream\r\n")
                head.append("Content-Length: $total\r\n")
                if (isRange) head.append("Content-Range: bytes $start-$endIncl/$fileSize\r\n")
                head.append("Connection: keep-alive\r\n")
                head.append("\r\n")
                out.write(head.toString().toByteArray())

                if (method == "HEAD") {
                    out.flush()
                    continue
                }

                // 分块从 ByteSource 读出（32KB 粒度）
                val buf = ByteArray(32 * 1024)
                var pos = start
                var remaining = total
                while (remaining > 0 && running.get()) {
                    val want = minOf(buf.size.toLong(), remaining).toInt()
                    val n = synchronized(lock) { source.readAt(pos, buf, 0, want) }
                    if (n <= 0) break   // 源 EOF/错误——断开让客户端重试
                    out.write(buf, 0, n)
                    pos += n
                    remaining -= n
                }
                out.flush()
            }
        } catch (e: Exception) {
            // 客户端断开/超时——正常关闭路径，不吞协议错误（错误由 ByteSource 抛给上层）
        } finally {
            runCatching { sock.close() }
        }
    }
}
