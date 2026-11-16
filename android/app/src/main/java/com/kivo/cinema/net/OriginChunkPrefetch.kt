package com.kivo.cinema.net

import java.util.concurrent.Executors

/**
 * 293：**下一块的有界短时前瞻** —— 把「每块必付的表头空窗」重叠掉。
 *
 * ## 为什么需要它（292 真机定因，见 279 §13.3）
 *
 * 主链路是**串行补块**：[OriginConnectionLease] 读完一块 → 归还连接 →
 * 生产循环再 `open()` 下一块，而 `open()` 要等源站**响应头**（本地装置
 * 精确注入 3.6 s，公网 1.5–8 s）。这段窗口 `rate=0.0M/s`、环缓冲不增长，
 * 一次空窗吃掉约 3.6 s runway ⇒ 起播期 runway 只够扛一个空窗，
 * 连第二个就穿仓（292 w4：+18.0 s `underrun=true`）。
 *
 * ## 它怎么做（★ 不是叠加带宽，是重叠等待）
 *
 * 当前块**还剩 [leadBytes]** 时，后台线程**提前**发出下一块的 Range 请求，
 * 只等响应头、**不读 body**。等当前块读满、生产循环要开下一块时，
 * 直接把这条已就绪的流**提升**为当前块 —— 表头等待与当前块的最后一段
 * 传输、以及生产循环里的空转重叠掉了。
 *
 * 【为什么说不是叠加带宽】预开请求在等响应头时源站还没开始吐字节
 * （装置是 `setTimeout(fire, hdrMs)` 才发头）；等它开始吐时，当前块恰好
 * 读完 ⇒ 两条流在**时间上错开**，不是并发抢带宽。这正回应 275 §5.2
 * 「并发预读须证明源站允许叠加带宽」的顾虑。
 *
 * 【驻留字节怎么限】[HttpOriginClient.OriginStream.readInto] 没有内部预读，
 * 预开不读 body ⇒ 数据压在 socket 接收缓冲区，TCP window 关闭即反压源站
 * ⇒ 驻留量被内核自限（一个块以内），不会像 carry 那样额外吃一块内存。
 *
 * ## 红线（每一条都对应一次潜在事故）
 *
 * · **跳读 / 让路 / 丢流 / 关闭必须 [invalidate]**：否则预开的旧位置字节
 *   会被当成新位置的数据（顺序全乱）。用 `epoch` 计数让在飞任务的结果作废。
 * · **总连接数 ≤ 2**（当前块 + 预开块），不预读第三块。
 * · **起点必须精确匹配**才复用：[obtain] 只接受 `start == target` 的 pending。
 * · **不许静默**：启动 / 就绪 / 作废 / 失败 / 等待超时全部留 `[prefetch]` 日志。
 *
 * ## 开关（opt-in，默认关闭）
 *
 * `debug.kivo.chunkPrefetch=on` 才启用；缺省 / `off` ⇒ 一次都不发（旧路径一字不变）。
 * `debug.kivo.prefetchLeadMb` 覆盖提前量（默认 2 MiB ≈ 半块）。
 */
