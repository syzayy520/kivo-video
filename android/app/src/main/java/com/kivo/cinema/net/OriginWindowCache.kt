package com.kivo.cinema.net

/**
 * 273 v17/v18：**窗口缓存** —— 提前把片源里"内核起播一定会碰的那几段"抓进内存。
 *
 * 一组**显式范围**的窗口（片头 / 片尾 / 恢复点）。每个窗口在起播之前
 * 就从源站下好、落在内存里；读侧先查窗口，命中就本地供，**不碰源站**。
 *
 * 各窗口的**取数规律**不同（片尾冷、量小；片头大、量多；恢复点在中段），
 * 但**存与取**的机制完全一样 ⇒ 合成一个类，窗口各自带自己的
 * 范围、源站 client 与等待预算。**一种变化原因 ⇒ 一个文件。**
 *
 * ## 读侧的两种命中（273 v18）
 *  · **已就绪**：窗口下完了 ⇒ 本地供（v17 的行为）。
 *  · **在下**：窗口还在下 ⇒ **跟着它走**（等它把这一段的字节送上来）。
 *    【为什么必须能跟】恢复点那一段在源站是冷的（真机 0.5 MB/s ⇒ 首帧要多等 3.2 s）。
 *    预取连接**已经建好、已经在下** ⇒ 读侧跟着它，比自己另开一条连接
 *    （多付一次 TLS 握手、还多下一份同样的字节）**又快又省**。
 *
 * ## 缓存是加速器，不是依赖
 * 读不到就回源站，功能永远不缺。
 */
class OriginWindowCache {

    private val lock = Object()
    private val windows = ArrayList<OriginWindow>()

    /** 源被关掉：预取不许再往下下（否则离场后还有一条连接在偷偷跑）。 */
    @Volatile private var closed = false

    /**
     * 预取一段窗口（幂等）。
     *
     * 【幂等】同一段已经在飞或已就绪 ⇒ 直接返回 —— 起播路径可能多次触发。
     *
     * @param replaceTag 同标签的旧窗口要被换掉（落点窗口每次拖动都换）。
     *   换掉 = 旧窗口**从缓存里移除**并**叫停它的下载** ⇒ 内存与流量都有界。
     * @param parts 并行分片数（273 v32：>1 = 每片一条连接同时下）。
     */
    fun prefetch(
        start: Long,
        endExclusive: Long,
        client: HttpOriginClient,
        tag: String,
        waitMs: Long = DEFAULT_WAIT_MS,
        replaceTag: String? = null,
        parts: Int = 1,
    ) {
        if (endExclusive <= start) return
        val window = synchronized(lock) {
            if (closed) return
            val existing = windows.firstOrNull { it.start == start && it.endExclusive == endExclusive }
            if (existing != null && (existing.state == OriginWindow.State.PENDING || existing.state == OriginWindow.State.READY)) return
            if (existing != null) windows.remove(existing)
            if (replaceTag != null) {
                // 【换掉同标签的旧窗口】落点窗口每次拖动都换 ⇒ 不换就是内存与流量双泄漏。
                windows.filter { it.tag == replaceTag }.forEach {
                    it.cancelled = true
                    windows.remove(it)
                }
            }
            OriginWindow(start, endExclusive, client, tag, waitMs, parts).also { windows.add(it) }
        }
        Thread({ fetch(window) }, "KivoWindowPrefetch").apply {
            isDaemon = true
            start()
        }
    }

    /**
     * 登记一段**已经下好**的字节（273 v28）。
     *
     * 【它消掉哪一跳】内核 seek 时要读 MKV 的 cues 表（片尾那一段）。
     * 我们解析索引时已经下过**同一段** —— 不复用就是下两遍，
     * 而内核那一次在源站要新建连接（真机 v27-a：表头 **1,424 ms**）。
     *
     * 【与 [prefetch] 的区别】prefetch 是**去下**；这里是**收下已有的**
     * ⇒ 零额外下载、零额外往返。
     *
     * @param tag 同标签的旧窗口会被换掉（重复发布时不会留下两份）。
     */
    fun publishBytes(start: Long, bytes: ByteArray, tag: String) {
        if (bytes.isEmpty()) return
        val window = OriginWindow.ready(start, bytes, tag)
        synchronized(lock) {
            if (closed) return
            windows.filter { it.tag == tag }.forEach { windows.remove(it) }
            windows.add(window)
        }
        NetDiag.log("[$tag] 发布已就绪窗口 start=$start bytes=${bytes.size}")
    }

