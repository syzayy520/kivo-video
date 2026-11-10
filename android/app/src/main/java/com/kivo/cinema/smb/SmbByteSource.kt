package com.kivo.cinema.smb

import com.kivo.cinema.net.ByteSource

/**
 * SMB 随机读源（250 D4）。**只编排顺序，不实现任何一件事。**
 *
 * 拆分前这个类 664 行、6 个职责（窗口 / 预读 / 重连 / 统计 / 契约 / 常量）挤在一起，
 * 改任何一处都在动同一个文件。现在它按 `readAt` 的实际决策顺序把工作分给五个单元：
 *
 * ```
 *   顺序读？ ──预读缓冲──► 命中直接拷内存
 *            └─块窗口───► 命中直接拷内存
 *            └─等预读───► 顺序落后时等它追上，绝不自己另开同步读
 *            └─填窗口───► 一次网络读，喂满一整块
 *            └─直读─────► 跨块边界 / 大请求
 * ```
 *
 * 三条不能丢的读取纪律（都来自真机事故）：
 * 1. **只有真到文件尾才返回 -1**。未到尾部却读到 0 是传输异常，抛 [IOException]。
 *    伪装成 EOF 会让播放器认为"片源结束"，画面静默停住。
 * 2. **顺序播放时只等预读，不并发第二条读**。两条连接同时拉同一台服务器会互相拖慢，
 *    实测 1 MiB 单次从 92 ms 恶化到 543 ms（6 倍）。抢着读只会越读越慢。
 * 3. **读写与 close 共用一把锁**（[lock]）：解码线程在反复读，用户可能同时退出。
 *
 * 线程模型：`readAt` 可能被多个 fd 回调线程调用（探针 + 解码器各开一个 fd），
 * 而 libsmb2 的 context 不支持并发，所以 [lock] 把整条编排串起来。
 */
