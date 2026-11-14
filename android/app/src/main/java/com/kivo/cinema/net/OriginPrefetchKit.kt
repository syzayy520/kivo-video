package com.kivo.cinema.net

/**
 * 273 v18/v26：**预取编排** —— 决定"起播与拖动之前要抓哪几段字节"。
 *
 * ## 它是什么
 * 一组**显式范围**的窗口，每个窗口一个独立的源站 client（独立标签 ⇒
 * 证据里一眼分得清哪一跳是预取）：
 * · **片尾**（`tail`）：MKV 的 cues 表 —— 内核起播必跳到片尾读它。
 * · **片头**（`head`）：容器头 + 第一个关键帧 —— 内核起播的第一跳与第二跳。
 * · **恢复点**（`resume`）：续播那一簇 —— 用 cues 算出偏移后定向抓（273 v18）。
 * · **落点**（`seek`）：拖动落点那一簇 —— 同样用 cues 算（273 v26）。
 *
 * ## 为什么从 [HttpOriginSource] 里分出来
 * "起播前抓哪几段"与"起播时怎么读"是**两种变化原因**：
 * 加一种窗口（片头 / 恢复点 / 落点）不该动读侧派发逻辑，反之亦然。
 *
 * ## 窗口是加速器，不是依赖
 * 抓不到就回源站，功能永远不缺。
 */
