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
 *
 * ── 252：并发安全与"填充不挡命中" ────────────────────────────────────────
 *
 * 真机实测（Shelter，12.19 GB / 2×TrueHD + PGS）出现过**偶发杂音**，
 * 证据是 `network-native-sync.json`：
 *
 * ```
 *   output.frames_silenced        = 320     ← 音频环被抽干，补了 6.7 ms 静音
 *   clock.tail.long_gaps          = 3       ← 整条管线停了 3 次
 *   clock.tail.max_mono_step_ns   = 320177605   ← 最长一次停 320 ms
 *   ring_capacity_frames          = 16384  ← 音频环只有 341 ms
 * ```
 *
 * 根因是**两个消费者共用一条读路径**：视频线程与音频线程各开一个 fd，
 * 都走到这个窗口；而整个 `readAt` 编排串在同一把锁上。于是：
 *
 *  · 视频线程在填一块 1 MiB（这台 NAS 的 `max_read_size` 就是 1 MiB，≈120 ms）时，
 *    **连"缓存命中"的拷贝都要排队等它读完** —— 音频要的块明明已经在内存里；
 *  · 一次停顿 320 ms > 音频环可用余量（341 ms − 钟延迟 149 ms ≈ 190 ms），
 *    环被抽干 → 补静音 → 听得见的杂音。
 *
 * 所以这里做两件事：
 *  1. **窗口自带锁**（原来完全没有）：视频/音频并发挑槽位时可能挑到同一块、
 *     各自往同一个 ByteArray 里写 → 交错字节 = 花屏/杂音级损坏；
 *  2. **网络读放在锁外**：填充期间别人仍可命中其它块、照常拷贝内存。
 *     锁只保护记账（槽位表），不保护"等网络"。
 */
internal class SmbReadWindow(private val tuning: SmbReadTuning) {

    /**
     * 窗口自己的锁。**只保护记账，不覆盖网络读**。
     *
     * 原来这个类完全没有同步 —— 两个线程可能挑到同一个槽位、各自往同一块
     * ByteArray 里写，交错字节 = 花屏级损坏。252 起补上。
     */
    private val lock = Any()

    private val slots = arrayOfNulls<ByteArray>(tuning.blockCount)
    private val starts = LongArray(tuning.blockCount) { -1L }
    private val lengths = IntArray(tuning.blockCount)
    private val used = LongArray(tuning.blockCount)

    /** 正在被填充的槽位：填充期间不许被别人挑走（否则两块写同一个缓冲）。 */
    private val reserved = BooleanArray(tuning.blockCount)
    private var clock = 0L

    /** 请求是否**完整**落在某一块已填充的区间内。部分命中不算：拼两次网络读更贵。 */
    fun copyIfPresent(offset: Long, buf: ByteArray, off: Int, want: Int): Boolean =
        synchronized(lock) {
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
            false
        }

    /**
     * 把 [offset] 所在的整块读进缓存，返回该块结束偏移；失败返回 -1。
     *
     * [readFully] 与 `SmbPreadChannel.readFully` 同签名：尽量读满，返回实际字节数，
     * -1 表示一个字节都没读到。短读（返回 < 请求长度）照收 —— 那说明到文件尾了。
     *
     * **网络读在窗口锁之外**：填充一块 1 MiB 要 ~120 ms，这段时间里别的消费者
     * 仍应能命中其它块并立刻拷走（252：否则音频要排队等网络，环被抽干 → 杂音）。
     */
    fun fill(offset: Long, sizeBytes: Long, readFully: (Long, ByteArray, Int, Int) -> Int): Long {
        val aligned = align(offset)
        val length = minOf(tuning.blockBytes.toLong(), sizeBytes - aligned).toInt()
        if (length <= 0) return -1L

        val slot: Int
        val data: ByteArray
        synchronized(lock) {
            // 该块已完整驻留就直接返回：**跨块边界的那次请求**会带着仍落在本块的 offset 走到这里，
            // 若照旧填一遍，就是把刚读进来的整块又读一次。真机实测（251）24 MiB 顺序读因此
            // 实际拉了 50 MiB —— 一倍的带宽白烧，吞吐从 7.8 掉到 4.0 MB/s。
            residentEnd(aligned, sizeBytes, length)?.let { return it }

            slot = pickSlot(aligned)
            reserved[slot] = true
            // 先标废：中途抛异常时，这个槽位不能继续以旧偏移的身份被命中。
            starts[slot] = -1L
            lengths[slot] = 0
            data = slots[slot] ?: ByteArray(tuning.blockBytes).also { slots[slot] = it }
        }

        // ── 网络读在锁外 ──
        val done = try {
            readFully(aligned, data, 0, length)
        } catch (error: Throwable) {
            synchronized(lock) { reserved[slot] = false }
            throw error
        }

        synchronized(lock) {
            reserved[slot] = false
            if (done > 0) {
                starts[slot] = aligned
                lengths[slot] = done
                used[slot] = ++clock
            }
        }
        if (done <= 0) return -1L
        return aligned + done
    }

    /** 连接换过之后，旧数据一律不可信。 */
    fun invalidate() {
        synchronized(lock) {
            for (index in slots.indices) {
                starts[index] = -1L
                lengths[index] = 0
            }
        }
    }

    /**
     * 若 [aligned] 这一块已经**完整**驻留，返回它的结束偏移；否则 null。
     *
     * "完整"只认两种：读满了整块，或读到了文件尾（尾部块本来就短）。
     * 中途短读（传输异常）留下的半块不算 —— 那种情形必须重读，否则会拿残数据当有效数据。
     *
     * 调用方必须已持有 [lock]（它会写 `used`）。
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

    /** 调用方必须已持有 [lock]。 */
    private fun pickSlot(aligned: Long): Int {
        for (index in slots.indices) if (starts[index] == aligned && !reserved[index]) return index
        for (index in slots.indices) if (starts[index] < 0L && !reserved[index]) return index
        var oldest = -1
        for (index in slots.indices) {
            if (reserved[index]) continue
            if (oldest < 0 || used[index] < used[oldest]) oldest = index
        }
        // 全部槽位都在填充中：并发填充数超过块数才会发生（当前只有两个消费者）。
        // 退化为 0 号槽，至少不会越界。
        return if (oldest >= 0) oldest else 0
    }
}
