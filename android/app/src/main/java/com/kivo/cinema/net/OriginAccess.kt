package com.kivo.cinema.net

/**
 * 273 v26：**总长与访问校验** —— 源站报出来的片源长度，以及"源站有没有明确拒绝"。
 *
 * ## 为什么从 [HttpOriginSource] 里拆出来
 * "片源多长 / 能不能访问"与"怎么读 / 怎么预取"是**两种变化原因**：
 * 改预取编排不该动总长的来源，反之亦然。
 *
 * ## 总长的三个来源（273 v15）
 *  · [learnedSize]：源站**实际响应**报出来的（`Content-Range` 的 `/total`）—— **真值**，
 *    代理写响应头只用它。
 *  · 外给的提示（Emby 详情里的 `MediaSources[0].Size`）—— 只用来让片尾预取
 *    **在 t=0 就发出去**（尾段在源站是冷的，晚发就晚到）。
 *  · 探测：两者都没有时的一次 1 字节范围请求（≈300 ms 往返）—— 兜底。
 */
internal class OriginAccess(
    url: String,
    token: String?,
    label: String,
    chunkBytes: Long,
) {

    /**
     * 源站 client（连接、分块、凭据都在它里面）。
     *
     * 【为什么回调写在这里】任何一次响应报出总长时都要立刻记下来 ——
     * 那一次响应就是最省的一跳（不额外往返）。
     */
    val client = HttpOriginClient(url, token, label, chunkBytes) { learnedSize = it }

    /** 源站报出来的总长（-1 = 还不知道）。这是**真值**。 */
    @Volatile var learnedSize: Long = -1L

    /** 源站报出来的总长（-1 = 还不知道）。 */
    fun knownSize(): Long = learnedSize

    /**
     * 预取片尾窗口要用的总长：**外给的提示优先**。
     *
     * 【为什么提示优先】预取必须在 t=0 就发出去 —— 尾段在源站是**冷的**
     * （真机 273 v13/v14：0.34 MB/s，比片头慢一个量级），
     * 晚发 1 ms 就晚发 1 ms 到。而尾窗口的起点 = 总长 - 窗口大小，
     * 所以总长必须**当场**有：Emby 详情里就有（零额外请求）。
     */
    fun prefetchTotal(sizeHint: Long): Long = if (sizeHint > 0) sizeHint else learnedSize

    /**
     * 片源总长（还不知道就探一次）。
     *
     * 【探测为什么只做一次】并发调用共用这一个探测 ——
     * 起播路径上好几个地方都要总长，各探一次就是几个 RTT。
     */
    fun size(): Long {
        learnedSize.takeIf { it > 0 }?.let { return it }
        synchronized(this) {
            learnedSize.takeIf { it > 0 }?.let { return it }
            val probed = client.totalSize()
            learnedSize = probed
            return probed
        }
    }

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
    fun accessVerdict(): Throwable? = try {
        client.totalSize()
        null
    } catch (e: IllegalStateException) {
        e
    } catch (e: Exception) {
        null
    }
}
