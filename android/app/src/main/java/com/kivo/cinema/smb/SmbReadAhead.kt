package com.kivo.cinema.smb

/**
 * 顺序预读（250 D4/D5）：线程生命周期 + 网络读 + 代际交接。
 *
 * 环形缓冲本身在 [SmbReadRing]（纯数据、可单测）；这里只负责"谁去填、什么时候填、
 * 什么时候停"。为什么必须另起一条连接：窗口填充是同步网络读，1 MiB 一次，
 * 真机实测单次 100–300 ms；解码线程撞上填充就得干等，摊到 24 fps 上就是
 * "每十几帧卡一下"。缩窗口让卡顿更频繁，放大窗口让每次卡更久 —— **同步读本身就错了**。
 *
 * 明文约束（每条都对应一次真实事故）：
 * 1. 网络读一律在锁**之外**做。持锁做网络读等于把预读退化成同步读。
 * 2. 任何失败都不许污染播放：预读只是加速，坏了就置 [disabled] 退回同步路径。
 * 3. 代际校验在 [SmbReadRing.append] 里做，回写前必须对上 epoch。
 *
 * **可重启（D5，修 P1-2/P1-3）**：上一版线程到文件尾 `return` 就再也不启动，
 * 持有线程的字段仍非 null，于是"播到结尾再拖回中间"预读永久失效，画面回到
 * 每 12 帧卡 174 ms 的状态。现在：
 *
 * ```
 * Idle ──start()──► Running ──EOF / stop()──► Idle ──start()──► Running
 *                      │
 *                      └── open/read 失败 ──► Disabled（终态，本实例不再启用）
 * ```
 */
