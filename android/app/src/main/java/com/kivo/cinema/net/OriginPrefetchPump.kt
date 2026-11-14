package com.kivo.cinema.net

/**
 * 273 v6：预读**生产循环** —— 只负责"什么时候读、往哪写、要不要跳读"。
 *
 * ## 它为什么不管连接
 * "连接怎么活下来"（开哪一块、什么时候读完、失败了怎么重开）是**连接策略**，
 * 落在 [OriginConnectionLease] 里。两种变化原因：
 * 改策略不该动循环，改循环不该动策略。
 *
 * ## v5 → v6：把"每 3.5 秒新建一次连接"这条慢性病治掉
 *
 * v5 一条连接读到片尾（`Range: bytes=OFF-`），响应永不结束 ⇒ 连接回不了
 * keep-alive 池 ⇒ v5 的"闲置即回收"让每 3.5 秒付一次新建连接的代价。
 * 真机实测：新建连接表头 754–968 ms 且只有 ≈2 MB/s，复用连接只要 ≈300 ms
 * 且能跑 ≈10 MB/s —— 差 5 倍。真机 207 秒里 166 秒 mpv 缓冲就是这么来的。
 *
 * v6 改成**有界范围**（默认 8 MiB 一块），块读完 ⇒ 连接回池 ⇒ 下一块复用。
 *
 * ## 本循环的三条纪律
 *
 *  1. **读到的字节立刻进环**（不等整块读完）⇒ 起播快、seek 后首帧快；
 *  2. **环满时把块的尾巴先读进 carry** ⇒ 这一块总能读完 ⇒ 连接总能回池。
 *     【为什么不能停在半途】半途停下的响应会把连接晾在池外，下次续读时
 *     那条 TCP 可能已经退化成 60–250 KB/s（v5 实测），而新建连接要 800 ms。
 *  3. **跳读先作废 carry** ⇒ 旧位置的尾巴绝不会写进新位置的环里。
 */
