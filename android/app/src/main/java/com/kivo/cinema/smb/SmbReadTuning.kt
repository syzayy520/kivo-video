package com.kivo.cinema.smb

/**
 * 一次播放使用的 SMB 读取参数（250 D6）。
 *
 * 为什么需要这个类：这些数字原本是 [SmbByteSource] 里的五个 `private const`
 * （`BLOCK_BYTES` / `BLOCK_COUNT` / `PF_CAPACITY` / `PF_CHUNK` / `PF_WAIT_NS`），
 * 全部写死，短片与 20 GB 原盘吃同一份预算 —— 探测一次文件头也要先付 12 MiB。
 *
 * 这里只做一件事：**给定文件大小，算出一组自洽的参数**。不含读取逻辑、
 * 不碰 Android、不碰 libsmb2，所以可以 JVM 单测（反馈是毫秒级，不是插设备 40 秒）。
 *
 * 默认档逐位等于 247 实测通过的那组值（6 × 2 MiB + 6 MiB + 350 ms），
 * 不因为"看起来更大更好"去动它：`Shelter` 的 2×TrueHD + PGS 需要四条轨常驻，
 * 6 块是有证据的下限，不是拍脑袋的上限。
 */
data class SmbReadTuning(
    /** 单个缓存块的字节数。块必须按它对齐，否则每次请求起点都不同，缓存永远对不上。 */
    val blockBytes: Int = DEFAULT_BLOCK_BYTES,
    /** 常驻块数。决定"能同时记住几个读写位置"。 */
    val blockCount: Int = DEFAULT_BLOCK_COUNT,
    /** 预读环容量。按"要盖住一次网络读的最坏抖动"推导。 */
    val aheadBytes: Int = DEFAULT_AHEAD_BYTES,
    /** 单次预读请求的字节数。太小被往返次数拖住，太大挤占主读。 */
    val aheadChunk: Int = DEFAULT_AHEAD_CHUNK,
    /** 顺序播放时允许等预读多久。只覆盖"预读已经在路上、就差这一下"。 */
    val aheadWaitNs: Long = DEFAULT_AHEAD_WAIT_NS,
) {
    /** 块窗口占用的字节数（不含预读环）。 */
    val windowBytes: Long get() = blockBytes.toLong() * blockCount

    /** 单路读取的常驻内存上限 = 块窗口 + 预读环。 */
    val residentBytes: Long get() = windowBytes + aheadBytes

    /**
     * 把参数夹回可工作的区间。
     *
     * 三条不变量（违反任何一条都会让读取退化或直接死锁）：
     * 1. 块不小于 1 MiB —— 更小的块让 SMB 往返次数把带宽吃光；
     * 2. 预读环至少能放两个 chunk —— 只有一个 chunk 时，消费者取走一部分后
     *    写指针立刻追尾读指针，预读线程会在"填满 → 立刻满"之间空转；
     * 3. 等待上限不低于 100 ms —— 低于这个值等于没等，预读永远来不及。
     */
    fun clamped(): SmbReadTuning {
        val block = blockBytes.coerceIn(MIN_BLOCK_BYTES, MAX_BLOCK_BYTES)
        val chunk = aheadChunk.coerceIn(MIN_AHEAD_CHUNK, minOf(MAX_AHEAD_CHUNK, block))
        val ahead = aheadBytes
            .coerceIn(MIN_AHEAD_BYTES, MAX_AHEAD_BYTES)
            .coerceAtLeast(chunk * 2)
            .coerceAtMost(MAX_AHEAD_BYTES)
        return copy(
            blockBytes = block,
            blockCount = blockCount.coerceIn(MIN_BLOCK_COUNT, MAX_BLOCK_COUNT),
            aheadBytes = ahead,
            aheadChunk = chunk,
            aheadWaitNs = aheadWaitNs.coerceIn(MIN_AHEAD_WAIT_NS, MAX_AHEAD_WAIT_NS),
        )
    }

    companion object {
        const val DEFAULT_BLOCK_BYTES = 2 shl 20
        const val DEFAULT_BLOCK_COUNT = 6
        const val DEFAULT_AHEAD_BYTES = 6 shl 20
        const val DEFAULT_AHEAD_CHUNK = 1 shl 20
        const val DEFAULT_AHEAD_WAIT_NS = 350_000_000L

        const val MIN_BLOCK_BYTES = 1 shl 20
        const val MAX_BLOCK_BYTES = 2 shl 20
        const val MIN_BLOCK_COUNT = 2
        const val MAX_BLOCK_COUNT = 6
        const val MIN_AHEAD_BYTES = 2 shl 20
        const val MAX_AHEAD_BYTES = 6 shl 20
        const val MIN_AHEAD_CHUNK = 256 shl 10
        const val MAX_AHEAD_CHUNK = 1 shl 20
        const val MIN_AHEAD_WAIT_NS = 100_000_000L
        const val MAX_AHEAD_WAIT_NS = 1_000_000_000L

        /** 分档阈值：256 MiB 以下只留 2 块（短片/剧集），2 GiB 以下留 4 块（单层 MKV）。 */
        const val SMALL_FILE_BYTES = 256L shl 20
        const val MEDIUM_FILE_BYTES = 2L shl 30

        /** 247 实测档：大文件走这组，逐位不变。 */
        val default = SmbReadTuning()

        /**
         * 按文件大小推导。大小未知（<= 0）时给默认档 ——
         * 宁可多占几 MiB，也不要因为一个错误的估计把窗口压到不可用。
         */
        fun forFile(sizeBytes: Long): SmbReadTuning {
            if (sizeBytes <= 0L) return default
            val tier = when {
                sizeBytes < SMALL_FILE_BYTES -> default.copy(blockCount = 2, aheadBytes = 2 shl 20)
                sizeBytes < MEDIUM_FILE_BYTES -> default.copy(blockCount = 4, aheadBytes = 4 shl 20)
                else -> default
            }
            return tier.clamped()
        }
    }
}