    /**
     * 某个标签的窗口是不是**还在下**（273 v31）。
     *
     * 【谁问它】播放主链路的预读线程：落点窗口在下的那几秒，
     * 落点那一跳在关键路径上，必须独占源站带宽 ⇒ 主链路这时让路。
     */
    fun hasPending(tag: String): Boolean = synchronized(lock) {
        windows.any { it.tag == tag && it.state == OriginWindow.State.PENDING }
    }

    /**
     * 这个偏移**有没有被某个窗口圈住**。
     *
     * 【为什么问"圈住"而不是"已经下到"】落点预取之前要问的是：
     * "这段字节是不是已经有人在下了？" —— **在下的窗口也算**，
     * 因为它边下边给（见 [OriginWindow.publish]），读侧跟着它走。
     * 若只认"已下到的字节"，那么拖动过程中每换一个位置都会
     * 另开一条预取（把上一条作废、白下一份）⇒ 流量与内存双泄漏。
     *
     * 【为什么归"范围"而不是"字节"】窗口还没下到的部分也是**承诺过的**字节
     * —— 预取连接已经建好、已经在下 ⇒ 不该再开第二条。
     */
    fun coversRange(offset: Long): Boolean {
        val snapshot = synchronized(lock) { ArrayList(windows) }
        for (w in snapshot) {
            if (offset >= w.start && offset < w.endExclusive) return true
        }
        return false
    }

    /**
     * 下一段窗口。
     *
     * 【边下边公布】每读到一块就公布（见 [OriginWindowDownloader]）——
     * 读侧不用等整块下完（片头 4 MiB 在冷源上要十几秒）。
     */
    private fun fetch(window: OriginWindow) {
        val started = System.currentTimeMillis()
        val outcome = OriginWindowDownloader(
            window,
            { closed || window.cancelled },
            { synchronized(lock) { lock.notifyAll() } },
        ).run()
        val ok = outcome.bytes > 0 && outcome.completed && !closed && !window.cancelled
        synchronized(lock) {
            window.state = if (ok) OriginWindow.State.READY else OriginWindow.State.FAILED
            lock.notifyAll()
        }
        NetDiag.log(
            "[${window.tag}] prefetch window=${window.start}..${window.endExclusive} " +
                "bytes=${outcome.bytes} parts=${window.sliceCount} ok=$ok " +
                "ms=${System.currentTimeMillis() - started}",
        )
    }

    /**
     * 从窗口里读一段。
     *
     * 【先命中、再跟、再回源站】已就绪的窗口直接供；落在**正在预取**的窗口里时，
     * 等它把这段字节送上来（上限 = 该窗口的等待预算）；都不行 ⇒ 回源站。
     *
     * @return 读到的字节数；-1 = 不在窗口里（调用方回源站）。
     */
    fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int {
        if (len <= 0 || offset < 0) return 0
        val snapshot = synchronized(lock) { ArrayList(windows) }
        for (w in snapshot) {
            val hit = w.copyTo(offset, buf, off, len)
            if (hit > 0) {
                NetDiag.log("[${w.tag}] cache hit offset=$offset bytes=$hit")
                return hit
            }
        }
        for (w in snapshot) {
            if (w.state != OriginWindow.State.PENDING || w.waitMs <= 0) continue
            if (offset < w.start || offset >= w.endExclusive) continue
            if (!awaitBytes(w, offset, w.waitMs)) continue
            val hit = w.copyTo(offset, buf, off, len)
            if (hit > 0) {
                NetDiag.log("[${w.tag}] cache hit offset=$offset bytes=$hit src=inflight")
                return hit
            }
        }
        return -1
    }

    /** 等某个偏移处有字节可读（窗口失败 / 超时 ⇒ 放弃）。 */
    private fun awaitBytes(w: OriginWindow, offset: Long, timeoutMs: Long): Boolean {
        val deadline = System.currentTimeMillis() + timeoutMs
        synchronized(lock) {
            while (w.bytesAt(offset) <= 0) {
                if (w.state != OriginWindow.State.PENDING) return w.bytesAt(offset) > 0
                val remain = deadline - System.currentTimeMillis()
                if (remain <= 0) return false
                try {
                    lock.wait(remain)
                } catch (interrupted: InterruptedException) {
                    Thread.currentThread().interrupt()
                    return false
                }
            }
            return true
        }
    }

