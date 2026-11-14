package com.kivo.cinema.net

import java.io.IOException

/**
 * 273 v6：**一条源站连接 + 自带预读**的独立读者（读侧）。
 *
 * ## 它只负责读语义
 *  · [readAt]：顺序读；**跳读**（seek）⇒ 丢弃缓冲、重开一块带 Range 的请求。
 *  · 读失败 ⇒ 抛出（上层转成用户可见 reason），不许返回半截数据装作成功。
 *  · 源站把连接掐掉 ⇒ 生产侧自己重开（见 [OriginPrefetchPump]），不是故障。
 *
 * ## 与 v4 的差别
 * v4 把"缓冲 + 生产线程 + 连接策略"全塞在这一个文件里，结果这个文件
 * 超了 300 行铁律，且两种变化原因互相踩。v5 拆成三份：
 *  · [OriginPrefetchBuffer] —— 共享的那份数据（环 + 同步）；
 *  · [OriginPrefetchPump] —— 供给侧（连接生命周期、失败恢复）；
 *  · 本文件 —— 消费侧（[ByteSource] 契约）。
 *
 * ## 线程模型
 * 一个生产线程（读源站）+ 调用方（读缓冲）。缓冲用锁 + wait/notify 同步。
 */
class HttpOriginReader(
    client: HttpOriginClient,
    startOffset: Long,
    capacityBytes: Int = DEFAULT_CAPACITY_BYTES,
    label: String = "play",
) : ByteSource {

    private val buffer = OriginPrefetchBuffer(capacityBytes, startOffset)
    private val pump = OriginPrefetchPump(client, buffer, label)

    override fun size(): Long {
        pump.start()
        val deadline = System.currentTimeMillis() + SIZE_WAIT_MS
        while (true) {
            buffer.knownTotal.takeIf { it > 0 }?.let { return it }
            buffer.lastFailure?.let { throw it }
            if (buffer.isClosed) throw IOException("读取层已关闭")
            if (System.currentTimeMillis() >= deadline) throw IOException("服务器未返回片源长度")
            try {
                Thread.sleep(100)
            } catch (interrupted: InterruptedException) {
                Thread.currentThread().interrupt()
                throw IOException("读取层被中断")
            }
        }
    }

    override fun readAt(offset: Long, buf: ByteArray, off: Int, len: Int): Int {
        if (len <= 0 || offset < 0) return 0
        pump.start()
        while (true) {
            if (buffer.readPosition != offset) {
                // 位置对不上 = 内核跳读了：丢掉缓冲，从新位置重开一条连接。
                try {
                    pump.requestJump(offset)
                } catch (interrupted: InterruptedException) {
                    Thread.currentThread().interrupt()
                    return 0
                }
                continue
            }
            val readable = try {
                buffer.awaitReadable(READ_WAIT_MS)
            } catch (interrupted: InterruptedException) {
                // 被中断 = 这一层要收工了（不该再供数）。
                Thread.currentThread().interrupt()
                return 0
            }
            if (readable) {
                val got = buffer.copyOut(offset, buf, off, len)
                if (got > 0) return got
                continue
            }
            buffer.lastFailure?.let { throw it }
            if (buffer.reachedEnd) return -1
            if (buffer.isClosed) return 0
        }
    }

    override fun close() {
        pump.close()
    }

    companion object {
        /**
         * 预读缓冲大小（字节）。
         *
         * 【为什么 8 MiB】4K 片源 435 KB/s ⇒ 8 MiB ≈ **19 秒**的片源。
         * 环的作用是**吸收源站的表头延迟**：每块 4 MiB 的范围请求要付一次
         * ≈350 ms 表头，环够深 ⇒ 内核永远有数据可读，看不见这个延迟。
         *
         * 【为什么不是 16 MiB】**每个 mpv 连接各有一份环**（内核每次 seek
         * 都新开连接），真机上同时活 3 条（head / cues / 播放）⇒
         * 16 MiB 环 = 3 × (16 环 + 8 carry) = **72 MiB** 常驻，
         * 叠上 mpv 自己的 demux 缓存（64 MiB）就是 273 的 924 MB 峰值。
         * 收成 8 MiB 后同一口径是 36 MiB，跑道仍有 19 秒。
         */
        const val DEFAULT_CAPACITY_BYTES = 8 * 1024 * 1024

        /** 等可读数据的轮询间隔。 */
        private const val READ_WAIT_MS = 300L

        /** 等总长的上限。 */
        private const val SIZE_WAIT_MS = 30_000L
    }
}
