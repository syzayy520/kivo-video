package com.kivo.cinema.net

/**
 * 273 v9：源站 keep-alive 连接池的**保温**。
 *
 * ## 它解决什么
 *
 * 真机 v7 取数层日志（`net-diag.log`，4K HEVC 片源）里，表头延迟分成两档：
 *
 * | 档 | 表头延迟 | 出现在 |
 * | --- | --- | --- |
 * | 复用 | **270–360 ms** | 绝大多数分块请求 |
 * | 新建 | **983–1301 ms** | 读文件尾的 cues、跳到断点续播位置、拖动到未缓存位置 |
 *
 * 差的那 ~800 ms 是 TLS 握手。**为什么偏偏这些要新建**：内核每次跳读都新开一条
 * 客户端连接 ⇒ 取数层多一个读者 ⇒ 新读者要一条自己的源站连接；
 * 而那一刻**池里的连接都在传数据**（每个读者的预读线程正占着一条），
 * 没有空闲连接可借 ⇒ 只能新建。
 *
 * ## 为什么"一次性预热"不够
 *
 * v7 只在源创建时预热 2 条。起播窗口里够了（实测表头 max 从 1076 ms 降到 594 ms），
 * 但**播放中拖动到未缓存位置**仍然新建：真机实测那一跳表头 **1170 ms**，
 * 从"松手"到"出画面"要 **2.2 秒** —— 这正是用户说的"后面没缓存到的地方直接白屏"。
 *
 * 复盘：预热的那两条连接**被起播阶段吃掉了**（cues 读者 + 播放读者各一条），
 * 之后池里长期是空的 —— 顺序播放时只有一条连接在"取—还—取"之间循环，
 * 池里并没有第二条空闲连接留给跳读。
 *
 * ## 做法：持续保温
 *
 * [count] 条后台线程各自每 [REFRESH_MS] 发一个 **1 字节**范围请求并读完。
 * 读满响应体 ⇒ 连接回 JDK 的 keep-alive 池 ⇒ 池里始终留着空闲连接，
 * 跳读随时能借到（≈300 ms），不用付 TLS 握手（≈1000 ms）。
 *
 * 【为什么是并行而不是一轮发多个】一轮发多个 = 串行做完，前一个没回来时
 * 后一个还在排队 —— 真机实测这样会把**起播**拖慢：起播那两次跳读正好撞上
 * 保温请求占着连接，表头从 296 ms 变成 978 ms / 1211 ms（比不做保温还慢）。
 * 并行才有"三条连接同时握手"的效果。
 *
 * 【代价有多大】每个请求 1 字节；[count] = 2、周期 8 s ⇒ 15 次/分钟、15 字节/分钟。
 * 换来的是每一次跳读省 ~870 ms。
 *
 * 【为什么源站不会掐掉这些连接】真机实测（Node 探针，源站 keep-alive）：
 * 空闲 30 秒后复用，表头仍是 **320 ms**（= 复用，不是新建 1000+ ms）——
 * 服务端不会因为空闲而断开，所以保温只需要"别让池空着"，不需要高频 ping。
 *
 * 【为什么失败可以忽略】保温失败只是少一次复用机会，主路径完全不受影响
 * —— 所以这里吞掉异常，但**留一行日志**（不许静默）。
 */
internal class OriginWarmupPool(
    private val client: HttpOriginClient,
    private val label: String,
    /**
     * 这一路现在**该不该跳过保温**（273 v32b）。
     *
     * 【为什么需要】落点窗口正在下时，源站的空闲连接要**留给它**
     * —— 保温 ping 会借走一条空闲连接（≈300 ms），落点那一片就得多付一次握手。
     */
    private val skipPing: () -> Boolean = { false },
    /** 保温周期（毫秒）。 */
    private val periodMs: Long = REFRESH_MS,
) {

    @Volatile private var closed = false
    @Volatile private var started = false
    private val keepers = mutableListOf<Thread>()

    /**
     * 保温 [count] 条连接（0 = 不保温）。返回后不等它们完成（后台线程自己跑）。
     *
     * 【幂等】重复调用只生效一次 —— 保温线程是"长期在飞"的，
     * 第二次调用会多开一组线程，白占源站的连接额度。
     */
    fun start(count: Int) {
        if (count <= 0 || started || closed) return
        started = true
        repeat(count) { index ->
            val thread = Thread({ keep(index) }, "kivo-origin-warmup-$index").apply {
                isDaemon = true
                start()
            }
            synchronized(keepers) { keepers += thread }
        }
    }

    /** 停掉保温线程（源关闭时调用）。 */
    fun close() {
        closed = true
        val threads = synchronized(keepers) {
            val copy = keepers.toList()
            keepers.clear()
            copy
        }
        threads.forEach { runCatching { it.interrupt() } }
    }

    private fun keep(index: Int) {
        var round = 0
        while (!closed) {
            // 273 v29：让路期间**不发 ping**。
            // 【为什么】保温请求会从池里借走一条空闲连接并占住它（≈300 ms），
            // 落点那一跳撞上这个窗口就只能新建连接（真机新建表头 763–1,292 ms，
            // 复用只要 306–822 ms）⇒ 让路期间把空闲连接留在池里给落点用。
            if (!client.yieldGate.isYielding() && !skipPing()) {
                val ok = client.warmConnection()
                // 只在失败时记日志：成功是常态，几小时的电影不该刷出几千行。
                if (!ok) NetDiag.log("[$label] origin keepalive #$index round=$round failed")
            }
            round++
            try {
                Thread.sleep(periodMs)
            } catch (interrupted: InterruptedException) {
                Thread.currentThread().interrupt()
                return
            }
        }
    }

    companion object {
        /**
         * 保温周期（毫秒）。
         *
         * 【为什么 8 秒】源站空闲 30 秒仍可复用（真机实测），8 秒远在安全线内；
         * 再快只是白耗源站的连接额度。
         */
        private const val REFRESH_MS = 8_000L
    }
}
