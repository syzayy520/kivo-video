package com.kivo.cinema.smb

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
 */
internal class SmbReadStats(
    private val report: (String) -> Unit,
    private val nowMs: () -> Long = { System.nanoTime() / 1_000_000 },
) {
    private var reads = 0L
    private var hits = 0L
    private var prefetchHits = 0L
    private var fills = 0L
    private var fillMs = 0.0
    private var fillMaxMs = 0.0
    private var missFirstBlock = -1L
    private var missLastBlock = -1L
    private var prevMissBlock = -1L
    private var missJumpMax = 0L
    private var nextReportAtMs = 0L

    fun onRead() {
        ++reads
    }

    fun onHit(fromPrefetch: Boolean) {
        ++hits
        if (fromPrefetch) ++prefetchHits
    }

    fun onFill(elapsedMs: Double) {
        ++fills
        fillMs += elapsedMs
        if (elapsedMs > fillMaxMs) fillMaxMs = elapsedMs
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

        val percent = if (reads > 0) hits * 100.0 / reads else 0.0
        val average = if (fills > 0) fillMs / fills else 0.0
        report(
            "stats reads=%d hits=%d (%.0f%%) prefetch=%d fills=%d fill_avg=%.1fms fill_max=%.1fms miss_blk=[%d..%d] jump_max=%d"
                .format(reads, hits, percent, prefetchHits, fills, average, fillMaxMs,
                    missFirstBlock, missLastBlock, missJumpMax),
        )

        reads = 0L
        hits = 0L
        prefetchHits = 0L
        fills = 0L
        fillMs = 0.0
        fillMaxMs = 0.0
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
