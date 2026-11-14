package com.kivo.cinema.net

import java.util.concurrent.CountDownLatch

/**
 * 273 v16：**进程级**的源站预热 —— 详情页就把连接与片尾窗口备好，玩家页接手就用。
 *
 * ## 为什么要有它
 * 真机实测（273 v15/v16，emby.bangumi.ca，Redmi K40）：
 * · 源站**新建连接**要 1000–1600 ms（TLS 握手）；
 * · 源站**复用**连接只要 270–360 ms。
 *
 * 玩家页的起播关键路径上，第一跳（片头）恰好撞上"连接还没建好" ⇒ 白付一次握手。
 * 而**详情页早就有 URL、token、片源总长**（零额外请求，见 `EmbyResource`）——
 * 用户还在看详情页的时候，连接就可以先建好。
 *
 * ## 预热做两件事
 * · **探测**（1 字节范围请求）：顺带把**总长记进源**、把**连接放回 keep-alive 池**
 *   （读完响应体 ⇒ 连接回池 ⇒ 玩家页第一跳直接复用，约 300 ms）。
 * · **片尾预取**：把 MKV 的 cues 那一段抓进内存（内核起播必跳到片尾读 cues）。
 *
 * ## 为什么是**进程级**
 * 详情页与玩家页在**同一个进程**（同一个 applicationId）⇒
 * ① JDK 的 keep-alive 连接池是**进程全局**的（`HttpURLConnection` 的
 * `KeepAliveCache` 是静态的）⇒ 预热建的连接天然能被玩家页复用；
 * ② 片尾缓存挂在**源对象**上 ⇒ 源被玩家页接手，缓存跟着走。
 *
 * ## 所有权
 * · 登记处持有预热出去的源；玩家页 [take] 走 ⇒ 归玩家页（Activity 在 onDestroy 关）。
 * · 没人接手 ⇒ 详情页销毁时 [discard] 关掉（否则连接与缓存泄漏）。
 */
object OriginPrewarm {

    /**
     * 一次预热的结果。
     *
     * @param source 预热好的源（**片尾缓存与连接池都在它身上**）。
     * @param verdict 源站**明确拒绝**时的理由（null = 没有判决）。
     * @param ready 预热就绪信号（探测完成后放行 —— 片尾预取不占这个信号）。
     */
    class Entry(
        val source: HttpOriginSource,
        @Volatile var verdict: Throwable? = null,
        val ready: CountDownLatch = CountDownLatch(1),
    )

    private val lock = Object()
    private val entries = HashMap<String, Entry>()

    /**
     * 预热用的片尾窗口（字节）—— 比播放路径的默认窗口**大一档**。
     *
     * 【为什么不沿用默认的 64 KiB】内核读 cues 的长度**随片源变**：
     * 真机两个片源分别是 64,368 B 与 **130,494 B**（后者盖不住 64 KiB ⇒ 缓存不命中 ⇒
     * 那一跳仍要回源站，真机表头 278–370 ms）。窗口放大到 256 KiB 就都盖住了。
     *
     * 【为什么放大不会像 v14 那样变慢】v14 被打回来的原因是"1 MiB 窗口落地更晚，
     * 读侧要等它"—— 那条结论属于**起播瞬间才发**的预取（尾段在源站是冷的，
     * 0.34 MB/s）。这里的预热是**详情页**发起的：用户还在看简介，
     * 窗口早就落地 ⇒ 放大窗口只是多下一次几百 KB，**不占起播关键路径**。
     *
     * 【盖不住怎么办】缓存按响应里的真实起点算，盖不到内核要的那一段就是**不命中**
     * ⇒ 读侧照常回源站（功能不缺，只是没加速）。
     */
    private const val PREWARM_TAIL_WINDOW_BYTES = 256L * 1024L

    /**
     * 预热用的片头窗口（字节）。
     *
     * 【为什么是 4 MiB】内核起播有**两跳**在关键路径上：
     * ① 读容器头（片头那一跳）；② 读完之后**回跳到第一个关键帧**
     * （真机偏移 2,127,000）。第二跳的落点在第一跳读到的范围里 ——
     * 4 MiB 的窗口把它盖住 ⇒ 两跳都能本地供。
     *
     * 【为什么是 4 MiB 而不是更小】第一个关键帧的偏移随片源变
     * （真机这次是 2,127,000）。更小的窗口会盖不住 ⇒ 第二跳白走源站。
     *
     * 【为什么不再大】窗口越大，详情页阶段的下载越久、流量越大。
     * 4 MiB = 一次源站分块的大小（[HttpOriginClient.DEFAULT_CHUNK_BYTES]），
     * 刚好够一个关键帧簇，够了。
     *
     * 【盖不住怎么办】窗口按响应的真实起点算，盖不到就是**不命中** ⇒
     * 读侧照常回源站（功能不缺，只是没加速）。
     */
    private const val PREWARM_HEAD_WINDOW_BYTES = 4L * 1024L * 1024L

