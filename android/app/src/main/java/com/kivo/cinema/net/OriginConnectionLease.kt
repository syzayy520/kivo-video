package com.kivo.cinema.net

import java.io.IOException

/**
 * 273 v6：**一块源站数据**的生命周期 —— 开、读、读满归还、失败重开。
 *
 * ## 它为什么独立成一个文件
 * "连接怎么活下来"（开哪一块、什么时候把响应读完、失败了怎么重开）是**连接策略**，
 * "什么时候该读、该写缓冲、该跳读"是**生产循环**。两种变化原因：
 * 改策略不该动循环，改循环不该动策略。
 *
 * ## v6 的核心：有界范围 + 读满即归还
 *
 * 【v5 的做法与它的代价】v5 一条连接读到片尾（`Range: bytes=OFF-`），
 * 响应**永远不会结束** ⇒ 连接永远回不了 keep-alive 池 ⇒ 下一次取数只能新建连接。
 * 真机实测（emby.bangumi.ca，Redmi K40）：
 *
 *   · 新建连接：表头 **754–968 ms**，4 MiB 总耗时 2.0–2.3 s（≈2 MB/s）；
 *   · 复用连接：表头 **~300 ms**，4 MiB 总耗时 0.42 s（≈10 MB/s）；
 *   · 闲置 8 秒后复用：仍是 331 ms / 8.8 MB/s。
 *
 * 差 5 倍。v5 的"闲置即回收"（环满就关连接、3 秒后重开）正是每 3.5 秒
 * 付一次新建连接代价的来源 —— 真机 207 秒里 166 秒 mpv 缓冲的根因。
 *
 * 【v6 的做法】每次请求一段**有界**范围（默认 8 MiB），读满即 `finish()`
 * ⇒ 响应体读完 ⇒ 连接回 keep-alive 池 ⇒ 下一块复用同一条 TCP/TLS（~300 ms）。
 *
 * 【为什么"必须读满"这件事很关键】半途停下的响应会把连接晾在池外，
 * 下次续读时那条 TCP 可能已经退化成慢读（v5 实测 60–250 KB/s）。
 * 所以生产侧宁可多花一块内存（块的尾巴先放进 carry），也要让**每一块都读完**。
 *
 * ## 失败语义
 * 源站掐掉连接、读失败、块中途 EOF —— 都不是故障：自己重开接着读
 * （**保留缓冲**），只有连续失败超限才把原因交给上层（G9：不静默降级）。
 *
 * 【断连后谁来重开】本类只负责**把块丢掉**（[stream] 变 null）；
 * 生产循环看到没有块就会用**当前写入位置**重开一块 —— 那一段缓冲里的
 * 数据不动，所以不会丢字节。
 */
