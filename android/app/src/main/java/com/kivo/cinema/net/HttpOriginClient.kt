package com.kivo.cinema.net

import java.io.IOException
import java.io.InputStream
import java.net.HttpURLConnection

/**
 * 273 v6：源站 HTTP 连接的**唯一**出入口。
 *
 * ## 它只做三件事
 *  1. 带上凭据（token 只进 `X-Emby-Token` 请求头，**永不进 URL、永不进日志**）；
 *  2. 打开一条**分块**的顺序读流（`Range: bytes=OFF-END`，见下）；
 *  3. 把源站的失败翻译成用户看得懂的话（401/403/404/416 → reason）。
 *
 * ## v6 为什么改成"分块范围"
 *
 * 【v5 的做法与它的代价】v5 一条连接读到片尾（`Range: bytes=OFF-`），
 * 于是这条响应**永远不会结束** ⇒ 连接永远回不了 keep-alive 池 ⇒
 * 下一次要数据（续读、跳读、读 cues）只能**新建连接**。
 * 真机实测（emby.bangumi.ca，Redmi K40）：
 *
 *   · 新建连接：表头 **754–968 ms**，4 MiB 总耗时 2.0–2.3 s（≈2 MB/s）；
 *   · 复用连接：表头 **~300 ms**，4 MiB 总耗时 0.42 s（≈10 MB/s）；
 *   · 闲置 8 秒后复用：仍是 331 ms / 8.8 MB/s。
 *
 * "新建连接"既慢又窄，"复用连接"又快又宽 —— 差 5 倍。
 * v5 的"闲置即回收"（环满就关连接、3 秒后重开）正是每 3.5 秒
 * 付一次新建连接代价的来源。
 *
 * 【v6 的做法】每次请求一段**有界**范围（`bytes=OFF-END`，默认 8 MiB），
 * 读完这一段就 `close()` ⇒ 响应体读完 ⇒ 连接回 keep-alive 池 ⇒
 * 下一段请求复用同一条 TCP/TLS（~300 ms）。中途放弃（跳读/关闭）用
 * `disconnect()`，**不**把剩余字节读完 —— 读完会把整段余量白下下来。
 *
 * ## 池参数必须在第一次建连前设好
 * JDK 的连接池默认只有 5 条连接、闲置 5 秒就回收。我们同时有主播放、
 * 拖动预览、每次 seek 的新连接 —— 5 条不够（超出的请求会排队等空闲连接，
 * 表现为莫名其妙的卡顿），闲置 5 秒也短（环满时连接闲置常常超过 5 秒）。
 */