internal class OriginChunkPrefetch(
    private val client: HttpOriginClient,
    private val label: String = "play",
) {
    /** opt-in 开关：`debug.kivo.chunkPrefetch=on` 才启用。 */
    private val enabled: Boolean =
        prop(PROPERTY)?.trim()?.lowercase() == "on"

    /**
     * 提前量：剩这么多字节时开始预开下一块。
     *
     * 封顶为**一整块**（纯防御：配置值超过块长时，`remaining <= lead` 会恒成立、
     * 每轮循环都试图预开）。对默认值无影响：play 源 4 MiB 块 ⇒ 2 MiB（半块），
     * thumb 源 1 MiB 块 ⇒ 1 MiB（= 开块即预开，2 块窗口，预览可接受）。
     */
    private val leadBytes: Long = minOf(
        (prop(LEAD_PROPERTY)?.trim()?.toIntOrNull()?.takeIf { it in 1..4 } ?: DEFAULT_LEAD_MB) *
            1024L * 1024L,
        client.chunkBytes,
    )

    private val lock = Object()

    /** 已就绪、等被提升的下一块（null = 没有）。 */
    private var pending: HttpOriginClient.OriginStream? = null

    /** 本次预开的请求起点（-1 = 没有预开）。 */
    private var expectedStart = -1L

    /** 预开请求是否在路上（响应头还没回来）。 */
    private var inFlight = false

    private var closed = false

    /** 换流代数：任何 [invalidate] 都 +1 ⇒ 在飞任务的结果自动作废。 */
    private var epoch = 0

    private val worker = Executors.newSingleThreadExecutor { r ->
        Thread(r, "KivoChunkPrefetch").apply { isDaemon = true }
    }

    init {
        if (enabled) {
            NetDiag.log(
                "[$label] prefetch 启用：lead=${leadBytes / 1048576}MiB awaitMax=${AWAIT_MS}ms",
            )
        } else {
            NetDiag.log("[$label] prefetch 默认关闭（opt-in）：不预开任何块")
        }
    }

    /**
     * 当前块剩余 [remaining] 字节时尝试预开下一块（**非阻塞**，每次循环都可调）。
     *
     * @param nextStart 下一块起点（= 当前块的 `endExclusive`）。
     */
    fun maybeStart(nextStart: Long, remaining: Long) {
        if (!enabled || remaining <= 0 || remaining > leadBytes) return
        var myEpoch = 0
        synchronized(lock) {
            if (closed || inFlight) return
            val stale = pending
            if (stale != null) {
                if (expectedStart == nextStart) return // 已经预开到同一位置
                // 起点变了（换过流）：旧 pending 作废
                pending = null
                expectedStart = -1L
                runCatching { stale.abort() }
            }
            inFlight = true
            expectedStart = nextStart
            myEpoch = ++epoch
        }
        NetDiag.log("[$label] prefetch 启动 @$nextStart（当前块剩 ${remaining / 1024}KiB）")
        worker.execute { runPrefetch(nextStart, myEpoch) }
    }

    /**
     * 取走已就绪的下一块；起点必须精确匹配 [target]。
     *
     * 若预开还在路上且起点匹配，**有界等待**（最多 [AWAIT_MS]）——这段等待
     * 正是「本来就要付的表头时间」，等到了就省掉一次重开。
     *
     * @return 可用的流（起点已校验）；null = 没有可复用的，调用方自行开块。
     */
    fun obtain(target: Long): HttpOriginClient.OriginStream? {
        if (!enabled) return null
        synchronized(lock) {
            if (closed) return null
            val ready = pending
            if (ready != null) {
                pending = null
                expectedStart = -1L
                if (ready.start == target) return ready
                // 起点不符（源站规范化过 / 换过流）：宁可丢掉也不能给错字节。
                runCatching { ready.abort() }
                NetDiag.log("[$label] prefetch 丢弃：起点 ${ready.start} != 期望 $target")
            }
            if (!inFlight || expectedStart != target) return null
            val deadline = System.currentTimeMillis() + AWAIT_MS
            while (inFlight && !closed) {
                val remain = deadline - System.currentTimeMillis()
                if (remain <= 0) break
                lock.wait(remain)
                val arrived = pending
                if (arrived != null) {
                    pending = null
                    expectedStart = -1L
                    if (arrived.start == target) return arrived
                    runCatching { arrived.abort() }
                    break
                }
            }
            if (inFlight && System.currentTimeMillis() >= deadline) {
                NetDiag.log("[$label] prefetch 等待超时 @$target（>${AWAIT_MS}ms），主链路自行重开")
            }
            return null
        }
    }

    /** 换流 / 关闭：作废 pending 与在飞任务（[reason] 只进日志）。 */
    fun invalidate(reason: String) {
        var stale: HttpOriginClient.OriginStream? = null
        synchronized(lock) {
            stale = pending
            pending = null
            expectedStart = -1L
            inFlight = false
            epoch += 1
            lock.notifyAll()
        }
        if (stale != null) {
            runCatching { stale.abort() }
            NetDiag.log("[$label] prefetch 作废 pending（$reason）")
        }
    }

    fun close() {
        synchronized(lock) {
            closed = true
            lock.notifyAll()
        }
        invalidate("关闭")
        worker.shutdownNow()
    }

    /** 后台：只等响应头，不读 body（body 留在 socket，靠背压自限）。 */
    private fun runPrefetch(start: Long, myEpoch: Int) {
        var opened: HttpOriginClient.OriginStream? = null
        var why: String? = null
        try {
            opened = client.openChunk(start)
        } catch (e: Exception) {
            why = e.message ?: e.javaClass.simpleName
        }
        var stale = false
        var ready = false
        synchronized(lock) {
            if (myEpoch != epoch || closed) {
                stale = true
            } else {
                inFlight = false
                if (opened != null) {
                    pending = opened
                    ready = true
                } else {
                    expectedStart = -1L
                }
            }
            lock.notifyAll()
        }
        when {
            stale -> {
                runCatching { opened?.abort() }
                NetDiag.log("[$label] prefetch 作废 @$start（期间换流/关闭）")
            }
            ready -> NetDiag.log("[$label] prefetch 就绪 @$start")
            else -> NetDiag.log("[$label] prefetch 失败 @$start: $why（主链路将自行重开）")
        }
    }

    private fun prop(name: String): String? = try {
        Class.forName("android.os.SystemProperties")
            .getMethod("get", String::class.java).invoke(null, name) as? String
    } catch (e: Exception) {
        null
    }

    companion object {
        private const val PROPERTY = "debug.kivo.chunkPrefetch"
        private const val LEAD_PROPERTY = "debug.kivo.prefetchLeadMb"

        /** 等预开就绪的上限：覆盖公网最慢表头，超时就让主链路自己重开。 */
        private const val AWAIT_MS = 8_000L

        /** 默认提前量：半块（4 MiB / 2）≈ 3.4 s @614 KB/s，够叠一个 3.6 s 表头。 */
        private const val DEFAULT_LEAD_MB = 2
    }
}