class OriginConnectionLease(
    private val client: HttpOriginClient,
    private val buffer: OriginPrefetchBuffer,
    private val label: String = "play",
) {
    private val lock = Object()

    /** 当前这一块（null = 没有块）。 */
    private var stream: HttpOriginClient.OriginStream? = null

    /** 每次换块 +1：读失败时用它判断"这次失败是不是我自己换流造成的"。 */
    private var generation = 0

    private var consecutiveFailures = 0

    /** 当前块（null = 没有）。 */
    fun current(): HttpOriginClient.OriginStream? = synchronized(lock) { stream }

    /** 当前块还剩多少字节没读（0 = 没有块或已读完）。 */
    fun remaining(): Long = synchronized(lock) {
        val active = stream ?: return@synchronized 0L
        active.endExclusive - active.start - active.consumed
    }

    /**
     * 打开一块：从 [target] 起，最多 client.chunkBytes 字节。
     *
     * @param reset true = 跳读（丢弃缓冲，游标归位）；false = 断连后重开（**保留缓冲**）。
     */
    fun open(target: Long, reset: Boolean) {
        var old: HttpOriginClient.OriginStream? = null
        synchronized(lock) {
            old = stream
            stream = null
            generation += 1
            if (reset) {
                buffer.resetTo(target)
            }
            lock.notifyAll()
        }
        runCatching { old?.abort() }

        // 【越界起点就地判掉，不去打源站】
        //
        // 源站对越界起点回 416 —— 那是一次**注定失败**的网络往返：
        // 真机实测（4K 片源，总长 854 260 052 字节）hdrMs=**1867 ms**
        // （新建连接 + 等表头 + 416），而总长在第一个分块响应里就知道了
        // （Content-Range 的 "/total"）。
        //
        // 触发场景很具体：读完 MKV 的 Cues 之后 —— 这一块被源站夹到文件末尾
        // （Cues 在 854 195 684，总长 854 260 052），生产循环按"环的写入位置"
        // 开下一块，那个位置正好 = 文件末尾 = 越界。
        // 不就地判掉 ⇒ 起播白白多等近 2 秒。
        val total = buffer.knownTotal
        if (total > 0 && target >= total) {
            buffer.markEndOfFile()
            NetDiag.log("[$label] origin 到达片源末尾 @$target（总长 $total），不再向源站请求")
            return
        }

        try {
            val opened = client.openChunk(target)
            synchronized(lock) {
                stream = opened
                opened.total?.let { buffer.setTotal(it) }
                consecutiveFailures = 0
                lock.notifyAll()
            }
        } catch (e: Exception) {
            // 建连失败（源站抖动 / 网络断）⇒ 先自己重试若干次，再把原因交给上层。
            synchronized(lock) {
                consecutiveFailures += 1
                if (consecutiveFailures > MAX_CONSECUTIVE_FAILURES) {
                    buffer.markFailure(e)
                }
                lock.notifyAll()
            }
            NetDiag.log("[$label] origin open fail @$target: ${e.message}")
            sleepQuietly(REOPEN_BACKOFF_MS)
        }
    }

    /**
     * 从当前块读，最多 [len] 字节到 [dst] 的 [off] 处。
     *
     * @return > 0 = 读到的字节数；0 = 没有块或这一块已经读完
     *         （读完时连接已归还 keep-alive 池，可以开下一块）；
     *         -1 = 失败（连接已被丢掉，生产循环会用当前写入位置重开一块）。
     */
    fun read(dst: ByteArray, off: Int, len: Int): Int {
        val active = synchronized(lock) { stream } ?: return 0
        val myGeneration = synchronized(lock) { generation }
        val rest = active.endExclusive - active.start - active.consumed
        if (rest <= 0L) {
            finishChunk(active, myGeneration)
            return 0
        }
        val want = minOf(len.toLong(), rest).toInt()
        if (want <= 0) return 0

        val got = try {
            active.readInto(dst, off, want)
        } catch (e: Exception) {
            onReadFailure(e, myGeneration, active)
            return -1
        }
        if (got <= 0) {
            // 读到 0 字节（连接被源站掐掉）或 EOF。
            // **不等于片源读完** —— 这一块还有字节没拿到。
            onUnexpectedEof(active, myGeneration)
            return -1
        }
        synchronized(lock) {
            consecutiveFailures = 0
        }
        return got
    }

    /**
     * 这一块已经读满 ⇒ **归还连接**（关流 ⇒ 回 keep-alive 池）。
     *
     * 【为什么必须显式做】"读完"和"关流"是两件事：读满只意味着
     * [HttpOriginClient.OriginStream.consumed] 追上了块尾，连接还挂在池外 ——
     * 不显式关流，下一块只能**新建连接**（真机表头 754–968 ms，
     * 而复用只要 ≈300 ms）。
     *
     * @return true = 刚刚归还了一块（调用方可以开下一块了）。
     */
    fun finishIfExhausted(): Boolean {
        val active = synchronized(lock) { stream } ?: return false
        val myGeneration = synchronized(lock) { generation }
        val rest = active.endExclusive - active.start - active.consumed
        if (rest > 0L) return false
        finishChunk(active, myGeneration)
        return true
    }

    /** 跳读时由外部关掉当前块（生产线程随后会用新位置重开）。 */
    fun closeCurrent() {
        var old: HttpOriginClient.OriginStream? = null
        synchronized(lock) {
            old = stream
            stream = null
            generation += 1
        }
        runCatching { old?.abort() }
    }

    fun close() {
        var old: HttpOriginClient.OriginStream? = null
        synchronized(lock) {
            old = stream
            stream = null
            generation += 1
            lock.notifyAll()
        }
        runCatching { old?.abort() }
    }

    /** 这一块读满了：关流 ⇒ 连接回 keep-alive 池。 */
    private fun finishChunk(active: HttpOriginClient.OriginStream, myGeneration: Int) {
        active.finish()
        synchronized(lock) {
            if (stream === active) {
                stream = null
                generation += 1
            }
            lock.notifyAll()
        }
    }

    /**
     * 读失败的处理：**多数不是故障，是源站把连接掐了**。
     *
     * 正确的动作是**丢掉这块、重开一块接着读**（保留缓冲），
     * 而不是把错误抛给上层。
     */
    private fun onReadFailure(e: Exception, myGeneration: Int, active: HttpOriginClient.OriginStream) {
        val dropped = dropStream(active, myGeneration) ?: return
        runCatching { dropped.abort() }
        NetDiag.log("[$label] origin read fail: ${e.message}")
    }

    /** 块中途 EOF：只有真的读到片源末尾才算结束，否则按断连重开。 */
    private fun onUnexpectedEof(active: HttpOriginClient.OriginStream, myGeneration: Int) {
        val known = buffer.knownTotal
        val atEnd = known > 0 && active.start + active.consumed >= known
        if (atEnd) {
            val dropped = dropStream(active, myGeneration) ?: return
            runCatching { dropped.abort() }
            buffer.markEndOfFile()
            NetDiag.log("[$label] origin eof at file end")
            return
        }
        val dropped = dropStream(active, myGeneration) ?: return
        runCatching { dropped.abort() }
        NetDiag.log("[$label] origin eof unexpected, reopen @${buffer.writePosition}")
    }

    /**
     * 把当前块丢掉。
     *
     * @return 被丢掉的那块（null = 已经被别人换掉了，不是故障）。
     */
    private fun dropStream(
        active: HttpOriginClient.OriginStream,
        myGeneration: Int,
    ): HttpOriginClient.OriginStream? {
        var dropped: HttpOriginClient.OriginStream? = null
        synchronized(lock) {
            if (generation != myGeneration) {
                // 是我们自己换流造成的，不是故障。
                lock.notifyAll()
                return null
            }
            dropped = stream
            stream = null
            generation += 1
            consecutiveFailures += 1
            if (consecutiveFailures > MAX_CONSECUTIVE_FAILURES) {
                buffer.markFailure(
                    IOException("源站连接中断（已读 ${buffer.writePosition} / ${buffer.knownTotal}）"),
                )
            }
            lock.notifyAll()
        }
        return dropped
    }

    private fun sleepQuietly(ms: Long) {
        try {
            Thread.sleep(ms)
        } catch (interrupted: InterruptedException) {
            Thread.currentThread().interrupt()
        }
    }

    companion object {
        /** 连续失败多少次才把错误交给上层（期间自己重开连接）。 */
        private const val MAX_CONSECUTIVE_FAILURES = 5

        /** 重开连接前的退避。 */
        private const val REOPEN_BACKOFF_MS = 500L
    }
}
