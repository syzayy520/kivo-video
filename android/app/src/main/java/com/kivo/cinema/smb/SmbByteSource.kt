package com.kivo.cinema.smb

import com.kivo.cinema.net.ByteSource
import java.io.IOException

/**
 * 带预读窗口与断线重连的 SMB 随机读源（243 D3）。
 *
 * 为什么需要预读：libsmb2 每次 `pread` 就是一个 SMB2 READ 往返。fd 回调层通常按
 * 64 KiB 步进请求，如果每次都直连网络，4K 片源（80 Mbps）每秒要发几百次往返。
 * 这里按 [BLOCK_BYTES] 预读一块缓存住，顺序播放时 16 次回调才打一次网络。
 *
 * 三条从 Kivo 音乐播放器（F:\kivo）移植的读取纪律：
 * 1. **只有真到文件尾才返回 -1**。未到尾部却读到 0 是传输异常，抛 [IOException]，
 *    不能伪装成 EOF —— 否则播放器会认为"片源结束"，画面静默停住。
 * 2. **失败要能重连，并且允许重连多次**。Wi-Fi 抖动 / 硬盘休眠唤醒 / 服务器重启之后
 *    旧 socket 已经死了，但服务器还在，重开一次就能接着播。重连失败不进入终态，
 *    只按退避（400 ms → 4 s 封顶）推迟下一次尝试；"什么时候放弃"由 fd 层的耐心窗口决定。
 * 3. **读写与 close 共用一把锁**：解码线程在反复读，用户可能同时返回触发 close。
 *
 * 线程模型：`readAt` 可能被多个 fd 回调线程调用（探针 + 解码器各开一个 fd），
 * 而 libsmb2 的 context 不支持并发，所以 [lock] 把窗口与 pread 全部串起来。
 */
