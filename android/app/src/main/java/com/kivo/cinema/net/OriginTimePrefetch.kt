package com.kivo.cinema.net

/**
 * 273 v26：**按时间定向预取** —— 把"用户要去的时间"换算成"精确的簇字节偏移"，
 * 再提前把那几 MB 抓进内存。
 *
 * ## 它消掉的是哪一跳
 * 真机（v24 拖动回归）：拖到 1.6 GB 处，内核那一跳在源站**新建了一条连接**，
 * 表头 **1,274 ms** ⇒ 画面停 **0.597 s**（`Enter buffering` → `End buffering`）。
 * 落点那一簇若**事先在本地**，那一跳不必新建连接 ⇒ 停顿消失。
 *
 * ## 为什么能提前知道那个偏移
 * 片源的 MKV 里有一张 cues 表（"刻度时间 → 簇位置"）——
 * 由 [OriginCueIndexSource] 在后台读好（片头拿位置、片尾拿表）。
 *
 * ## 猜错的后果
 * 只有"没命中"（那一跳照常回源站）⇒ **绝不会把错字节喂给内核**。
 *
 * ## v27：**节流**（真机 v26 的教训）
 * v26 在拖动过程中**每来一个 scrub 事件就发起一次 4 MiB 预取**：
 * 真机一次拖动（900 ms）里发了 **19 条**预取请求 ⇒
 * · 源站连接被占满（表头延迟从 587 ms 涨到 1,917 ms）；
 * · 每一条都**新建一条连接**（各自一次 TLS 握手），且互相作废 ⇒ 白下。
 * 真机结果：落点那一跳反而更慢（出画面 **4.5 s**，比没有预取的 v24 的 2.5 s 还差）。
 *
 * 【为什么不能靠"多开几条"】真机 Node 探针（v27）：同一段冷区，
 * 1 条连接 1.67 MiB/s、4 条 1.15 MiB/s、8 条 1.47 MiB/s ——
 * **源站的带宽是封顶的，并发只会互相踩**。
 *
 * ⇒ 所以这里只做两件事：
 * · **一次只留一条落点预取**（同标签替换）；
 * · **两次落点预取之间至少隔 [MIN_INTERVAL_MS]**（拖动中不新开连接）；
 *   松手那一刻（force = true）才允许立刻再发起。
 */
internal class OriginTimePrefetch(
    /** 窗口缓存（预取落在这里，读侧先查它）。 */
    private val cache: OriginWindowCache,
    /** 片源总长（外给提示优先；-1 = 还不知道）。 */
    private val totalOf: () -> Long,
    /** cues 索引（null = 没有 ⇒ 不预取）。 */
    private val indexOf: () -> MkvCueIndex?,
) {

    /** 上一次**发起**落点预取的墙钟时刻（毫秒）；-1 = 从没发起过。 */
    private var lastStartWallMs = -1L

    /**
     * 预取"某个时间对应的簇"那一段。
     *
     * @param timeMs 目标时间（毫秒）。
     * @param tag 窗口标签（同一标签的旧窗口会被替换 ⇒ 内存有界）。
     * @param client 这一跳专用的源站 client。
     * @param windowBytes 窗口大小（字节）。
     * @param force true = 不受节流约束（松手那一刻用：那时位置已定，必须立刻下）。
     * @param parts 并行分片数（273 v32：>1 = 每片一条连接同时下）。
     * @return true = 已经发起预取；false = 没有预取（照常走网络，功能不缺）。
     */
    fun prefetch(
        timeMs: Long,
        tag: String,
        client: HttpOriginClient,
        windowBytes: Long,
        force: Boolean = false,
        parts: Int = 1,
    ): Boolean {
        if (timeMs <= 0L || windowBytes <= 0L) return false
        val index = indexOf() ?: return false
        // 【覆盖不到就不猜】cues 表可能被缓冲夹住（只读到前半段）⇒
        // 超出范围的时间算出来的偏移是**错的** ⇒ 宁可**不预取**。
        if (!index.covers(timeMs)) {
            NetDiag.log("[$tag] ${timeMs}ms 不在 cues 覆盖范围内 ⇒ 不预取")
            return false
        }
        val offset = index.clusterOffsetAt(timeMs)
        if (offset < 0L) return false
        // 【已经有人在下了】落点被某个窗口圈住 ⇒ 不重复预取
        // （那个窗口边下边给，读侧跟着它走 ⇒ 比另开一条更快也更省）。
        if (cache.coversRange(offset)) {
            NetDiag.log("[$tag] 落点 $offset 已在窗口内 ⇒ 不重复预取")
            return false
        }

        // 【节流】拖动过程中每 50 ms 就来一个位置 ⇒ 每来一个就开一条 4 MiB 的连接，
        // 真机一次拖动开了 19 条 ⇒ 源站连接被占满、落点那一跳更慢。
        // 这里把"发起新落点预取"的频率封顶；松手那一刻（force）不受此限。
        val now = System.currentTimeMillis()
        if (!force && lastStartWallMs >= 0L && now - lastStartWallMs < MIN_INTERVAL_MS) {
            NetDiag.log("[$tag] 距上次落点预取不足 ${MIN_INTERVAL_MS}ms ⇒ 跳过（不新开连接）")
            return false
        }
        lastStartWallMs = now

        val total = totalOf()
        // 【窗口贴着落点铺】起点往前挪一截（兜住内核那几字节的回退），
        // 终点正好是"落点 + 窗口大小" ⇒ 落点之后的窗口大小**整段**都在窗口里。
        val start = (offset - BACKOFF_BYTES).coerceAtLeast(0L)
        val end = if (total > 0L) minOf(offset + windowBytes, total) else offset + windowBytes
        if (end <= start) return false
        NetDiag.log(
            "[$tag] cues=${index.count} 条，${timeMs}ms → 偏移 $offset，预取 $start..$end",
        )
        cache.prefetch(start, end, client, tag, READ_WAIT_MS, replaceTag = tag, parts = parts)
        return true
    }

    companion object {
        /**
         * 读侧等落点窗口的**上限**（毫秒）。
         *
         * 【为什么这次要等】窗口是**边下边给**的（[OriginWindow.publish]），
         * 读侧跟着它走 ⇒ 等的是"本来就该到的字节"，不是白等。
         * 而且预取连接**已经建好** ⇒ 比读侧另开一条连接（多一次 TLS 握手、
         * 多下一份同样的字节）**更快也更省**。
         */
        const val READ_WAIT_MS = 8_000L

        /**
         * 窗口起点往前留的余量（字节）。
         *
         * 【为什么留】内核 seek 的落点可能比簇头早几个字节
         * （demuxer 的安全回退）。差这几个字节就要另开一条连接 ⇒ 留一截兜住。
         */
        const val BACKOFF_BYTES = 64L * 1024L

        /**
         * 两次落点预取之间的**最小间隔**（毫秒）。
         *
         * 【为什么是 800】真机 v26：拖动 900 ms 里发了 19 条预取（每 50 ms 一条），
         * 每条各自新建连接、各自 4 MiB ⇒ 源站连接被占满（表头 587→1,917 ms），
         * 落点那一跳反而从 2.5 s 变成 4.5 s。
         * 800 ms ⇒ 一次拖动最多两三条。
         */
        const val MIN_INTERVAL_MS = 800L
    }
}
