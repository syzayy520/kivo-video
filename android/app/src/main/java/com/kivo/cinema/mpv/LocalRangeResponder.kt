package com.kivo.cinema.mpv

import com.kivo.cinema.net.ByteSource
import com.kivo.cinema.net.NetDiag
import com.kivo.cinema.net.RangeReaderSource
import java.io.BufferedOutputStream
import java.util.concurrent.atomic.AtomicLong

/**
 * 273 v12：本地 range 代理的**范围语义** —— 把一次 GET 变成一段供给。
 *
 * 【为什么从 [LocalRangeServer] 里拆出来】
 * 服务器骨架管的是"连接怎么活"（accept 循环 / 请求头解析 / 源登记表），
 * 这里管的是"这一段怎么给"（Range 怎么算 / 读不到怎么办 / 给多长）。
 * 两种变化原因：改连接策略不该动范围语义，反之亦然。
 *
 * 【单次响应长度上限（[maxRangeBytes]）】
 * 内核每次 seek 都会新开一条连接，而**一条连接读到片尾的开放区间
 * （`Range: bytes=OFF-`）会让内核一路往前拉**——真机实测（273 v12）：
 * 预览帧在 seek 后 1.1 秒就拿到了，内核却继续往前读了 **9 秒 / 约 20 MiB**。
 * mpv 侧的旋钮（`cache` / `cache-secs` / `demuxer-max-bytes` /
 * `demuxer-readahead-secs`）都按 0 与否各跑过一轮，**没有一个能拦住它**
 * （见设计稿 §六 L2）。所以上限只能在我们这一层设。
 *
 * 【上限怎么设才不误伤内核】
 * 上限只截**这一次响应给多长**，不改内容：截短后响应仍是合法 206
 * （Content-Range 写清真实终点），内核看到的是"这一段到头了"，不是"网络断了"。
 * 播放主链路的上限是 0（不限）——它要的就是连续流。
 *
 * 【273 v19：片头（偏移 0）也截】
 * 旧实现给偏移 0 开豁免，理由是"内核靠片头那一跳的 `Content-Length` 推文件大小" ——
 * **那个理由是错的**：FFmpeg `http_read_header` 末尾写着
 * `if (s->filesize_from_content_range != UINT64_MAX) s->filesize = s->filesize_from_content_range;`
 * —— 只要响应里有 `Content-Range: bytes 0-N/<total>`，总长就是 `<total>`，与 Content-Length 无关。
 * 片头恰恰最该截：FFmpeg 打开片源的第一跳就是 `Range: bytes=0-`（开口区间），
 * 内核读完容器头就 seek 走，代理却还在往一个没人读的 socket 里灌数据 ——
 * 真机实测（v18 长稳）：预览取 0 秒那一帧，从源站顺序拉了 **24 MiB**。
 */