class SmbByteSource private constructor(
    private val address: SmbAddress,
    private val credential: SmbCredential,
    private val filePath: String,
    private var reader: SmbReader,
    private val sizeBytes: Long,
) : ByteSource {

    // ---- 分块缓存（LRU）----
    //
    // 为什么不是"一个 1 MiB 窗口"：硬解路径用的是 MediaExtractor，它按**轨道**在
    // 文件里来回跳（视频块 → 音频块 → 字幕块 → 回到视频）。单窗口下每次换轨都把
    // 上一块整个顶掉，于是永远不命中。实机实测（Shelter，HEVC 10bit + 2×TrueHD）：
    // 27 次读 / 27 次未命中 / 每次同步读 152 ms，命中率 0%。
    //
    // 帧率统计会被 MediaCodec 的深输入缓冲掩盖（看起来仍是 24 fps），但任何一次
    // 连续抖动都会直接变成画面停顿，两小时的片子迟早撞上。所以改成按块对齐的
    // 4 块 LRU：视频轨、音频轨、字幕轨各占一块常驻，另一块留给临时请求。
    // 块缓冲固定复用，不在读路径上分配内存。
    private val blockData = Array(BLOCK_COUNT) { ByteArray(BLOCK_BYTES) }
    private val blockStart = LongArray(BLOCK_COUNT) { -1L }
    private val blockLen = IntArray(BLOCK_COUNT) { 0 }
    private val blockUsed = LongArray(BLOCK_COUNT) { 0L }
    private var blockClock = 0L
    /** 上一次填充的块号：用来判断访问是"顺序推进"还是"按轨跳跃"。 */
    private var lastFilledBlock = -1L
    private val lock = Any()

    /**
     * 当前 [reader] 是否已经废掉。
     *
     * 重连失败时旧句柄会被 `close()`，但字段还指着它；此时再 `pread` 只会拿到
     * `SmbReader` 的"尚未打开"，把真实的"网络还没回来"盖掉 —— 上一版就是这么
     * 在断网恢复后仍然报"读取器尚未打开"的。有了这个标志，死句柄不会再被读，
     * 下一次尝试直接走重连。
     */
    private var readerDead = false

    /**
     * 已关闭。关闭后 fd 回调可能还会进来一两次，要当 EOF 处理而不是抛"尚未打开"。
     * 预读线程在锁外读它，所以必须是 volatile——否则关了它还照样往缓冲里灌。
     */
    @Volatile private var closed = false

    /**
     * 重连退避状态。
     *
     * 上一版把"重连失败"当成终态，记下原因后永远不再试 —— 结果断网 8 秒、
     * 网络第 3 秒就恢复了，剩下 5 秒的读全落在一个已经死掉的判断上，
     * 播放器报"重连失败"而不是接着播。
     *
     * 现在改成**有退避的持续重试**：每次失败把下次尝试时间往后推
     * （400 → 800 → 1600 → 3200 → 4000 ms 封顶），到点再试。
     * 真正"放弃"的决定权在 fd 层（[com.kivo.cinema.net.RangeFd] 的耐心窗口），
     * 不在这里——这一层只负责"能不能拿到字节"。
     */
    private var nextRetryAtMs = 0L
    private var retryBackoffMs = 0L
    private var lastReconnectError: String? = null

    // ---- 运行时读取画像（诊断）----
    //
    // **为什么必须分段看**：解码侧只报"每帧读用了 14.5 ms"，这是平均值，看上去
    // 只是个小开销。但 1 MiB 的窗口注定是"命中很多次、然后阻塞填一次"的锯齿模式
    // —— 平均值 14.5 ms 完全可以是"每 12 帧卡 174 ms"摊出来的。用户看到的是
    // 周期性顿一下，而平均值把这件事藏得干干净净。这里把命中率与单次填充的
    // 最长耗时一起报出来，锯齿才在数据里现形。
    //
    // 命中率低（<90%）= 窗口没覆盖住顺序读；fill_max 远大于 fill_avg = 网络有长尾。
    // 这两个结论决定下一步是加大窗口还是分离预读线程，而不是凭感觉调参数。
    private var statReads = 0L
    private var statHits = 0L
    private var statFills = 0L
    private var statFillMs = 0.0
    private var statFillMaxMs = 0.0
    private var statPfHits = 0L
    private var statReportAtMs = 0L
    private var statMissFirstBlock = -1L
    private var statMissLastBlock = -1L
    private var statPrevMissBlock = -1L
    private var statMissJumpMax = 0L

    // ---- 顺序预读（独立连接 + 环形窗口）----
    //
    // **为什么必须另起一条连接**：窗口填充是同步网络读，1 MiB 一次。解码线程一旦
    // 撞上填充就得干等——真机实测单次 100–300 ms，摊到 24 fps 上是"每十几帧卡一下"，
    // 观众看到的就是周期性顿帧。缩窗口只会让卡顿更频繁，放大窗口只会让每次卡更久，
    // 都不解决问题：**同步读本身就错了**。
    //
    // 这里让另一个线程用**自己的 SmbReader（自己的 libsmb2 context，独立 socket）**
    // 持续把后面的字节灌进环形缓冲。解码线程来要数据时几乎总是内存命中，
    // 网络延迟被彻底移出关键路径。
    //
    // 明文约束：
    // 1. 网络读一律在 [pfMutex] **之外**做——持锁做网络读等于把预读退化成同步读；
    // 2. 任何失败都不许污染播放：预读只是加速，坏了就置 [pfDisabled] 退回原路径；
    // 3. [pfEpoch] 是复位代际：消费者跳读时 +1，预读线程回写前必须核对，
    //    否则会把"旧位置的数据"追加到"新位置"的缓冲里——那是花屏/错帧级事故。
    private val pfMutex = Object()
    private var pfBuf = ByteArray(0)
    private var pfBase = 0L      // pfBuf[0] 对应的文件偏移
    private var pfFilled = 0     // pfBuf 中已灌入的字节数
    private var pfPos = 0        // 消费者已取走的字节数（相对 pfBuf）
    private var pfEpoch = 0
    private var pfThread: Thread? = null
    private var pfReader: SmbReader? = null
    @Volatile private var pfStop = false
    @Volatile private var pfDisabled = false
    @Volatile private var pfActive = false
    @Volatile private var prefetchEnabled = PREFETCH_DEFAULT

    override fun size(): Long = sizeBytes

    override fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int = synchronized(lock) {
        if (closed) return -1
        if (len <= 0) return 0
        if (offset < 0 || offset >= sizeBytes) return -1
        val want = minOf(len.toLong(), sizeBytes - offset).toInt()
        ++statReads

        // 顺序播放的主路径：预读缓冲命中就直接拷内存，网络延迟不在这条路上。
        if (copyFromPrefetch(offset, buf, off, want)) {
            ++statHits
            ++statPfHits
            reportReadStatsLocked()
            return want
        }

        if (copyFromWindow(offset, buf, off, want)) {
            ++statHits
            reportReadStatsLocked()
            return want
        }

        // 预读里有未消费数据、只是还没覆盖到这次请求 —— 这是顺序播放暂时落后，
        // **必须等预读**，不许自己另开一条同步读。
        //
        // 这条规则是实测换来的：两条连接同时拉同一台服务器会互相拖慢，
        // 1 MiB 单次读从 92 ms 恶化到 543 ms（差 6 倍）。抢着读只会越读越慢。
        if (waitPrefetch(offset, buf, off, want)) {
            ++statHits
            ++statPfHits
            reportReadStatsLocked()
            return want
        }
        val fillStarted = System.nanoTime()
        var filledEnd = -1L
        try {
            if (want >= BLOCK_BYTES / 2) return directRead(offset, buf, off, want)
            filledEnd = fillWindow(offset)
        } finally {
            val fillMs = (System.nanoTime() - fillStarted) / 1_000_000.0
            ++statFills
            statFillMs += fillMs
            if (fillMs > statFillMaxMs) statFillMaxMs = fillMs
            // 这一次是同步读（缓存没覆盖到），但下一次不该再同步：
            // 把预读基线推到"本次请求之后"，让它从那里接着灌。
            // 用块末尾而不是 offset+want —— 块那一整片已经到手了，让预读从重复读一遍
            // 等于把带宽白扔掉 1 MiB（正是首帧最容易卡的时刻）。
            if (filledEnd > 0) {
                val idx = (filledEnd - 1) / BLOCK_BYTES
                // 允许前进 0~3 块：块加大到 2 MiB 后，一次 miss 跨两三块是正常的
                // 顺序推进，判定太严会把预读误杀（实测：jump_max=1~2 却判成乱序）。
                // 只有**回退**或大幅跳跃才说明是乱序访问。
                val sequential = lastFilledBlock < 0 ||
                    (idx >= lastFilledBlock && idx <= lastFilledBlock + 3)
                lastFilledBlock = idx
                if (sequential) {
                    if (prefetchEnabled) restartPrefetchLocked(maxOf(offset + want, filledEnd))
                } else if (pfActive) {
                    // 位置大幅回退 = 真正的乱序（起播 seek、轨道错位）。
                    // **只暂停，不销毁**：上一版在这里永久关闭预读，结果起播阶段
                    // 那一次 5810 块的跳转就把预读钉死了，整部片子 prefetch 恒为 0。
                    // 暂停只是让它别在读乱序时段抢链路；访问恢复顺序后会被重新拉起。
                    synchronized(pfMutex) {
                        pfActive = false
                        ++pfEpoch
                        pfMutex.notifyAll()
                    }
                }
            }
            reportReadStatsLocked()
        }
        // fillWindow 已把整块放进缓存，这里从缓存取——命中判定只有一处，不会分叉。
        if (copyFromWindow(offset, buf, off, want)) return want
        // 跨块边界（块内剩余字节不够这次请求）：退化为直读。
        // 绝不能返回 -1——上层会把 -1 当 EOF，一次跨界就成了"文件提前结束"。
        return directRead(offset, buf, off, want)
    }

    /**
     * 等预读把这次请求覆盖上（顺序播放落后时的唯一正确做法）。
     *
     * 返回 false 的情形都是"别再等了，走同步读"：
     * - 预读没启用/缓冲空（首帧、seek 刚复位）；
     * - 请求落在已消费区间之前（基点跑过头，得复位）；
     * - 等到 [PF_WAIT_NS] 还没等到（网络真出问题，让同步路径去承担并暴露错误）。
     */
    private fun waitPrefetch(offset: Long, buf: ByteArray, off: Int, want: Int): Boolean {
        if (!pfActive || pfBuf.isEmpty()) return false
        val deadline = System.nanoTime() + PF_WAIT_NS
        synchronized(pfMutex) {
            while (true) {
                if (pfFilled > pfPos) {
                    val start = pfBase + pfPos
                    val end = pfBase + pfFilled
                    if (offset >= start && offset + want <= end) {
                        System.arraycopy(pfBuf, (offset - pfBase).toInt(), buf, off, want)
                        pfPos = (offset - pfBase).toInt() + want
                        pfMutex.notifyAll()
                        return true
                    }
                    // 请求比可用区间还早：预读基点已经被推过头，只能复位重来。
                    if (offset < start) return false
                } else {
                    return false
                }
                val remainNs = deadline - System.nanoTime()
                if (remainNs <= 0) return false
                pfMutex.wait((remainNs / 1_000_000).coerceAtLeast(1))
            }
        }
    }

    /**
     * 从预读环形缓冲取数据。区间必须**完整**落在可用窗口内才命中——
     * 部分命中意味着还要拼一次同步读，拼出来的顺序更难保证，不如直接走同步路径。
     */
    private fun copyFromPrefetch(offset: Long, buf: ByteArray, off: Int, want: Int): Boolean {
        if (!pfActive || pfBuf.isEmpty()) return false
        synchronized(pfMutex) {
            if (pfFilled <= pfPos) return false
            val start = pfBase + pfPos
            val end = pfBase + pfFilled
            if (offset < start || offset + want > end) return false
            System.arraycopy(pfBuf, (offset - pfBase).toInt(), buf, off, want)
            pfPos = (offset - pfBase).toInt() + want
            pfMutex.notifyAll()
            return true
        }
    }

    /** 把预读基线移到 [offset]，并（首次）拉起预读线程。调用方须持有 [lock]。 */
    private fun restartPrefetchLocked(offset: Long) {
        if (pfDisabled || closed || offset < 0 || offset >= sizeBytes) return
        synchronized(pfMutex) {
            if (pfBuf.isEmpty()) pfBuf = ByteArray(PF_CAPACITY)
            // 预读方向对的就别动它。
            //
            // 上一版无条件重置，等于**每一次同步读都把预读推倒重来**：消费者刚跳过
            // 2 MiB，预读刚灌到那里就被改基点、清零，于是永远追不上消费者，
            // 表现就是 prefetch 命中数恒为 0（实测硬解路径正是如此）。
            // 这里只在"请求落在预读覆盖区间之前"（真回退/seek）或"落在太远的未来"
            // 时才重设基点；顺路前进的情况一律让它接着灌。
            if (pfActive && pfFilled > 0 && offset >= pfBase && offset <= pfBase + PF_CAPACITY) {
                // 请求落在预读基点前方一个缓冲容量之内：它早晚会灌到那里，
                // 让预读接着往前走就行。阈值必须给到整个缓冲容量——给 1 MiB 时，
                // 消费者一次跨 2~4 块（2 MiB 块）会立刻触发重置，预读永远从零开始，
                // 表现就是"预读在跑但总是慢一步"，主读仍要自己读、两条连接互相拖慢。
                pfMutex.notifyAll()
            } else {
                pfBase = offset
                pfFilled = 0
                pfPos = 0
                ++pfEpoch
                pfActive = true
                pfMutex.notifyAll()
            }
        }
        if (pfThread == null) {
            val t = Thread({ prefetchLoop() }, "KivoSmbPrefetch")
            t.isDaemon = true
            pfThread = t
            t.start()
        }
    }

    /**
     * 预读线程：独立 [SmbReader]（独立 libsmb2 context），把后续字节顺序灌进 [pfBuf]。
     *
     * 只有一条纪律最重要：**网络读不持 [pfMutex]**。持锁读等于让消费者排在网络
     * 往返后面，那就和同步读一模一样，整个预读白做。
     */
    private fun prefetchLoop() {
        val local = try {
            SmbReader(address, credential, filePath).also { it.open() }
        } catch (e: Exception) {
            // 服务端可能限制同文件并发句柄。预读是加速层，开不出来就安静退回同步路径，
            // 绝不能让"为了更快"变成"开不了"。
            android.util.Log.w(
                "KivoSmbRead",
                "prefetch disabled (open failed): ${e.message?.take(80)}",
            )
            pfDisabled = true
            return
        }
        val chunk = ByteArray(PF_CHUNK)
        try {
        synchronized(pfMutex) { pfReader = local }
        while (!pfStop) {
            var base = 0L
            var filled = 0
            var epoch = 0
            synchronized(pfMutex) {
                while (!pfStop && (!pfActive || pfFilled - pfPos >= PF_CAPACITY)) {
                    pfMutex.wait(200)
                }
                if (pfStop) return
                // 消费者已经取走一半以上就把剩余部分前移，避免缓冲被"取过的部分"占死。
                if (pfPos >= PF_CAPACITY / 2) {
                    val remain = pfFilled - pfPos
                    System.arraycopy(pfBuf, pfPos, pfBuf, 0, remain)
                    pfBase += pfPos
                    pfFilled = remain
                    pfPos = 0
                }
                base = pfBase
                filled = pfFilled
                epoch = pfEpoch
            }
            val fileLeft = sizeBytes - (base + filled)
            val bufLeft = (PF_CAPACITY - filled).toLong()
            val want = minOf(PF_CHUNK.toLong(), fileLeft, bufLeft).toInt()
            if (want <= 0) {
                if (fileLeft <= 0) return  // 已到文件尾
                synchronized(pfMutex) { pfMutex.wait(100) }
                continue
            }
            val n = try {
                local.pread(base + filled, chunk, 0, want)
            } catch (e: Exception) {
                // 预读连接断了。不重连：重连要跟主读取器抢同一台服务器，
                // 反而会拖慢正在播的那条连接。直接降级。
                android.util.Log.w(
                    "KivoSmbRead",
                    "prefetch disabled (read failed): ${e.message?.take(80)}",
                )
                pfDisabled = true
                runCatching { local.close() }
                return
            }
            if (n <= 0) {
                try {
                    Thread.sleep(30)
                } catch (_: InterruptedException) {
                    return
                }
                continue
            }
            synchronized(pfMutex) {
                // 代际校验：消费者可能在这次网络读期间跳读了。把旧位置的数据
                // 追加进新位置的缓冲，画面会从错的地方开始——必须丢弃。
                if (epoch == pfEpoch) {
                    System.arraycopy(chunk, 0, pfBuf, filled, n)
                    pfFilled = filled + n
                }
                pfMutex.notifyAll()
            }
        }
        } finally {
            // 连接归本线程管：线程退出就把句柄还掉，否则每播一部片子漏一条 socket。
            synchronized(pfMutex) { if (pfReader === local) pfReader = null }
            runCatching { local.close() }
        }
    }

    /** 每 4 秒吐一行读取画像；数值全部来自本锁内部，天然自洽。 */
    private fun reportReadStatsLocked() {
        val nowMs = System.nanoTime() / 1_000_000
        if (statReportAtMs == 0L) {
            statReportAtMs = nowMs + STAT_REPORT_INTERVAL_MS
            return
        }
        if (nowMs < statReportAtMs) return
        statReportAtMs = nowMs + STAT_REPORT_INTERVAL_MS
        android.util.Log.i(
            "KivoSmbRead",
            "stats reads=%d hits=%d (%.0f%%) prefetch=%d fills=%d fill_avg=%.1fms fill_max=%.1fms miss_blk=[%d..%d] jump_max=%d"
                .format(
                    statReads, statHits, if (statReads > 0) statHits * 100.0 / statReads else 0.0,
                    statPfHits, statFills,
                    if (statFills > 0) statFillMs / statFills else 0.0, statFillMaxMs,
                    statMissFirstBlock, statMissLastBlock, statMissJumpMax,
                ),
        )
        // 每轮重置：报的是"刚过去的 4 秒"，不是开机以来的累计（累计会把偶发尖峰抹平）。
        statReads = 0
        statHits = 0
        statPfHits = 0
        statFills = 0
        statFillMs = 0.0
        statFillMaxMs = 0.0
        statMissFirstBlock = -1L
        statMissLastBlock = -1L
        statPrevMissBlock = -1L
        statMissJumpMax = 0L
    }

    /**
     * 按解码路径设置是否启用顺序预读。
     *
     * 软解是顺序流式读，预读命中率实测 100%（每 4 秒 90+ 次命中、0 次同步读），
     * 必须开；硬解是"突然要一大片"的突发访问，预读提前读的数据大半用不上，
     * 只会在同一条 SMB 链路上挤占主读，实测帧率从 24.00 掉到 20.27。
     */
    fun setPrefetchEnabled(enabled: Boolean) {
        prefetchEnabled = enabled
        if (!enabled) {
            pfStop = true
            synchronized(pfMutex) {
                pfActive = false
                ++pfEpoch
                pfMutex.notifyAll()
            }
        }
    }

    override fun close() = synchronized(lock) {
        closed = true
        // 预读线程可能正卡在一次网络读里（锁外，最长几百毫秒）。先置停止标志并
        // 叫醒它，再限时 join：它是 daemon，超时也不会留住进程。
        pfStop = true
        synchronized(pfMutex) { pfMutex.notifyAll() }
        pfThread?.let { runCatching { it.join(800) } }
        pfThread = null
        runCatching { pfReader?.close() }
        pfReader = null
        runCatching { reader.close() }
        Unit
    }

    /**
     * 从分块缓存取数据。块按 [BLOCK_BYTES] 对齐，请求必须完整落在某一块已填充的
     * 区间内才算命中——部分命中要拼两次网络读，拼出来的顺序更难保证。
     */
    private fun copyFromWindow(offset: Long, buf: ByteArray, off: Int, want: Int): Boolean {
        val aligned = offset - (offset % BLOCK_BYTES)
        for (i in 0 until BLOCK_COUNT) {
            if (blockStart[i] != aligned) continue
            val len = blockLen[i]
            val relative = (offset - aligned).toInt()
            if (relative < 0 || relative + want > len) return false
            System.arraycopy(blockData[i], relative, buf, off, want)
            blockUsed[i] = ++blockClock
            return true
        }
        return false
    }

    private fun directRead(offset: Long, buf: ByteArray, off: Int, want: Int): Int {
        var done = 0
        while (done < want) {
            val read = preadOrReconnect(offset + done, buf, off + done, want - done)
            if (read < 0) break                      // 真 EOF
            if (read == 0) {
                // 未到尾部却读到 0：按传输异常处理，不交给上层当 EOF。
                throw IOException("SMB 读取中断（${offset + done} 字节处返回空数据）")
            }
            done += read
        }
        return if (done == 0) -1 else done
    }

    /**
     * 把 [offset] 所在的整块读进缓存，返回该块结束位置（失败返回 -1）。
     *
     * 块按 [BLOCK_BYTES] 对齐是关键：不对齐的话每次请求起点都不同，填出来的区间
     * 彼此错开，缓存永远对不上——那正是上一版 0% 命中率的成因之一。
     */
    private fun fillWindow(offset: Long): Long {
        val aligned = offset - (offset % BLOCK_BYTES)
        val length = minOf(BLOCK_BYTES.toLong(), sizeBytes - aligned).toInt()
        if (length <= 0) return -1L
        var slot = -1
        for (i in 0 until BLOCK_COUNT) {
            if (blockStart[i] == aligned) { slot = i; break }
        }
        if (slot < 0) {
            slot = 0
            var oldest = blockUsed[0]
            for (i in 0 until BLOCK_COUNT) {
                if (blockStart[i] < 0) { slot = i; break }   // 空槽优先
                if (blockUsed[i] < oldest) { oldest = blockUsed[i]; slot = i }
            }
        }
        val data = blockData[slot]
        var done = 0
        while (done < length) {
            val read = preadOrReconnect(aligned + done, data, done, length - done)
            if (read < 0) break
            if (read == 0) {
                throw IOException("SMB 读取中断（${aligned + done} 字节处返回空数据）")
            }
            done += read
        }
        if (done <= 0) return -1L
        blockStart[slot] = aligned
        blockLen[slot] = done
        blockUsed[slot] = ++blockClock
        // 诊断：未命中的块号分布。单调推进 = 顺序访问（预读本该覆盖）；
        // 在少数几个块号之间来回跳 = 按轨交错（需要多块常驻，不是加大预读能解决的）。
        val idx = aligned / BLOCK_BYTES
        if (statMissFirstBlock < 0) statMissFirstBlock = idx
        statMissLastBlock = idx
        if (statPrevMissBlock >= 0) {
            val jump = kotlin.math.abs(idx - statPrevMissBlock)
            if (jump > statMissJumpMax) statMissJumpMax = jump
        }
        statPrevMissBlock = idx
        return aligned + done
    }

    /**
     * 一次读；失败时重连并重试同一位置。
     *
     * 旧 socket 已经死了，`close` 也可能失败或卡住 —— 失败无所谓，反正要丢掉。
     * 重连成功后清空窗口（旧数据来自已死的连接，不能再信）。
     *
     * 重连本身失败时**不进入终态**：记下真实原因、按退避把下次尝试推后，
     * 并标记 [readerDead]，让后续尝试跳过死句柄、直接重连。
     * "什么时候放弃"不在这一层，由 [com.kivo.cinema.net.RangeFd] 的耐心窗口决定。
     */
    private fun preadOrReconnect(offset: Long, buf: ByteArray, off: Int, len: Int): Int {
        if (closed) return -1
        val nowMs = System.nanoTime() / 1_000_000
        if (nowMs < nextRetryAtMs) {
            throw IOException(lastReconnectError ?: "SMB 连接已断开，正在等待重连")
        }

        if (!readerDead) {
            try {
                val read = reader.pread(offset, buf, off, len)
                if (read > 0) clearBackoff()
                return read
            } catch (_: Exception) {
                // 落到下面的重连路径；真实原因由重连结果决定（成功就没事，
                // 失败会带上连不上的原因，比这里的 socket 报错更有用）。
            }
        }

        runCatching { reader.close() }
        readerDead = true
        val fresh = SmbReader(address, credential, filePath)
        try {
            fresh.open()
        } catch (reopenFailed: Exception) {
            val reason = "SMB 连接已断开，重连失败：${reopenFailed.message?.take(80) ?: "未知原因"}"
            lastReconnectError = reason
            retryBackoffMs = if (retryBackoffMs == 0L) 400L else minOf(retryBackoffMs * 2, 4000L)
            nextRetryAtMs = nowMs + retryBackoffMs
            throw IOException(reason, reopenFailed)
        }
        reader = fresh
        readerDead = false
        // 旧数据来自已死的连接，整批作废。
        for (i in 0 until BLOCK_COUNT) {
            blockStart[i] = -1L
            blockLen[i] = 0
        }
        clearBackoff()
        return fresh.pread(offset, buf, off, len)
    }

    private fun clearBackoff() {
        retryBackoffMs = 0L
        nextRetryAtMs = 0L
        lastReconnectError = null
    }

    companion object {
        /**
         * 缓存块 2 MiB。
         *
         * 1 MiB 时实测命中率只有 82%：MKV 的音视频交织距离会超过 1 MiB，一旦跨过
         * 就要重新同步读一次（约 100 ms），在画面上就是一次可见的顿。放大到 2 MiB
         * 让单块覆盖更宽的交织跨度，命中率与单次成本一起权衡。
         *
         * 按块对齐是这个值能生效的前提——不对齐等于每次请求都换一段区间，缓存对不上。
         */
        private const val BLOCK_BYTES = 2 shl 20

        /**
         * 缓存块数量 6（共 12 MiB）。
         *
         * 依据是硬解路径的实际访问形态：MKV 里视频轨、音频轨（这部片有 2 条 TrueHD）、
         * PGS 字幕轨各自在被按轨交错读取，同一时刻常驻的位置有 3 个，留 3 个机动。
         * 12 MiB 常驻内存对播放器可以忽略。
         */
        private const val BLOCK_COUNT = 6

        /**
         * 预读环容量 6 MiB。
         *
         * 依据是"要盖住一次网络读的最坏抖动"：真机实测单次 1 MiB 填充最长 300 ms 量级，
         * 这部 REMUX 码率约 3.6 MB/s，6 MiB ≈ 提前 1.7 秒，够吸收连续两次抖动而
         * 解码线程完全不碰网络。再大只是白占内存。
         */
        private const val PF_CAPACITY = 6 shl 20

        /**
         * 单次预读块 1 MiB。
         *
         * 比 256 KiB 更划算：SMB 的往返开销摊在更大的块上，实测 1 MiB 一次能跑到
         * 11 MB/s（92 ms），而码率只有 3.6 MB/s，余量 3 倍；块再小反而被往返次数拖住。
         */
        private const val PF_CHUNK = 1 shl 20

        /**
         * 顺序播放时允许等预读多久。
         *
         * 一次 1 MiB 读最坏 300 ms。这里给 350 ms —— 只覆盖"预读已经在路上、
         * 就差这一下"的情形。**不能给到秒级**：硬解路径的请求会跳到预读窗口
         * 前面很远，等下去就是白白把一帧的预算耗光。
         */
        private const val PF_WAIT_NS = 350_000_000L

        /**
         * 顺序预读的默认值：**关闭**，由调用方按解码路径显式开启。
         *
         * 实测（Shelter，HEVC 10bit / 12.2 GB / SMB）：
         * 硬解 + 预读 20.27 fps，末尾连续 183/233/266/567 ms 卡顿；
         * 硬解 + 预读关闭 24.00 fps，帧间隔 max 50 ms。
         *
         * 原因是预读用**第二条 SMB 连接**，和主读分同一条链路；硬解的访问是
         * "突然要一大片"，预读提前读的数据大半没被用上，却把主读挤慢了。
         * 软解是顺序流式读，预读命中率实测 100%、收益为正——所以它是按路径
         * 设置的字段，不是全局常量。
         */
        private const val PREFETCH_DEFAULT = false

        /** 画像上报周期。4 秒 ≈ 96 帧，足够看出锯齿，又不会把日志刷爆。 */
        private const val STAT_REPORT_INTERVAL_MS = 4_000L

        /** 打开文件并建好读源（连接、取大小都在这里完成，失败直接抛给调用方）。 */
        fun open(address: SmbAddress, credential: SmbCredential, filePath: String): SmbByteSource {
            val reader = SmbReader(address, credential, filePath)
            reader.open()
            return SmbByteSource(address, credential, filePath, reader, reader.size)
        }
    }
}