class OriginPrefetchPump(
    private val client: HttpOriginClient,
    private val buffer: OriginPrefetchBuffer,
    private val label: String = "play",
) {
    private val lock = Object()

    /** 连接策略：开哪一块、什么时候读完、失败了怎么重开。 */
    private val lease = OriginConnectionLease(client, buffer, label)

    /** 生产线程的中转缓冲。 */
    private val scratch = ByteArray(SCRATCH_BYTES)

    /**
     * 当前这一块还没交给环的尾巴。
     *
     * 环满的时候，块的剩余部分先落到这里 —— 这样这一块总能读完，
     * 连接总能回到 keep-alive 池（否则半途停下的响应会退化成慢读）。
     */
    private val carry = ByteArray(client.chunkBytes.toInt())
    private var carrySize = 0

    /** 待执行的跳读目标；>= 0 表示"正在换流"。 */
    private var jumpTo = -1L

    /** 有跳读在等着处理（此时不许开"续读"连接，避免白开一条）。 */
    private var jumpPending = false

    private var closed = false
    private var producer: Thread? = null

    /**
     * 让路决策（273 v31）。
     *
     * 【为什么让路要掐掉在飞的块】源站带宽是封顶的（真机 Node 探针：
     * 同一段 4 MiB，单独下 1,650 ms，同时三条在跑 3,302 ms）——
     * 只"不开新块"没用，已经开的那块照样吃带宽。
     */
    private val yieldPolicy = OriginYieldPolicy(
        client.yieldGate,
        label,
        { client.landingInFlight?.invoke() == true },
    ) { lease.closeCurrent() }

    /** 启动生产线程（幂等）。 */
    fun start() = synchronized(lock) {
        if (producer != null || closed) return
        producer = Thread({ produce() }, "KivoOriginPrefetch").apply {
            isDaemon = true
            start()
        }
    }

    /**
     * 跳读：丢掉当前缓冲，从 [offset] 重开一块。
     *
     * 【为什么要先关旧流】生产线程可能正阻塞在旧块的 read() 里；
     * 不关它，跳读要等到那次读超时（30 s）才生效 —— seek 会变成"卡 30 秒"。
     */
    fun requestJump(offset: Long) {
        synchronized(lock) {
            if (closed) return
            jumpPending = true
            lock.notifyAll()
        }
        // 先关旧流：解除生产线程的阻塞；此刻还不发布跳读目标，
        // 生产线程看到 jumpPending 会停下"续读"这条路。
        lease.closeCurrent()
        synchronized(lock) {
            jumpTo = offset
            lock.notifyAll()
        }
        // 等生产线程把环挪到新位置。
        synchronized(lock) {
            while (!closed && buffer.readPosition != offset) {
                lock.wait(200)
            }
        }
    }

    fun close() {
        synchronized(lock) {
            if (closed) return
            closed = true
            lock.notifyAll()
        }
        lease.close()
        runCatching { producer?.interrupt() }
        buffer.close()
    }

    /**
     * 生产线程：把源站的字节持续拉进环形缓冲。
     *
     * 【为什么整段包在 try 里】线程被中断**不是故障**，是"该收工了"：
     * 关闭时 [close] 会中断本线程，而 [OriginPrefetchBuffer.awaitRoom] 里的
     * `wait()` 会把中断抛成 [InterruptedException]。不接住它，异常会逃出线程
     * 的 run() —— Android 对任何未捕获异常都会杀掉整个进程，
     * 表现为"seek 一下就白屏/闪退"（真机 2026-11-14 实测）。
     */
    private fun produce() {
        try {
            produceLoop()
        } catch (_: InterruptedException) {
            // 被叫停：正常收工。
            Thread.currentThread().interrupt()
        }
    }

    private fun produceLoop() {
        while (!closed) {
            val jump = synchronized(lock) {
                if (jumpTo >= 0) {
                    val t = jumpTo
                    jumpTo = -1L
                    jumpPending = false
                    t
                } else {
                    null
                }
            }
            if (jump != null) {
                // 跳读要**丢弃**缓冲（游标归位到新位置），旧尾巴也不能留。
                carrySize = 0
                lease.open(jump, reset = true)
                continue
            }

            val pendingJump = synchronized(lock) { jumpPending }
            if (pendingJump) {
                sleepQuietly(PENDING_WAIT_MS)
                continue
            }

            // 273 v31：**让路**（拖动期间 / 落点窗口在下时）。
            // 【为什么策略在另一个文件】见 [OriginYieldPolicy]（含 v30 的实测教训）。
            if (yieldPolicy.shouldYield()) {
                sleepQuietly(YIELD_WAIT_MS)
                continue
            }

            if (buffer.reachedEnd) {
                // 片源读完了：不再向源站要数据，等上层关闭。
                sleepQuietly(IDLE_WAIT_MS)
                continue
            }

            if (buffer.lastFailure != null) {
                // 已经出过错，原因已交给上层；不再徒劳地重试。
                sleepQuietly(IDLE_WAIT_MS)
                continue
            }

            // ① 先把尾巴交回环（顺序必须在这里保证：尾巴属于当前块，
            //    必须在开下一块之前交完）。
            if (carrySize > 0) {
                if (buffer.freeSpace <= 0) {
                    buffer.awaitRoom(ROOM_WAIT_MS)
                    continue
                }
                val n = buffer.copyIn(carry, 0, carrySize)
                if (n < carrySize) {
                    System.arraycopy(carry, n, carry, 0, carrySize - n)
                    carrySize -= n
                } else {
                    carrySize = 0
                }
                continue
            }

            // ② 没有块 ⇒ 开一块（从写入位置续读：环里那段还没交给内核，不能丢）。
            if (lease.current() == null) {
                lease.open(buffer.writePosition, reset = false)
                continue
            }

            val room = buffer.freeSpace
            val rest = lease.remaining()
            if (room <= 0) {
                // ③ 环满了：把当前块的剩余部分读进 carry ——
                //    这一块因此能读完，连接能回 keep-alive 池。
                if (rest <= 0) {
                    lease.finishIfExhausted()
                    continue
                }
                if (rest > carry.size - carrySize) {
                    // 尾巴放不下（源站忽略了范围请求，块大得离谱）⇒ 只能等环腾空间。
                    buffer.awaitRoom(ROOM_WAIT_MS)
                    continue
                }
                val got = lease.read(carry, carrySize, rest.toInt())
                if (got < 0) {
                    // 连接断了：尾巴作废，下一轮从环的写入位置重开一块。
                    carrySize = 0
                    continue
                }
                carrySize += got
                continue
            }

            // ④ 环有空间：边读边进环（不等整块读完 ⇒ 起播快）。
            val want = minOf(SCRATCH_BYTES.toLong(), room, rest).toInt()
            if (want <= 0) {
                // 这一块读满了 ⇒ 归还连接，让下一块能复用它。
                lease.finishIfExhausted()
                continue
            }
            val got = lease.read(scratch, 0, want)
            if (got <= 0) {
                // 0 = 这一块读完（连接已归还，可以开下一块）；-1 = 失败（连接已丢）。
                continue
            }
            buffer.copyIn(scratch, 0, got)
        }
    }

    private fun sleepQuietly(ms: Long) {
        try {
            Thread.sleep(ms)
        } catch (interrupted: InterruptedException) {
            Thread.currentThread().interrupt()
        }
    }

    companion object {
        /** 一次从源站读多少（中间缓冲，避免每 8 KB 一次系统调用）。 */
        private const val SCRATCH_BYTES = 256 * 1024

        /**
         * 等空间的轮询间隔。
         *
         * 【为什么不能太大】环满时生产线程就在这里等；用户此时拖动进度条，
         * 跳读要等这一轮超时才能被处理 ⇒ 白屏时间被这个值直接拉长。
         */
        private const val ROOM_WAIT_MS = 150L

        /** 等待办的轮询间隔。 */
        private const val PENDING_WAIT_MS = 100L

        /** 读完之后空等的间隔。 */
        private const val IDLE_WAIT_MS = 500L

        /**
         * 让路期间的轮询间隔。
         *
         * 【为什么是 100 ms】让路窗口以毫秒计，闸门一关就要能立刻恢复取数；
         * 1 s 太粗（松手后白等 1 s），20 ms 太密（空转）。
         */
        private const val YIELD_WAIT_MS = 100L
    }
}