internal class OriginPrefetchKit(
    url: String,
    token: String?,
    label: String,
    /** 片尾窗口大小（字节）；0 = 不抓。 */
    private val tailWindowBytes: Long,
    /** 片头窗口大小（字节）；0 = 不抓。 */
    private val headWindowBytes: Long,
    /** 恢复点窗口大小（字节）；0 = 不抓。 */
    private val resumeWindowBytes: Long,
    /** 落点窗口大小（字节）；0 = 不抓。 */
    private val seekWindowBytes: Long,
    /**
     * **借来的**窗口缓存；null = 自己建一个。
     *
     * 【谁会借】拖动预览源（273 v26）：它与播放主链路**共用同一批窗口**，
     * 于是落点窗口下载的那几 MB，**预览与播放各取所需，不必各下各的**。
     * 真机 v24：预览与播放各下一次同一段（`from=1613145699` 出现两次），
     * 带宽被劈成两半，两边都慢。
     */
    externalWindowCache: OriginWindowCache? = null,
    /** 片源总长（外给提示优先；-1 = 还不知道）。 */
    private val totalOf: () -> Long,
    /** 任何一次响应报出总长时叫一声（源据此记住总长）。 */
    private val onTotalKnown: (Long) -> Unit,
) {

    /** 窗口缓存：所有窗口都住在这里，读侧查它。 */
    val windowCache = externalWindowCache ?: OriginWindowCache()

    /** 借来的缓存不由本 kit 关（关源是它主人的事）。 */
    private val ownsWindowCache = externalWindowCache == null

    /** 片尾预取专用 client；null = 不做片尾预取。 */
    private val tailClient = if (tailWindowBytes > 0) {
        HttpOriginClient(url, token, "$label-tail", tailWindowBytes, onTotalKnown)
    } else {
        null
    }

    /** 片头预取专用 client；null = 不做片头预取。 */
    private val headClient = if (headWindowBytes > 0) {
        HttpOriginClient(url, token, "$label-head", headWindowBytes, onTotalKnown)
    } else {
        null
    }

    /** 恢复点预取专用 client；null = 不做恢复点预取。 */
    private val resumeClient = if (resumeWindowBytes > 0) {
        HttpOriginClient(url, token, "$label-resume", resumeWindowBytes, onTotalKnown)
    } else {
        null
    }

    /** 落点预取专用 client；null = 不做落点预取。 */
    private val seekClient = if (seekWindowBytes > 0) {
        HttpOriginClient(url, token, "$label-seek", seekWindowBytes, onTotalKnown)
    } else {
        null
    }

    /**
     * 落点连接的**保温池**（273 v32b）。
     *
     * 【为什么需要】落点窗口是 K 片并行（每片一条连接）⇒ 它一次要 K 条连接。
     * 池里没有空闲连接 ⇒ 每片各自新建 ⇒ 每片都付一次 TLS 握手
     * （真机 v32：4 片的表头 280 / 966 / 1041 / 2076 ms —— 三片是新建的）。
     * 保温 K 条 ⇒ 落点那几片都能借到**热连接**（表头 ≈300 ms）。
     *
     * 【为什么保温期间要跳过 ping】落点窗口在下时，空闲连接要留给它 ——
     * 保温 ping 会借走一条空闲连接（≈300 ms）⇒ 那一片就得多付一次握手。
     */
    private val seekWarmup = seekClient?.let {
        OriginWarmupPool(
            it,
            "$label-seek",
            { windowCache.hasPending(OriginWindowCache.TAG_SEEK) },
            SEEK_WARMUP_PERIOD_MS,
        )
    }

    /**
     * cues 索引的来源；null = 不需要（没有落点/恢复点窗口）。
     *
     * 【为什么索引要先备好】落点与恢复点都要"时间 → 字节偏移"这张表 ⇒
     * 表在后台读好（两次小请求），拖动/续播时**零等待**。
     */
    private val cueIndexSource = if (resumeWindowBytes > 0L || seekWindowBytes > 0L) {
        OriginCueIndexSource(
            HttpOriginClient(url, token, "$label-cue", CUE_CHUNK_BYTES, onTotalKnown),
            windowCache,
        )
    } else {
        null
    }

    /** 按时间定向预取（落点与恢复点共用一张索引、同一套算法）。 */
    private val timePrefetch = OriginTimePrefetch(
        windowCache,
        totalOf,
    ) { cueIndexSource?.index }

    init {
        // 【晚 [SEEK_WARMUP_DELAY_MS] 再起保温池】落点那几片要一次借 K 条连接，
        // 池里没有就各自新建（每片一次 TLS 握手）⇒ 落点表头 966–2076 ms。
        // 起播关键路径上那几个连接更金贵 ⇒ 让它们先建，保温池晚点再补。
        val pool = seekWarmup
        if (pool != null) {
            Thread({
                try {
                    Thread.sleep(SEEK_WARMUP_DELAY_MS)
                } catch (interrupted: InterruptedException) {
                    Thread.currentThread().interrupt()
                    return@Thread
                }
                pool.start(LANDING_WINDOW_PARTS)
            }, "KivoSeekWarmupDelay").apply {
                isDaemon = true
                start()
            }
        }
    }

    /**
     * 预取片尾窗口（**不等总长探测**）。
     *
     * 【什么时候调】起播准备阶段，越早越好 —— 尾段在源站是冷的，
     * 预取每晚发 1 ms，内核那一跳就晚 1 ms 拿到（真机那一跳 ≈950 ms）。
     *
     * 【失败不影响播放】预取失败只是少一次加速，读侧照常走网络。
     */
    fun prefetchTail() {
        val client = tailClient ?: return
        val total = totalOf()
        if (total <= 0) return
        val start = (total - tailWindowBytes).coerceAtLeast(0L)
        windowCache.prefetch(start, total, client, OriginWindowCache.TAG_TAIL)
    }

    /**
     * 预取片头窗口。
     *
     * 【为什么这一段值钱】内核起播要读容器头（片头那一跳），
     * 读完之后又会**回跳到第一个关键帧**（真机偏移 2,127,000）—— 那是第二跳。
     * 两跳都在起播关键路径上，各付一次源站表头（≈300 ms 往返）。
     * 第一个关键帧落在**片头窗口**里（真机 2,127,000 < 4 MiB）⇒
     * 两跳都能本地供 ⇒ 关键路径上**一次源站往返都不剩**。
     *
     * 【落不了就回源站】窗口没落地时读侧**不等**它（等待预算 = 0）——
     * 起播那一跳等不起，回源站仍是老路（300 ms 往返），不会更差。
     */
    fun prefetchHead() {
        val client = headClient ?: return
        val total = totalOf()
        val end = if (total > 0) minOf(headWindowBytes, total) else headWindowBytes
        windowCache.prefetch(0L, end, client, OriginWindowCache.TAG_HEAD, HEAD_WAIT_MS)
    }

    /**
     * 预取**恢复点**那一段（273 v18）。
     *
     * 【它消掉哪一跳】续播时内核要 seek 到"续播时间对应的簇"。
     * 真机那一跳（字节 8,490,718）多付 3.2 s —— 因为中段在源站是冷的。
     * 这里用**已经下好的 cues** 算出那个偏移，提前把那几 MB 抓下来。
     *
     * 【失败不影响播放】解析不出来 / 窗口没落地 ⇒ 不预取 ⇒ 那一跳照常回源站。
     */
    fun prefetchResume(resumeMs: Long) {
        val client = resumeClient ?: return
        // 【索引要先就绪】算偏移要 cues 表 ⇒ 这里同步读一次
        // （这一步在详情页阶段做，不占起播关键路径）。
        cueIndexSource?.warm()
        timePrefetch.prefetch(
            resumeMs, OriginWindowCache.TAG_RESUME, client, resumeWindowBytes,
            parts = LANDING_WINDOW_PARTS,
        )
    }

    /**
     * 预取**拖动落点**那一段（273 v26）。
     *
     * 【它消掉哪一跳】远跳之后内核要重取数。真机（v24）：拖到 1.6 GB 处时
     * 那一跳在源站**新建了一条连接**，表头 **1,274 ms** ⇒ 画面停 **0.597 s**。
     * 落点那一簇事先在本地 ⇒ 那一跳不必新建连接。
     *
     * @param force true = 松手那一刻（位置已定，必须立刻下，不受节流约束）。
     */
    fun prefetchSeek(timeMs: Long, force: Boolean = false) {
        val client = seekClient ?: return
        timePrefetch.prefetch(
            timeMs, OriginWindowCache.TAG_SEEK, client, seekWindowBytes, force,
            parts = LANDING_WINDOW_PARTS,
        )
    }

    /**
     * 后台把 cues 索引读出来（拖动之前备好 ⇒ 拖动时零等待）。
     */
    fun warmCueIndex() {
        cueIndexSource?.warmAsync()
    }

    /** 源被关掉：叫停在飞的预取（借来的缓存不关 —— 它还有别的主人）。 */
    fun close() {
        seekWarmup?.close()
        if (ownsWindowCache) windowCache.close()
    }

    companion object {
        /**
         * 落点窗口（落点 / 恢复点）分几片并行下（273 v32）。
         *
         * 【为什么是 4】源站的吞吐是**每连接**封顶的（真机 K40：单连接
         * ≈700 KB/s，8 条并起来 ≈2,483 KB/s）⇒ 4 片能把落点那一段的
         * 墙钟时间压到约 1/4。片再细（8 × 512 KiB）反而更慢 ——
         * 每片都要一次表头往返，摊不薄。
         */
        private const val LANDING_WINDOW_PARTS = 4

        /**
         * 落点保温池的**起跑延迟**（毫秒）。
         *
         * 【为什么要延后】落点保温池一次占 [LANDING_WINDOW_PARTS] 条连接；
         * 而起播关键路径上那几个连接更金贵（内核第一跳、cues、片头窗口）
         * ⇒ 让它们先建，保温池晚点再补。
         */
        private const val SEEK_WARMUP_DELAY_MS = 5_000L

        /**
         * 落点保温池的周期（毫秒）。
         *
         * 【为什么是 15 秒】源站空闲 30 秒仍可复用（真机实测）⇒ 15 秒安全；
         * 又比播放侧保温池（8 秒）省一半的往返。
         */
        private const val SEEK_WARMUP_PERIOD_MS = 15_000L

        /**
         * 片头窗口的等待预算（毫秒）。
         *
         * 【为什么是 0 —— 不等】片头窗口是 4 MiB 量级的下载，
         * 起播那一瞬间它常常**还没落地**。等它 = 白等几秒；
         * 不等 = 回源站（老路 ≈300 ms 往返）。宁可走老路也不赌。
         */
        private const val HEAD_WAIT_MS = 0L

        /**
         * 读 cues 用的分块大小（字节）。
         *
         * 【为什么是 256 KiB】片头探针 64 KiB + cues 本体 ≈130 KB ⇒
         * 一次请求就够，不用"探 + 重取"两次往返。
         */
        private const val CUE_CHUNK_BYTES = 256L * 1024L
    }
}
