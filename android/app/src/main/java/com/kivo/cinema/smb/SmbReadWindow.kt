package com.kivo.cinema.smb

/**
 * 按块对齐的 LRU 读取窗口（250 D4）。
 *
 * 为什么是"按块对齐 + 多块"而不是"一个大窗口"：硬解路径的 MediaExtractor 是**按轨**在
 * 文件里来回跳的（视频块 → 音频块 → 字幕块 → 回到视频）。单窗口下每次换轨都把上一块
 * 整个顶掉，于是永远不命中 —— 这在 247 之前实测是 27 次读 / 27 次未命中 / 命中率 0%。
 * 多块常驻让四条轨各自占一格，剩下两格做机动。
 *
 * 两条与正确性有关的不变量：
 * 1. **块边界必须对齐**，否则每次请求起点都不同，填出来的区间彼此错开，缓存永远对不上；
 * 2. **填充失败要先把槽位标废**。原实现是填完才写 `start`，中途抛异常时槽位仍指着
 *    旧的偏移、内容却已经被写了一半 —— 下一次"命中"会返回错位的数据（花屏级）。
 *    这里进入填充前就置 -1，失败后该槽不可能被误命中。
 *
 * 只负责"记住哪一块在内存里"，不负责"怎么读到"——数据来自调用方注入的 [read]。
 */
internal class SmbReadWindow(private val tuning: SmbReadTuning) {

    private val slots = arrayOfNulls<ByteArray>(tuning.blockCount)
    private val starts = LongArray(tuning.blockCount) { -1L }
    private val lengths = IntArray(tuning.blockCount)
    private val used = LongArray(tuning.blockCount)
    private var clock = 0L

    /** 请求是否**完整**落在某一块已填充的区间内。部分命中不算：拼两次网络读更贵。 */
    fun copyIfPresent(offset: Long, buf: ByteArray, off: Int, want: Int): Boolean {
        val aligned = align(offset)
        for (index in slots.indices) {
            if (starts[index] != aligned) continue
            val length = lengths[index]
            val relative = (offset - aligned).toInt()
            if (relative < 0 || relative + want > length) return false
            val data = slots[index] ?: return false
            System.arraycopy(data, relative, buf, off, want)
            used[index] = ++clock
            return true
        }
        return false
    }

    /**
     * 把 [offset] 所在的整块读进缓存，返回该块结束偏移；失败返回 -1。
     *
     * [readFully] 与 `SmbPreadChannel.readFully` 同签名：尽量读满，返回实际字节数，
     * -1 表示一个字节都没读到。短读（返回 < 请求长度）照收 —— 那说明到文件尾了。
     */
    fun fill(offset: Long, sizeBytes: Long, readFully: (Long, ByteArray, Int, Int) -> Int): Long {
        val aligned = align(offset)
        val length = minOf(tuning.blockBytes.toLong(), sizeBytes - aligned).toInt()
        if (length <= 0) return -1L

        // 该块已完整驻留就直接返回：**跨块边界的那次请求**会带着仍落在本块的 offset 走到这里，
        // 若照旧填一遍，就是把刚读进来的整块又读一次。真机实测（251）24 MiB 顺序读因此
        // 实际拉了 50 MiB —— 一倍的带宽白烧，吞吐从 7.8 掉到 4.0 MB/s。
        residentEnd(aligned, sizeBytes, length)?.let { return it }

        val slot = pickSlot(aligned)
        val data = slots[slot] ?: ByteArray(tuning.blockBytes).also { slots[slot] = it }
        // 先标废：中途抛异常时，这个槽位不能继续以旧偏移的身份被命中。
        starts[slot] = -1L
        lengths[slot] = 0

        val done = readFully(aligned, data, 0, length)
        if (done <= 0) return -1L

        starts[slot] = aligned
        lengths[slot] = done
        used[slot] = ++clock
        return aligned + done
    }

    /** 连接换过之后，旧数据一律不可信。 */
    fun invalidate() {
        for (index in slots.indices) {
            starts[index] = -1L
            lengths[index] = 0
        }
    }

    /**
     * 若 [aligned] 这一块已经**完整**驻留，返回它的结束偏移；否则 null。
     *
     * "完整"只认两种：读满了整块，或读到了文件尾（尾部块本来就短）。
     * 中途短读（传输异常）留下的半块不算 —— 那种情形必须重读，否则会拿残数据当有效数据。
     */
    private fun residentEnd(aligned: Long, sizeBytes: Long, want: Int): Long? {
        for (index in slots.indices) {
            if (starts[index] != aligned) continue
            val length = lengths[index]
            if (length < want && aligned + length < sizeBytes) return null
            used[index] = ++clock
            return aligned + length
        }
        return null
    }

    /** 块内偏移换算：返回该偏移所属块的起始位置。 */
    fun align(offset: Long): Long = offset - (offset % tuning.blockBytes)

    /** 块序号，供诊断统计判断"访问是顺序推进还是按轨跳跃"。 */
    fun blockIndex(offset: Long): Long = align(offset) / tuning.blockBytes

    private fun pickSlot(aligned: Long): Int {
        for (index in slots.indices) if (starts[index] == aligned) return index
        for (index in slots.indices) if (starts[index] < 0L) return index
        var oldest = 0
        for (index in slots.indices) if (used[index] < used[oldest]) oldest = index
        return oldest
    }
}