internal class LocalRangeResponder(
    /** 单次响应最多给多少字节；0 = 不限（播放主链路）。 */
    private val maxRangeBytes: Long = 0L,
) {

    /** 代理已关闭：读重试要立刻放弃（否则连接线程会挂满整个重试窗口）。 */
    private val closed = AtomicLong(0)

    /** 服务器收工时调用：让正在重试的读立刻放弃。 */
    fun markClosed() {
        closed.incrementAndGet()
    }

    /** HEAD：内核用它拿片源总长。 */
    fun respondHead(out: BufferedOutputStream, size: Long) {
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

    /** GET：按 [rangeHeader] 给一段；读不到就 503（让内核按退避重试）。 */
    fun respondGet(
        out: BufferedOutputStream,
        source: ByteSource,
        rangeHeader: String?,
    ) {
        val spec = MpvRangeSpec.parse(rangeHeader)

        // 【请求头先留痕】响应结束才写的那行日志**看不到片头那一跳**（开口区间，
        // 代理会一直灌到内核把连接关掉）⇒ "预览下了一整片"在证据里完全消失。
        NetDiag.log("PROXY REQ range=${rangeHeader ?: "-"} cap=$maxRangeBytes")

        // 【按连接派发读者】
        // 内核每次 seek 都会新开一条连接，新旧连接会短暂重叠。
        // 能按连接派发的源（[RangeReaderSource]）必须给这条连接一套**自己的**
        // 连接与缓冲，否则两个读者会互相把对方顶掉。
        //
        // 【先开读者、再问总长 —— 省掉一次往返】
        // 源站请求在"开读者"这一刻就发出去了，而**这一跳的响应头里就带着总长**
        // （Content-Range 的 "/total"）⇒ 问读者要总长**不额外往返**。
        // 旧实现是先 `source.size()` 单独探一次（1 字节范围请求，一次 RTT
        // ≈300 ms），起播因此白等一整个往返（真机：内核 t=0 之前那 300 ms）。
        val reader: ByteSource = if (source is RangeReaderSource) source.openReader(spec.start) else source
        try {
            val size = try {
                reader.size()
            } catch (e: Exception) {
                // 源站的判决（401/403/404/起点越界）或网络故障都在这里冒头。
                NetDiag.log("PROXY size fail: ${e.javaClass.simpleName}: ${e.message}")
                writeSimple(out, 503, "Service Unavailable")
                return
            }
            if (size <= 0) {
                NetDiag.log("PROXY size unknown")
                writeSimple(out, 503, "Service Unavailable")
                return
            }
            var end = spec.endInclusive ?: (size - 1)
            if (spec.start >= size || spec.start > end) {
                writeSimple(out, 416, "Range Not Satisfiable")
                return
            }
            end = end.coerceAtMost(size - 1)
            end = capEnd(spec.start, end)

            val length = end - spec.start + 1
            // 【截了就要用 206 把总长交给内核】"Content-Range" 的 "/total" 才是内核认的
            // 总长来源；只要这一路有上限，就必须回 206 + Content-Range，哪怕内核没带 Range。
            val partial = spec.partial || maxRangeBytes > 0L
            serveRange(out, reader, size, spec.start, end, length, partial, rangeHeader)
        } finally {
            if (reader !== source) runCatching { reader.close() }
        }
    }

    /**
     * 【上限】这一段最多给到哪个偏移。
     *
     * 【为什么每一段都要截】内核打开片源的第一跳就是开口区间（`bytes=0-`），
     * 读完容器头就 seek 走，代理却还在往没人读的 socket 里灌 —— 真机实测
     * （v12/v18）：预览取 0 秒与 475 秒那两帧，分别从源站顺序拉了 **24 MiB**
     * 与 **9 秒 / 约 24 MiB**。mpv 侧的旋钮各跑过一轮都拦不住它 ⇒ 只能在供给侧截。
     * 截短后响应仍是合法 206（Content-Range 写清真实终点），内核看到的是"到头了"。
     */
    private fun capEnd(start: Long, end: Long): Long {
        if (maxRangeBytes <= 0) return end
        val span = end - start + 1
        if (span <= maxRangeBytes) return end
        return start + maxRangeBytes - 1
    }

    /** 把 [start]–[end] 这一段从 [reader] 读出来写给内核。 */
    private fun serveRange(
        out: BufferedOutputStream,
        reader: ByteSource,
        size: Long,
        start: Long,
        end: Long,
        length: Long,
        partial: Boolean,
        rangeHeader: String?,
    ) {
        // 【为什么先读第一块再发响应头】
        // 断网时读会失败。如果先发 `200 OK` + Content-Length 再去读，
        // 中途失败就只能**直接断连** —— FFmpeg 看到的是"网络错误"，
        // 而它的 http 协议默认**不会**在网络错误后重连（reconnect_on_network_error=0）。
        //
        // 先读第一块再发头：读失败 ⇒ 返回 **503 Service Unavailable**，
        // FFmpeg 收到明确的 HTTP 状态码，只要 URL 上带
        // `reconnect_on_http_error=503` 就会按退避重试。
        // 这条路径比"断连重连"可靠得多（断连在 TCP 层，FFmpeg 分不清是
        // "服务端挂了"还是"网络抖了"）。
        // 【为什么首块 64 KiB】响应头要等第一块读完才发（503 重连语义）。
        // 首块越小，内核拿到首字节越快 —— 真机实测 256 KiB 时起播多花数秒。
        val chunk = ByteArray(CHUNK_BYTES)
        val firstWant = minOf(chunk.size.toLong(), length).toInt()
        val firstGot = readWithRetry(reader, start, chunk, 0, firstWant)
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
        // 【为什么后续用 256 KiB】首块要小（快出首字节），后续块要大（少几次系统调用）。
        val bulk = ByteArray(BULK_CHUNK_BYTES)
        val started = System.currentTimeMillis()
        var served = firstGot
        out.write(chunk, 0, firstGot)
        out.flush()
        var offset = start + firstGot
        while (offset <= end) {
            val want = minOf(bulk.size.toLong(), end - offset + 1).toInt()
            val got = readWithRetry(reader, offset, bulk, 0, want)
            if (got <= 0) break
            out.write(bulk, 0, got)
            out.flush()
            served += got
            offset += got
        }
        // 【诊断】每个请求的区间 / 实际供出字节 / 上限 / 耗时 ——
        // 判断"这一跳拉了多少"与"上限有没有生效"的唯一依据。
        NetDiag.log(
            "GET range=$rangeHeader bytes=$served cap=$maxRangeBytes ms=${System.currentTimeMillis() - started}",
        )
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
                if (attempt == 1 || attempt % 10 == 0) {
                    NetDiag.log("PROXY read fail @$offset attempt=$attempt: ${e.message}")
                }
                if (System.currentTimeMillis() >= deadline || closed.get() != 0L) {
                    NetDiag.log("PROXY read give up @$offset after $attempt attempts")
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

    /** 一个不带正文的错误响应（404 / 405 / 416 / 503）。 */
    fun respondSimple(out: BufferedOutputStream, code: Int, message: String) {
        writeSimple(out, code, message)
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

        /** 首块大小：要小 —— 响应头要等它读完才发（503 重连语义），越小起播越快。 */
        private const val CHUNK_BYTES = 64 * 1024

        /** 后续块大小：要大 —— 少几次读+写系统调用，吞吐更稳。 */
        private const val BULK_CHUNK_BYTES = 256 * 1024
    }
}