class SmbByteSource private constructor(
    address: SmbAddress,
    credential: SmbCredential,
    filePath: String,
    reader: SmbReader,
    private val tuning: SmbReadTuning,
    online: () -> Boolean,
) : ByteSource {

    private val sizeBytes: Long = reader.size
    private val lock = Any()
    private val gate = SmbRetryGate(online = online)
    private val window = SmbReadWindow(tuning)
    private val stats = SmbReadStats(report = { android.util.Log.i("KivoSmbRead", it) })

    /** 重连成功后旧数据一律不可信。lambda 在调用时求值，所以可以引用后面声明的字段。 */
    private val channel = SmbPreadChannel(
        address, credential, filePath, gate, reader,
        onReconnected = { window.invalidate() },
        onBytes = { stats.onDeliver(it) },
    )

    private val ahead = SmbReadAhead(
        tuning = tuning,
        sizeBytes = sizeBytes,
        openReader = { SmbReader(address, credential, filePath) },
        log = { android.util.Log.w("KivoSmbRead", it) },
    )

    /**
     * 已关闭。关闭后 fd 回调可能还会进来一两次，要当 EOF 处理而不是抛"尚未打开"。
     * 预读线程在锁外读它，所以必须是 volatile —— 否则关了它还照样往缓冲里灌。
     */
    @Volatile private var closed = false

    /** 上一次填充的块号：用来判断访问是"顺序推进"还是"按轨跳跃"。 */
    private var lastFilledBlock = -1L

    @Volatile private var prefetchEnabled = false

    override fun size(): Long = sizeBytes

    /**
     * 累计从网络读到的字节。**只用于诊断**：拿它除以时间就是这条源实际吃到的带宽，
     * 与解码侧需要的码率一比，就能分清卡顿是"网络供不上"还是"解码跟不上"。
     */
    fun deliveredBytes(): Long = stats.deliveredBytes

    override fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int = synchronized(lock) {
        if (closed) return -1
        if (len <= 0) return 0
        if (offset < 0 || offset >= sizeBytes) return -1
        val want = minOf(len.toLong(), sizeBytes - offset).toInt()
        stats.onRead()

        // 顺序播放的主路径：预读命中就直接拷内存，网络延迟不在这条路上。
        if (ahead.copyIfPresent(offset, buf, off, want)) {
            stats.onHit(fromPrefetch = true)
            stats.maybeReport()
            return want
        }
        if (window.copyIfPresent(offset, buf, off, want)) {
            stats.onHit(fromPrefetch = false)
            stats.maybeReport()
            return want
        }
        // 预读里有未消费数据、只是还没覆盖到这次请求 —— 这是顺序播放暂时落后，必须等它。
        if (ahead.waitFor(offset, buf, off, want, tuning.aheadWaitNs)) {
            stats.onHit(fromPrefetch = true)
            stats.maybeReport()
            return want
        }

        val fillStarted = System.nanoTime()
        var filledEnd = -1L
        try {
            // 请求本身就比半块大：填窗口只会白白多读一遍，直接读。
            if (want >= tuning.blockBytes / 2) return channel.readFully(offset, buf, off, want)
            filledEnd = window.fill(offset, sizeBytes, channel::readFully)
        } finally {
            val elapsed = (System.nanoTime() - fillStarted) / 1_000_000.0
            stats.onFill(elapsed)
            if (filledEnd > 0) {
                stats.onMissBlock(window.blockIndex(offset))
                // 顺序推进就把预读基线推到"本次请求之后"接着灌；大幅回退说明是乱序
                // （起播 seek、轨道错位）—— 只暂停，不销毁。上一版在这里永久关闭预读，
                // 结果起播那一次跳转就把预读钉死了，整部片子 prefetch 恒为 0。
                // 允许前进 0~3 块：块 2 MiB 时一次 miss 跨两三块是正常顺序推进。
                val index = window.blockIndex(offset)
                val sequential = lastFilledBlock < 0 || (index >= lastFilledBlock && index <= lastFilledBlock + 3)
                lastFilledBlock = index
                if (sequential) {
                    if (prefetchEnabled) ahead.start(maxOf(offset + want, filledEnd))
                } else if (ahead.active) {
                    ahead.pause()
                }
            }
            stats.maybeReport()
        }

        // 填充已把整块放进缓存，这里从缓存取 —— 命中判定只有一处，不会分叉。
        if (window.copyIfPresent(offset, buf, off, want)) return want
        // 跨块边界（块内剩余字节不够这次请求）：退化为直读。
        // 绝不能返回 -1 —— 上层会把 -1 当 EOF，一次跨界就成了"文件提前结束"。
        return channel.readFully(offset, buf, off, want)
    }

    /**
     * 按解码路径设置是否启用顺序预读。
     *
     * 软解是顺序流式读，预读命中率实测 100%（每 4 秒 90+ 次命中、0 次同步读），必须开；
     * 硬解是"突然要一大片"的突发访问，预读提前读的数据大半用不上，只会在同一条 SMB 链路上
     * 挤占主读，实测帧率从 24.00 掉到 20.27。所以默认关闭，由调用方显式开启。
     *
     * 关闭只是**暂停**，不是销毁：调用方切换路径后重新开启仍然生效（修 P1-3）。
     */
    fun setPrefetchEnabled(enabled: Boolean) {
        prefetchEnabled = enabled
        if (!enabled) ahead.pause()
    }

    override fun close() = synchronized(lock) {
        closed = true
        ahead.close()
        channel.close()
        Unit
    }

    companion object {
        /**
         * 打开文件并建好读源。连接与取大小都在这里完成，失败直接抛给调用方。
         *
         * [online] 由播放页注入系统网络可达性；默认恒真，保证这个类在 JVM 上可测。
         */
        fun open(
            address: SmbAddress,
            credential: SmbCredential,
            filePath: String,
            online: () -> Boolean = { true },
        ): SmbByteSource {
            val reader = SmbReader(address, credential, filePath)
            reader.open()
            return SmbByteSource(
                address, credential, filePath, reader,
                SmbReadTuning.forFile(reader.size), online,
            )
        }
    }
}