    /**
     * 预热用的**恢复点**窗口（字节）—— 273 v18。
     *
     * 【它是什么】续播时内核 seek 到的那一簇（真机字节 8,490,718）。
     * 那一段在源站是**冷的**（真机 0.5 MB/s，且 CDN 边缘没缓存 ⇒ 首次取要 3.2 s）。
     * 详情页阶段先把它抓下来 ⇒ 起播那一跳**一次源站往返都不付**。
     *
     * 【为什么是 4 MiB】真机实测：那一跳到首帧要 ~1.6 MB（3.2 s ÷ 0.5 MB/s）。
     * 4 MiB 把它盖住还有余量；再大就是白下（内核读完 4 MiB 就换连接了）。
     *
     * 【猜错的后果】cues 解析不出来 ⇒ 不预取 ⇒ 那一跳照常回源站（功能不缺）。
     */
    private const val PREWARM_RESUME_WINDOW_BYTES = 4L * 1024L * 1024L

    /**
     * 预热一个 URL 的片源（详情页拿到 URL/token/总长时调）。
     *
     * @param resumeMs 续播位（毫秒）；0 = 没有续播位（不预取恢复点）。
     *
     * 【幂等】同一个 URL 已经在预热 ⇒ 不重复 —— 重复开连接是白占源站额度。
     */
    fun offer(url: String, token: String?, sizeHint: Long, resumeMs: Long = 0L) {
        synchronized(lock) {
            if (entries.containsKey(url)) return
        }
        val source = HttpOriginSource(
            url, token, "play",
            tailWindowBytes = PREWARM_TAIL_WINDOW_BYTES,
            headWindowBytes = PREWARM_HEAD_WINDOW_BYTES,
            resumeWindowBytes = PREWARM_RESUME_WINDOW_BYTES,
            sizeHint = sizeHint,
        )
        val entry = Entry(source)
        synchronized(lock) { entries[url] = entry }
        // URL 里**没有 token**（token 只在请求头里）⇒ 可以进日志：
        // 证据里要一眼看出"这一轮预热的是哪个片源"。
        NetDiag.log("[prep] 预热开始 url=$url size=$sizeHint resumeMs=$resumeMs")
        Thread({
            try {
                // 片头窗口**先发**：内核起播的第一跳就是它（读容器头），
                // 而它也是起播关键路径上最长的一段下载（4 MiB）。
                source.prefetchHead()
                // 片尾预取：有总长提示时它不必等探测（尾段在源站是冷的，
                // 早发 1 ms 就早 1 ms 拿到）。
                if (sizeHint > 0) source.prefetchTail()
                // 探测：顺带把连接放回池（读完响应体 ⇒ 回池 ⇒ 玩家页第一跳复用）。
                entry.verdict = source.accessVerdict()
                if (sizeHint <= 0) source.prefetchTail()
            } catch (e: Exception) {
                NetDiag.log("[prep] 预热失败：${e.javaClass.simpleName}: ${e.message}")
            } finally {
                entry.ready.countDown()
            }
            // 恢复点窗口（273 v18）：**不占起播关键路径** ⇒ 放在 ready 之后。
            // 它要等片头（布局）与片尾（cues）两个窗口落地才能算出偏移。
            try {
                source.prefetchResume(resumeMs)
            } catch (e: Exception) {
                NetDiag.log("[resume] 恢复点预取失败：${e.javaClass.simpleName}: ${e.message}")
            }
        }, "KivoOriginPrewarm").start()
    }

    /**
     * 玩家页接手（URL 对得上才接手）。
     *
     * @return 预热结果；null = 没有可接手的预热（玩家页自己从头备）。
     */
    fun take(url: String): Entry? = synchronized(lock) { entries.remove(url) }

    /**
     * 详情页销毁：没人接手的预热要关掉（否则连接与片尾缓存泄漏）。
     *
     * 【不会误关玩家页正在用的源】玩家页接手时 [take] 已经把它从登记处摘走了。
     */
    fun discard(url: String) {
        val entry = synchronized(lock) { entries.remove(url) } ?: return
        runCatching { entry.source.close() }
    }
}
