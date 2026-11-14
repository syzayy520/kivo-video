package com.kivo.cinema.net

/**
 * 273 v6：**HTTP 片源**（Emby 等）的取数入口。
 *
 * ## 它是什么
 * 一层薄壳：真正的读在 [HttpOriginReader] 里（一条源站连接 + 环形预读 + 断连自愈），
 * 这里只负责"总长"与"按连接派发读者"。
 *
 * ## 为什么按连接派发
 * mpv 内核走 FFmpeg 的 `http` 协议，**每次 seek 都新开一条连接**，新旧连接会短暂重叠。
 * 单游标的源会被两个读者互相顶掉 ⇒ 每个连接必须有一套自己的连接与缓冲。
 * 见 [RangeReaderSource]。
 *
 * ## 预读为什么必要（真机实测）
 * 源站单连接能跑 4.9 MB/s（片源需求只有 415 KB/s），瓶颈不在源站，
 * 而在"按需取数"的节奏：mpv 要数据的瞬间才有供给 ⇒ 缓存被抽干 ⇒ 卡顿。
 * 详见 [HttpOriginReader] 的类注释。
 *
 * ## 分块大小（[chunkBytes]）
 * 每一块是一段**有界**范围请求：块读完 ⇒ 连接回 keep-alive 池 ⇒ 下一块复用
 * （真机：复用 ≈300 ms / 10 MB/s，新建 ≈800 ms / 2 MB/s）。
 * 预览源只要几十 KB（关键帧 / cues），块太大就是白下载 ⇒ 用小块。
 *
 * ## token
 * 只进 `X-Emby-Token` 请求头（[HttpOriginClient] 内部），**永不进 URL、永不进日志**。
 *
 * ## 总长的三个来源（273 v15）
 *  · [knownSize]：源站**实际响应**报出来的（`Content-Range` 的 `/total`）—— **真值**，
 *    代理写响应头只用它。
 *  · 外给的提示（Emby 详情里的 `MediaSources[0].Size`）—— 只用来让片尾预取
 *    **在 t=0 就发出去**（尾段在源站是冷的，晚发就晚到）。
 *  · 探测：两者都没有时的一次 1 字节范围请求（≈300 ms 往返）—— 兜底。
 *
 * ## 片尾窗口（273 v14/v15）与片头窗口（273 v17）
 * 内核起播时一定会跳到片尾读 MKV 的 cues，那一跳在关键路径上（真机 950 ms），
 * 且起播后还会**回跳到第一个关键帧**（真机偏移 2,127,000）—— 那是第二跳。
 * 窗口缓存（[OriginWindowCache]）把这两段提前抓进内存，命中时读侧
 * **完全不碰源站**。窗口大小 0 = 不启用（预览源不用）。
 */