internal class SmbReadAhead(
    private val tuning: SmbReadTuning,
    private val sizeBytes: Long,
    private val openReader: () -> SmbReader,
    private val log: (String) -> Unit,
) {
    private val ring = SmbReadRing(tuning.aheadBytes)
    private val mutex = Object()
    private var worker: Thread? = null
    private var workerReader: SmbReader? = null

    /** 启动后因失败永久关闭。**只留给真降级**，原因已经进 log。 */
    @Volatile private var disabled = false
    @Volatile private var closed = false
    @Volatile private var stopping = false
    @Volatile private var running = false

    /** 消费者当前是否在工作。乱序访问时暂停，别去抢链路。 */
    @Volatile var active = false
        private set

    /**
     * 把预读基线移到 [offset]，必要时拉起线程。
     *
     * 方向对的就不动它。上一版无条件重置，等于**每次同步读都把预读推倒重来**：
     * 消费者刚跳过 2 MiB，预读刚灌到那里就被改基点、清零，于是永远追不上，
     * 表现就是 prefetch 命中恒为 0（硬解路径正是如此）。
     *
     * 阈值给到整个缓冲容量：给 1 MiB 时消费者一次跨 2~4 块会立刻触发重置，
     * 预读永远从零开始 —— "预读在跑但总是慢一步"。
     */
    fun start(offset: Long) {
        if (closed || disabled || offset < 0 || offset >= sizeBytes) return
        synchronized(mutex) {
            val keepGoing = active && ring.filled > 0 &&
                offset >= ring.base && offset <= ring.base + tuning.aheadBytes
            if (!keepGoing) {
                ring.reset(offset)
                active = true
            }
            mutex.notifyAll()
            if (running) return
            stopping = false
            running = true
        }
        val thread = Thread({ loop() }, "KivoSmbPrefetch")
        thread.isDaemon = true
        synchronized(mutex) { worker = thread }
        thread.start()
    }

    /** 乱序访问：只暂停，不销毁。访问恢复顺序后会被 [start] 重新拉起。 */
    fun pause() {
        synchronized(mutex) {
            active = false
            ring.invalidate()
            mutex.notifyAll()
        }
    }

    /** 请求是否完整落在可用区间内。部分命中要拼两次网络读，不如直接走同步路径。 */
    fun copyIfPresent(offset: Long, buf: ByteArray, off: Int, want: Int): Boolean {
        if (!active || !ring.ready) return false
        synchronized(mutex) {
            val index = ring.indexOf(offset, want)
            if (index < 0) return false
            ring.copyFrom(index, buf, off, want)
            ring.consume(index, want)
            mutex.notifyAll()
            return true
        }
    }

    /**
     * 等预读把这次请求覆盖上（顺序播放落后时的唯一正确做法）。
     *
     * 返回 false 都是"别再等了"：没启用/缓冲空（首帧、seek 刚复位）、
     * 请求落在已消费区间之前（基点跑过头）、或等到超时（网络真出问题）。
     */
    fun waitFor(offset: Long, buf: ByteArray, off: Int, want: Int, timeoutNs: Long): Boolean {
        if (!active || !ring.ready) return false
        val deadline = System.nanoTime() + timeoutNs
        synchronized(mutex) {
            while (true) {
                val index = ring.indexOf(offset, want)
                if (index >= 0) {
                    ring.copyFrom(index, buf, off, want)
                    ring.consume(index, want)
                    mutex.notifyAll()
                    return true
                }
                if (ring.filled <= ring.position) return false
                if (offset < ring.base + ring.position) return false
                val remain = deadline - System.nanoTime()
                if (remain <= 0) return false
                mutex.wait((remain / 1_000_000).coerceAtLeast(1))
            }
        }
    }

    fun close() {
        closed = true
        synchronized(mutex) {
            active = false
            stopping = true
            mutex.notifyAll()
        }
        worker?.let { runCatching { it.join(800) } }
        worker = null
        runCatching { synchronized(mutex) { workerReader }?.close() }
    }

    /** 预读线程。唯一一条最重要的纪律：**网络读不持锁**。 */
    private fun loop() {
        val local = try {
            openReader().also { it.open() }
        } catch (e: Exception) {
            // 服务端可能限制同文件并发句柄。预读是加速层，开不出来就安静退回同步路径，
            // 绝不能让"为了更快"变成"开不了"。
            log("prefetch disabled (open failed): ${e.message?.take(80)}")
            disabled = true
            settle(null)
            return
        }
        val chunk = ByteArray(tuning.aheadChunk)
        try {
            synchronized(mutex) { workerReader = local }
            while (true) {
                var writeOffset = 0L
                var writable = 0
                var epoch = 0
                synchronized(mutex) {
                    // 背压：缓冲真的满了才等（不是"不足一个 chunk"就等，那会一直等）。
                    while (!stopping && (!active || ring.writable <= 0)) {
                        mutex.wait(200)
                    }
                    if (stopping) return
                    ring.compact()
                    writeOffset = ring.writeOffset
                    writable = ring.writable
                    epoch = ring.epochValue
                }
                if (writable <= 0) {
                    synchronized(mutex) { mutex.wait(100) }
                    continue
                }
                val fileLeft = sizeBytes - writeOffset
                if (fileLeft <= 0) return  // 到文件尾：回到 Idle，下次 start 还能干起来
                val want = minOf(tuning.aheadChunk.toLong(), fileLeft, writable.toLong()).toInt()
                val read = try {
                    local.pread(writeOffset, chunk, 0, want)
                } catch (e: Exception) {
                    // 预读连接断了。不重连：重连要跟主读取器抢同一台服务器，
                    // 反而会拖慢正在播的那条连接。直接降级。
                    log("prefetch disabled (read failed): ${e.message?.take(80)}")
                    disabled = true
                    return
                }
                if (read <= 0) {
                    try {
                        Thread.sleep(30)
                    } catch (_: InterruptedException) {
                        return
                    }
                    continue
                }
                synchronized(mutex) {
                    ring.append(epoch, chunk, read)
                    mutex.notifyAll()
                }
            }
        } finally {
            // 连接归本线程管：线程退出就把句柄还掉，否则每播一部片子漏一条 socket。
            settle(local)
        }
    }

    /** 统一收尾：还句柄、回到 Idle。**把 worker 置 null 是 D5 的关键**，否则永不重启。 */
    private fun settle(local: SmbReader?) {
        synchronized(mutex) {
            if (local != null && workerReader === local) workerReader = null
            running = false
            worker = null
            stopping = false
            active = false
        }
        local?.let { runCatching { it.close() } }
    }
}