    /**
     * 等某个**标签**的窗口落地，取它的内容。
     *
     * 【为什么读侧用不到它】读侧只要"这段字节在不在"⇒ [readAt] 就够。
     * 这里要的是"整块内容"（要**解析**它）⇒ 只有解析方（cues 索引）用得上。
     *
     * @param minBytes > 0 时：**前缀够这么多字节就可以走**（不必等整块下完）。
     *   解析方用它把"等整块 4 MiB"换成"等开头 64 KiB"⇒ 冷源上早十几秒开工。
     * @return 窗口快照；null = 没有这个窗口 / 预取失败 / 超时。
     */
    fun awaitWindow(tag: String, timeoutMs: Long, minBytes: Int = 0): Snapshot? {
        val deadline = System.currentTimeMillis() + timeoutMs
        synchronized(lock) {
            while (true) {
                val w = windows.firstOrNull { it.tag == tag } ?: return null
                when {
                    w.state == OriginWindow.State.READY -> {
                        val data = w.snapshot(w.filled) ?: return null
                        return Snapshot(w.actualStart, data)
                    }
                    minBytes > 0 && w.filled >= minBytes -> {
                        val data = w.snapshot(minBytes) ?: return null
                        return Snapshot(w.actualStart, data)
                    }
                    w.state == OriginWindow.State.FAILED -> return null
                    else -> {
                        val remain = deadline - System.currentTimeMillis()
                        if (remain <= 0L) return null
                        try {
                            lock.wait(remain)
                        } catch (interrupted: InterruptedException) {
                            Thread.currentThread().interrupt()
                            return null
                        }
                    }
                }
            }
        }
    }

    /** 某个标签的窗口**请求范围**（起点..终点，不含终点）。 */
    fun requestedRange(tag: String): LongRange? = synchronized(lock) {
        windows.firstOrNull { it.tag == tag }?.let { it.start until it.endExclusive }
    }

    /**
     * 源被关掉：叫停在飞的预取。
     *
     * 【为什么要有】预取在 t=0 就发出去 ⇒ 离场时它常常正在下载。
     * 不叫停就是"离场后还有一条连接在偷偷跑"（源站的连接额度也是它的）。
     */
    fun close() {
        synchronized(lock) {
            if (closed) return
            closed = true
            lock.notifyAll()
        }
    }

    /** 一个已落地的窗口：起点 + 内容。 */
    class Snapshot(val start: Long, val data: ByteArray)

    companion object {
        /** 窗口标签：片头（内核起播读容器头 + 第一个关键帧）。 */
        const val TAG_HEAD = "head"

        /** 窗口标签：片尾（MKV 的 cues 表）。 */
        const val TAG_TAIL = "tail"

        /** 窗口标签：恢复点（续播那一簇，273 v18）。 */
        const val TAG_RESUME = "resume"

        /** 窗口标签：拖动落点（273 v26 —— 每次拖动换一个）。 */
        const val TAG_SEEK = "seek"

        /** 窗口标签：cues 表本体（273 v28 —— 复用已经下好的那段）。 */
        const val TAG_CUES = "cues"

        /**
         * 读侧等预取落地的上限（毫秒）。
         *
         * 【为什么是 5 秒】内核起播时比预取早到约 350 ms。此时若读侧直接
         * 回源站，就是**又开一条连接**下同一段字节（还多付一次表头）。
         * 等它落地 ⇒ 一条连接都不多开。但等太久比回源站更糟 ⇒ 有上限。
         */
        const val DEFAULT_WAIT_MS = 5_000L

        /**
         * 解析 cues 所需的**最小前缀**（字节）。
         *
         * 【为什么是 64 KiB】cues 解析只需要开头的 SeekHead + Info
         * （真机实测在文件头几百 KB 内）⇒ 64 KiB 足够，冷源上 ≈0.3 s 就到。
         */
        const val PARSE_MIN_BYTES = 64 * 1024
    }
}