class HttpOriginSource(
    url: String,
    token: String? = null,
    private val label: String = "play",
    private val capacityBytes: Int = HttpOriginReader.DEFAULT_CAPACITY_BYTES,
    chunkBytes: Long = HttpOriginClient.DEFAULT_CHUNK_BYTES,
    /** 保温几条 keep-alive 连接（0 = 不保温）。见 [OriginWarmupPool]。 */
    private val warmupConnections: Int = DEFAULT_WARMUP_CONNECTIONS,
    /** 片尾窗口大小（字节）；0 = 不做片尾缓存（预览源用 0，见 §十二）。 */
    tailWindowBytes: Long = DEFAULT_TAIL_WINDOW_BYTES,
    /**
     * 片头窗口大小（字节）；0 = 不做片头缓存（**只有详情页预热用**，见 273 v17）。
     *
     * 【为什么不是默认开】片头窗口是 4 MiB 量级的下载，**不该**在起播
     * 那一瞬间才发出去（那会跟内核的第一跳抢连接）。只有详情页预热
     * 那种"用户还在看简介"的场景才划算。
     */
    headWindowBytes: Long = 0L,
    /**
     * 恢复点窗口大小（字节）；0 = 不做恢复点预取（273 v18，**只有详情页预热用**）。
     *
     * 【它是什么】续播时内核 seek 到的那一簇（真机 8,490,718）提前抓进内存。
     * 中段在源站是冷的 ⇒ 提前下 ⇒ 起播那一跳一次源站往返都不付。
     */
    resumeWindowBytes: Long = 0L,
    /**
     * 落点窗口大小（字节）；0 = 不做落点预取（273 v26）。
     *
     * 【它是什么】拖动落点那一簇提前抓进内存。远跳之后内核要重取数，
     * 真机那一跳新建连接要 1,274 ms ⇒ 画面停 0.597 s（v24）。
     * 落点事先在本地 ⇒ 那一跳不必新建连接。
     */
    seekWindowBytes: Long = 0L,
    /**
     * **外给**的片源总长（Emby 详情里就有，零额外请求）。
     *
     * 【它只用来让片尾预取提前发请求】代理写响应头用的总长
     * 必须来自**实际响应**（`Content-Range` 的 `/total`），不采信这个提示 ——
     * 提示错了最多让预取白下一段（缓存按响应里的真实起点算，不会喂错字节）。
     */
    private val sizeHint: Long = -1L,
    /**
     * **借来的**窗口缓存；null = 自己建一个（273 v26）。
     *
     * 【谁会借】拖动预览源：它与播放主链路共用同一批窗口 ⇒ 落点窗口
     * 下载的那几 MB **预览与播放共用**，不必各下各的（真机 v24：
     * 同一段 `from=1613145699` 被下了两次，带宽被劈成两半）。
     */
    externalWindowCache: OriginWindowCache? = null,
) : RangeReaderSource, ByteSource {

    /**
     * 总长与访问校验（273 v26 拆出 —— 见 [OriginAccess]）。
     *
     * 【为什么拆】"片源多长 / 能不能访问"与"怎么读 / 怎么预取"是两种变化原因。
     */
    private val access = OriginAccess(url, token, label, chunkBytes)

    private val client get() = access.client

    /**
     * 播放侧的保温池。
     *
     * 【为什么落点窗口在飞时要跳过 ping】保温 ping 会借走一条空闲连接
     * （≈300 ms）⇒ 落点那几片就得多付一次 TLS 握手（真机 v32：表头到 2076 ms）。
     */
    private val warmupPool = OriginWarmupPool(client, label, skipPing = {
        client.landingInFlight?.invoke() == true
    })

    /**
     * 预取编排（273 v18）：片尾 / 片头 / 恢复点三个窗口都住在这里。
     *
     * 【为什么要单独的 client】证据里要能一眼分清"这一跳是预取还是播放"，
     * 所以每个预取用独立标签（`play-tail` / `play-head` / `play-resume`）。
     * 连接池是 JDK 全局的（`http.maxConnections` 是系统属性），
     * 几个 client 仍共用同一批 TCP。
     */
    private val prefetchKit = OriginPrefetchKit(
        url, token, label,
        tailWindowBytes, headWindowBytes, resumeWindowBytes, seekWindowBytes,
        externalWindowCache,
        { access.prefetchTotal(sizeHint) },
    ) { access.learnedSize = it }

    init {
        // 273 v31：落点窗口在飞时，主链路预读让路（真机 v30 落点只拿到 632 KB/s）。
        client.landingInFlight = { prefetchKit.windowCache.hasPending(OriginWindowCache.TAG_SEEK) }
    }

    /**
     * 本源的窗口缓存（读侧先查它）。
     *
     * 【谁要它】拖动预览源借用它（273 v26）⇒ 落点窗口下载一次，两边共用。
     */
    val windowCache: OriginWindowCache get() = prefetchKit.windowCache

    /**
     * 启动**保温池**（273 v15：延后调用，不在构造期自动启动）。
     *
     * 【为什么要延后】保温连接与起播关键路径上的请求**抢同一次 TLS 握手**：
     * 真机实测同时发起 4 条时每条要 1.2–1.6 s，而单独一条只要 ~0.8 s。
     * 起播要的是"**已经有**一条建好的连接"，不是"很多条"——
     * 所以让关键路径上的那一条**单独**建出来，其余连接随后再补。
     *
     * 【不调用会怎样】播放中拖动到未缓存位置没有空闲连接可借 ⇒ 新建连接
     * ⇒ 那一跳从 ~350 ms 变成 ~1.2 s（273-C 的白屏）。
     */
    fun startWarmup() {
        warmupPool.start(warmupConnections)
    }

    /** 旧内核（fd 路径）用的单一读者；MPV 路径走 [openReader]。 */
    @Volatile private var legacyReader: HttpOriginReader? = null

    /**
     * 源站报出来的总长（-1 = 还不知道）。
     *
     * 【代理只认它】响应头里的总长错了，内核会把文件长度看错
     * （截断或读越界）⇒ 只采信源站**实际响应**报出来的那个。
     */
    fun knownSize(): Long = access.knownSize()

    /**
     * 预取片尾窗口要用的总长：**外给的提示优先**。
     *
     * 【为什么提示优先】预取必须在 t=0 就发出去 —— 尾段在源站是**冷的**
     * （真机 273 v13/v14：0.34 MB/s，比片头慢一个量级），
     * 晚发 1 ms 就晚 1 ms 拿到。而尾窗口的起点 = 总长 - 窗口大小，
     * 所以总长必须**当场**有：Emby 详情里就有（零额外请求）。
     */
    fun prefetchTotal(): Long = access.prefetchTotal(sizeHint)

    override fun size(): Long = access.size()

    /**
     * 访问校验：**判决**用（并行跑，不挡起播）。
     *
     * 【为什么不再挡起播】v14 是在起播前同步探一次总长（真机 ≈300 ms 往返），
     * 只为把 401/403/404 变成一句用户看得懂的话。v15 把它挪到后台：
     * 起播立刻开始，只有**判决**才中止播放。
     *
     * 【什么算判决】源站**明确拒绝**（401/403/404/不支持范围）——
     * 这些是"再等也没用"的状态。瞬时 IO 错误（超时、断网）**不算判决**：
     * 读侧会自己撞上并报错，误杀一条本来能播的链路比多等 0.3 s 更糟。
     *
     * @return 判决原因（null = 没有判决）。
     */
    fun accessVerdict(): Throwable? = access.accessVerdict()

    override fun openReader(offset: Long): ByteSource = openReader(offset, capacityBytes)

    /**
     * 开一个读者，**自定环的大小**。
     *
     * 【谁用这个】拖动预览：它只看一帧，却也要占一条连接与一份环 ——
     * 预览的环按 2 MiB 给（[PlayerUrlStart.THUMB_CAPACITY_BYTES]）。
     */
    fun openReader(offset: Long, capacity: Int): ByteSource {
        val openDelegate: (Long) -> ByteSource = { at ->
            HttpOriginReader(client, at, capacity, label)
        }
        return CachedReader(offset, openDelegate, prefetchKit.windowCache) { knownSize() }
    }

    /** 预取片尾窗口（273 v15：**不等总长探测**）。 */
    fun prefetchTail() = prefetchKit.prefetchTail()

    /** 预取片头窗口（273 v17）：容器头 + 第一个关键帧。 */
    fun prefetchHead() = prefetchKit.prefetchHead()

    /**
     * 预取**恢复点**那一段（273 v18）。
     *
     * 【它消掉哪一跳】续播时内核要 seek 到"续播时间对应的簇"。
     * 真机那一跳（字节 8,490,718）多付 3.2 s —— 因为中段在源站是冷的。
     * 这里用**已经下好的 cues**（片尾窗口里）算出那个偏移，提前把那几 MB 抓下来。
     *
     * 【失败不影响播放】解析不出来 / 窗口没落地 ⇒ 不预取 ⇒ 那一跳照常回源站。
     */
    fun prefetchResume(resumeMs: Long) = prefetchKit.prefetchResume(resumeMs)

    /**
     * 预取**拖动落点**那一段（273 v26）。
     *
     * 【它消掉哪一跳】远跳之后内核要重取数。真机那一跳新建连接要 1,274 ms
     * ⇒ 画面停 0.597 s。落点那一簇事先在本地 ⇒ 那一跳不必新建连接。
     *
     * 【失败不影响播放】索引没有 / 落点算不出来 ⇒ 不预取 ⇒ 那一跳照常回源站。
     *
     * @param force true = 松手那一刻（不受节流约束）。
     */
    fun prefetchSeek(timeMs: Long, force: Boolean = false) = prefetchKit.prefetchSeek(timeMs, force)

    /** 让源站给落点那一跳让路（273 v29：拖动期间主链路预读与保温都停手）。 */
    fun yieldSource(ms: Long) = client.yieldGate.yieldFor(ms)

    /**
     * 后台把 cues 索引读出来（273 v26）。
     *
     * 【什么时候调】首帧之后 —— 拖动随时可能发生，索引备好就不用等。
     */
    fun warmCueIndex() = prefetchKit.warmCueIndex()

    /**
     * [ByteSource] 兼容口（旧内核的 fd 路径用）。
     *
     * 旧内核按随机偏移读，这里用**一个**读者顶着：它的跳读会重开连接。
     * MPV 路径不走这里（内核的每次 seek 都是一条新连接 ⇒ [openReader]）。
     */
    override fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int {
        val reader = legacyReader ?: synchronized(this) {
            legacyReader ?: HttpOriginReader(client, offset, capacityBytes, label).also { legacyReader = it }
        }
        return reader.readAt(offset, buf, off, len)
    }

    override fun close() {
        warmupPool.close()
        runCatching { prefetchKit.close() }
        val reader = synchronized(this) {
            val current = legacyReader
            legacyReader = null
            current
        }
        runCatching { reader?.close() }
    }

    companion object {
        /**
         * 默认保温几条连接。
         *
         * 【为什么是 2】同时活着的读者最多 3 个：
         * ① 顺序读片头的那个；② 跳去读文件尾 cues 的那个；③ 跳到断点续播位置的那个。
         * 每个读者占住一条连接，于是**跳读时池里没有空闲连接**可借
         * ⇒ 只能新建 ⇒ 多付一次 TLS 握手（真机 983–1301 ms vs 复用 270–360 ms）。
         * 保温 2 条 ⇒ 两次跳读各借一条；播放中拖动到未缓存位置也有连接可借
         * （真机 v9：那一跳从 1170 ms 降到 ≈350 ms）。
         *
         * 【为什么不是更多】每条保温连接都要占源站的连接额度；
         * 2 条已经覆盖了起播与拖动的全部跳读，再多只是徒增服务端压力。
         */
        const val DEFAULT_WARMUP_CONNECTIONS = 2

        /**
         * 默认片尾窗口大小（字节）。
         *
         * 【为什么是 64 KiB】MKV 的 cues 在片尾。真机实测（273 v14/v15）：
         * 内核请求的 cues 区间是 `bytes=854195684-`，即**最后 64,368 字节** ——
         * 64 KiB（65,536）刚好盖住它。
         *
         * 【为什么不做"精确 cues 大小"】要精确定位 cues 就得解析 EBML，
         * 而"字节匹配错一位"会把**错数据**喂给内核 —— 那比慢更糟。
         * 这里只按"片尾固定窗口"取，不解析容器结构。
         *
         * 【窗口不够大怎么办】缓存按响应里的真实起点算，
         * 盖不到内核要的那一段就是**不命中** ⇒ 读侧照常回源站（功能不缺，只是没加速）。
         */
        const val DEFAULT_TAIL_WINDOW_BYTES = 64L * 1024L

        /**
         * 默认落点窗口大小（字节）。
         *
         * 【为什么是 4 MiB】真机实测：远跳那一跳到首帧要 ~1.6 MB。
         * 4 MiB 把它盖住还有余量；再大就是白下（内核读完 4 MiB 就换连接了）。
         */
        const val DEFAULT_SEEK_WINDOW_BYTES = 4L * 1024L * 1024L
    }
}
