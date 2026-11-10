package com.kivo.cinema.smb

import java.util.concurrent.atomic.AtomicLong

/**
 * 读取画像（250 D4）。只记录，不决定任何事。
 *
 * 为什么必须分段看：解码侧只报"每帧读用了 14.5 ms"是平均值。而块窗口注定是
 * "命中很多次、然后阻塞填一次"的锯齿 —— 平均值 14.5 ms 完全可以是
 * "每 12 帧卡 174 ms"摊出来的。用户看到的是周期性顿一下，平均值把这件事藏得干干净净。
 *
 * 所以这里同时报**命中率**与**单次填充的最长耗时**，锯齿才会在数据里现形：
 * 命中率低（<90%）= 窗口没覆盖住顺序读；fill_max 远大于 fill_avg = 网络有长尾。
 *
 * 上报函数由调用方注入（不在本类里直接调 `android.util.Log`），
 * 这样它在 JVM 单测里也能实例化。
 *
 * ── 252：计数改成原子 ─────────────────────────────────────────────────────
 *
 * 命中路径挪到网络读之外之后，视频线程与音频线程会**并发**调用这里的计数函数。
 * 原来全是普通 `Long`/`Double` 字段，并发自增会丢计数，`fillMaxMs` 这种
 * double 还可能被写坏 —— 而这份画像正是判断"卡顿是网络还是解码"的唯一依据，
 * 数字不可信就等于没有。所以全部换成原子量。
 */
internal class SmbReadStats(
    private val report: (String) -> Unit,
    private val nowMs: () -> Long = { System.nanoTime() / 1_000_000 },
) {
    private val reads = AtomicLong()
    private val hits = AtomicLong()
    private val prefetchHits = AtomicLong()
    private val fills = AtomicLong()
    private val fillMicros = AtomicLong()
    private val fillMaxMicros = AtomicLong()
    private var missFirstBlock = -1L
    private var missLastBlock = -1L
    private var prevMissBlock = -1L
    private var missJumpMax = 0L
    private var nextReportAtMs = 0L

    /** 累计从网络拿到的字节。**不随上报清零** —— 它回答的是"整段播放一共吃了多少带宽"。 */
    private val bytesTotal = AtomicLong()

    /** 当前累计字节。用于对照"解码侧需要多少"，判断卡顿是网络供不上还是解码跟不上。 */
    val deliveredBytes: Long get() = bytesTotal.get()

    fun onRead() {
        reads.incrementAndGet()
    }

    fun onDeliver(bytes: Int) {
        if (bytes > 0) bytesTotal.addAndGet(bytes.toLong())
    }

    fun onHit(fromPrefetch: Boolean) {
        hits.incrementAndGet()
        if (fromPrefetch) prefetchHits.incrementAndGet()
    }

    fun onFill(elapsedMs: Double) {
        fills.incrementAndGet()
        val micros = (elapsedMs * 1000.0).toLong()
        fillMicros.addAndGet(micros)
        // 只增不减的最大值：并发写用 CAS 循环，避免"后写的更小值把大值盖掉"。
        var current = fillMaxMicros.get()
        while (micros > current && !fillMaxMicros.compareAndSet(current, micros)) {
            current = fillMaxMicros.get()
        }
    }

    /**
     * 未命中的块号。单调推进 = 顺序访问（预读本该覆盖）；
     * 在少数几个块号之间来回跳 = 按轨交错（需要多块常驻，加大预读没用）。
     */
    fun onMissBlock(index: Long) {
        if (missFirstBlock < 0) missFirstBlock = index
        missLastBlock = index
        if (prevMissBlock >= 0) {
            val jump = if (index > prevMissBlock) index - prevMissBlock else prevMissBlock - index
            if (jump > missJumpMax) missJumpMax = jump
        }
        prevMissBlock = index
    }

    /** 每 4 秒吐一行。报的是"刚过去的 4 秒"，不是开机以来的累计（累计会把偶发尖峰抹平）。 */
    fun maybeReport() {
        val now = nowMs()
        if (nextReportAtMs == 0L) {
            nextReportAtMs = now + INTERVAL_MS
            return
        }
        if (now < nextReportAtMs) return
        nextReportAtMs = now + INTERVAL_MS

        val readCount = reads.getAndSet(0L)
        val hitCount = hits.getAndSet(0L)
        val prefetchCount = prefetchHits.getAndSet(0L)
        val fillCount = fills.getAndSet(0L)
        val fillTotal = fillMicros.getAndSet(0L)
        val fillMax = fillMaxMicros.getAndSet(0L)

        val percent = if (readCount > 0) hitCount * 100.0 / readCount else 0.0
        val average = if (fillCount > 0) fillTotal / 1000.0 / fillCount else 0.0
        report(
            "stats reads=%d hits=%d (%.0f%%) prefetch=%d fills=%d fill_avg=%.1fms fill_max=%.1fms miss_blk=[%d..%d] jump_max=%d"
                .format(readCount, hitCount, percent, prefetchCount, fillCount, average,
                    fillMax / 1000.0, missFirstBlock, missLastBlock, missJumpMax),
        )

        missFirstBlock = -1L
        missLastBlock = -1L
        prevMissBlock = -1L
        missJumpMax = 0L
    }

    companion object {
        /** 4 秒 ≈ 96 帧，足够看出锯齿，又不会把日志刷爆。 */
        const val INTERVAL_MS = 4_000L
    }
}