class HttpOriginClient(
    private val url: String,
    private val token: String? = null,
    private val label: String = "main",
    /** 单次请求的范围长度（字节）。有界范围才能让连接回池复用。 */
    val chunkBytes: Long = DEFAULT_CHUNK_BYTES,
    /** 任何一次响应报出总长时叫一声（源据此记住总长，省掉后面再探一次）。 */
    private val onTotalKnown: ((Long) -> Unit)? = null,
) {

    /**
     * 让路闸（273 v29）：拖动期间让本 client 主动让出源站带宽。
     *
     * 【为什么挂在 client 上】预读线程与保温线程共用同一个 client，
     * 而它们正是拖动时抢带宽的那两路 ⇒ 闸门一开，两路一起让路。
     */
    internal val yieldGate = OriginYieldGate()

    /**
     * 落点窗口是不是正在下（273 v31）。
     *
     * 【为什么挂在这个 client 上】预读线程与保温线程共用同一个 client，
     * 而它们正是拖动时抢带宽的那两路 ⇒ 落点窗口在飞时，
     * 本 client 的预读线程一律让路（见 [OriginYieldPolicy]）。
     */
    internal var landingInFlight: (() -> Boolean)? = null

    /**
     * 一条已打开的**分块**范围流。
     *
     * 【三种收尾的区别】
     *  · [finish]：本块读完 ⇒ 关流 ⇒ **连接回池**（下次复用，约 300 ms）；
     *  · [abort]：中途放弃 ⇒ 断开连接（不回池，也**不**把余量读干净）；
     *  · [close]：等价于 [abort]（"关掉这条流"在语义上就是不要它的数据了）。
     */
    class OriginStream(
        private val conn: HttpURLConnection,
        val input: InputStream,
        /** 本块实际起点（服务器可能把请求规范化过，取 Content-Range 为准）。 */
        val start: Long,
        /** 本块实际结束（不含）。 */
        val endExclusive: Long,
        /** 片源总长（能取到时）。 */
        val total: Long?,
    ) : AutoCloseable {
        /** 本块已读字节数。 */
        var consumed: Long = 0L
            private set

        /** 本块是否已经读完（读满 [endExclusive] - [start] 字节）。 */
        val exhausted: Boolean get() = consumed >= endExclusive - start

        /** 读一块；返回 0 表示流结束（块读完，或源站把连接掐了 —— 看 [exhausted] 区分）。 */
        fun readInto(dst: ByteArray, off: Int, len: Int): Int {
            val n = input.read(dst, off, len)
            if (n > 0) consumed += n
            return n
        }

        /** 本块读完：关流 ⇒ 连接回 keep-alive 池。 */
        fun finish() {
            runCatching { input.close() }
        }

        /** 中途放弃：直接断开（不回池，也**不**把剩余字节读干净）。 */
        fun abort() {
            runCatching { conn.disconnect() }
        }

        override fun close() {
            abort()
        }
    }

    /**
     * 取片源总长。
     *
     * 用一次 1 字节的范围请求（`bytes=0-0`）而不是 HEAD ——
     * 源站对 HEAD 的支持不一致，而范围请求是播放本来就要用的形态。
     *
     * 【为什么读完那 1 字节再关】响应体读完 ⇒ 连接回池 ⇒
     * 紧接着的第一个分块请求就不用重新握手（省 400–700 ms）。
     */
    fun totalSize(): Long {
        val started = System.currentTimeMillis()
        val conn = open(0L, 0L)
        var body: InputStream? = null
        try {
            val code = conn.responseCode
            checkCode(code)
            val total = OriginHeadParser.total(
                OriginHeadParser.contentRange(conn),
                OriginHeadParser.contentLength(conn),
                code,
            ) ?: throw IOException("服务器未返回可用的片源长度")
            total.let { onTotalKnown?.invoke(it) }
            body = conn.inputStream
            body.read()
            NetDiag.log(
                "[$label] origin probe total=$total hdrMs=${System.currentTimeMillis() - started} " +
                    "thread=${Thread.currentThread().name}",
            )
            return total
        } finally {
            if (body != null) {
                runCatching { body.close() }
            } else {
                runCatching { conn.disconnect() }
            }
        }
    }

    /**
     * **预热一条** keep-alive 连接：发一个 1 字节范围请求并把它读完。
     *
     * 【为什么需要它】读满响应体 ⇒ 连接**回池** ⇒ 下一次请求复用它。
     * 真机实测（v7，emby.bangumi.ca）：
     * · 复用连接的表头延迟 **270–360 ms**；
     * · 新建连接的表头延迟 **983–1301 ms**（TLS 握手 ≈700–1000 ms）。
     * 内核在起播阶段一定会跳读两次（读文件尾的 cues、跳到断点续播位置），
     * 而那一刻**所有在用的连接都在传数据** ⇒ 跳读只能新建连接 ⇒ 每次多付 ~800 ms。
     * 预热就是提前把两条空连接放进池里，让跳读有连接可借。
     *
     * 【失败不影响播放】预热失败只少一次复用，主路径照旧。
     */
    fun warmConnection(): Boolean {
        val started = System.currentTimeMillis()
        return runCatching {
            val conn = open(0L, 0L)
            val code = conn.responseCode
            checkCode(code)
            conn.inputStream.use { it.read() }
            NetDiag.log(
                "[$label] origin warmup hdrMs=${System.currentTimeMillis() - started} " +
                    "thread=${Thread.currentThread().name}",
            )
            true
        }.getOrDefault(false)
    }

    /**
     * 打开从 [start] 开始、长度 [chunkBytes] 的一块顺序读流。
     *
     * @throws IOException 源站不可达/状态码不对/忽略了范围请求。
     */
    fun openChunk(start: Long): OriginStream = openRange(start, start + chunkBytes - 1)

    /**
     * 打开一段**精确长度**的范围流（`Range: bytes=start-endInclusive`）。
     *
     * 【为什么要有精确长度】片尾预取要的是**文件末尾那一段**，
     * 不是"从这里开始读 chunkBytes" —— 后者会被源站夹到 EOF，
     * 拿到的长度就不是自己要的（片尾预取要的是最后 1 MiB，不是最后 4 MiB）。
     */
    fun openRange(start: Long, endInclusive: Long): OriginStream {
        val started = System.currentTimeMillis()
        val requestedEnd = endInclusive
        val conn = open(start, requestedEnd)
        val code = conn.responseCode
        val hdrMs = System.currentTimeMillis() - started
        if (code == 416) {
            conn.disconnect()
            throw IOException("起点越过片源末尾")
        }
        checkCode(code)

        val range = OriginHeadParser.contentRange(conn)
        var actualStart = start
        var actualEnd = requestedEnd
        if (code == 206) {
            OriginHeadParser.rangeStart(range)?.let { actualStart = it }
            OriginHeadParser.rangeEnd(range)?.let { actualEnd = it }
        } else if (start > 0) {
            // 200 = 服务器把整个文件从头给了；这时"从 start 读"是假的，
            // 继续用就会把文件头当成文件中间 ⇒ 宁可报错也不能给错字节。
            conn.disconnect()
            throw IOException("服务器忽略了范围请求，无法定位到指定位置")
        }
        val total = OriginHeadParser.total(
            range,
            OriginHeadParser.contentLength(conn),
            code,
        )
        // 【总长顺手报给源】这一跳的响应头里就有总长（`Content-Range` 的 "/total"），
        // 源记住它 ⇒ 后面不用再单独探一次（真机：省一整个 RTT ≈300 ms）。
        total?.let { onTotalKnown?.invoke(it) }
        if (code == 200 && total != null) {
            actualEnd = total - 1
        }
        // 长度写进日志：证据复算要按**实际**块长统计字节，
        // 不能假设"每块都是 chunkBytes"——源站会在文件尾把块夹短（cues 块只有 64 KB）。
        NetDiag.log(
            "[$label] origin chunk from=$actualStart code=$code hdrMs=$hdrMs " +
                "len=${actualEnd - actualStart + 1} thread=${Thread.currentThread().name}",
        )

        val input = try {
            conn.inputStream
        } catch (e: IOException) {
            conn.disconnect()
            throw IOException("读取片源失败（${e.message}）")
        }
        return OriginStream(conn, input, actualStart, actualEnd + 1, total)
    }

    /** 打开一条连接；[endInclusive] 为 null 表示开放区间（读到文件尾）。 */
    private fun open(start: Long, endInclusive: Long?): HttpURLConnection {
        return OriginHttpConnection.open(url, token, start, endInclusive)
    }

    /** 源站的状态码 → 用户可见的话（不许静默降级，G9）。 */
    private fun checkCode(code: Int) = OriginHttpConnection.checkCode(code)

    companion object {
        /**
         * 单块范围的长度（字节）。
         *
         * 【为什么是 4 MiB】块必须和环配套：**块比环大，生产线程永远停在
         * carry 态**（没有跑道），而且每次跳读要白下"环 + 块"那么多字节
         * （真机 273：环 16 MiB + 块 8 MiB ⇒ 起播那次跳读白下 24 MiB）。
         *
         * 【跑道够不够】环 8 MiB + carry 4 MiB = 12 MiB，
         * 4K 片源需求 435 KB/s ⇒ **27 秒**的缓冲，够盖住源站 0.35 s 的表头
         * 与偶发抖动。
         *
         * 【表头开销】真机实测每块表头 317–1219 ms（中位 362 ms）：
         * 4 MiB 块的表头占比 ≈ 0.36 / (0.36 + 0.4) ≈ 47%，
         * 折算有效取数 5.3 MB/s —— 仍是需求（0.435 MB/s）的 **12 倍**。
         */
        const val DEFAULT_CHUNK_BYTES = 4L * 1024 * 1024
    }
}
