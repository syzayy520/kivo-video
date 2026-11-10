package com.kivo.cinema.smb

import org.junit.Assert.assertEquals
import org.junit.Test
import java.util.Arrays

/**
 * 块窗口（251）：**已完整驻留的块不许再读一次**。
 *
 * 这条来自真机实测：24 MiB 顺序读填了 25 次、从网上拉了 50 MiB —— 一倍的带宽白烧。
 * 触发条件是"请求跨块边界"，那一瞬间 `align` 仍落在**当前块**，于是刚读进来的整块
 * 又被读了一遍。用 1 KiB 的块在 JVM 上复现，比插着设备读 20 GB 片源快得多。
 */
class SmbReadWindowTest {

    private val tuning = SmbReadTuning(
        blockBytes = 1024,
        blockCount = 3,
        aheadBytes = 4096,
        aheadChunk = 1024,
    )

    /** 计数的假读：按需返回字节，不碰网络。 */
    private class FakeRead(private val size: Long, private val shortOnce: Int = 0) {
        var calls = 0
            private set
        var bytes = 0L
            private set

        fun read(offset: Long, buf: ByteArray, off: Int, len: Int): Int {
            ++calls
            if (offset >= size) return -1
            val room = minOf(len.toLong(), size - offset).toInt()
            val n = if (shortOnce > 0 && calls == 1) minOf(shortOnce, room) else room
            Arrays.fill(buf, off, off + n, 0x2A.toByte())
            bytes += n
            return n
        }
    }

    @Test
    fun `填充一次后同块再填不再读网络`() {
        val fake = FakeRead(8192)
        val window = SmbReadWindow(tuning)

        assertEquals(1024L, window.fill(0, 8192, fake::read))
        assertEquals(1, fake.calls)

        // 跨块边界的那一刻：offset 仍在块 0，必须直接返回，不能再读。
        assertEquals(1024L, window.fill(1000, 8192, fake::read))
        assertEquals(1, fake.calls)
    }

    @Test
    fun `顺序读 24 块只该发 24 次读`() {
        val size = 1024L * 24
        val fake = FakeRead(size)
        val window = SmbReadWindow(tuning)

        // 复刻生产：起点未按块对齐（真机从 size/2 起读，几乎必然错位），
        // 于是每块末尾都会出现一次跨边界请求。
        var offset = 100L
        while (offset < size) {
            if (!window.copyIfPresent(offset, ByteArray(64), 0, 64)) {
                window.fill(offset, size, fake::read)
                window.copyIfPresent(offset, ByteArray(64), 0, 64)
            }
            offset += 64
        }

        // 修复前是每块读两遍（实测 24 MiB 拉了 50 MiB）。
        assertEquals(24, fake.calls)
        assertEquals(size, fake.bytes)
    }

    @Test
    fun `中途短读留下半块必须重读`() {
        val fake = FakeRead(8192, shortOnce = 100)
        val window = SmbReadWindow(tuning)

        assertEquals(100L, window.fill(0, 8192, fake::read))
        assertEquals(1, fake.calls)

        // 不是 EOF 却只拿到 100 字节：残数据不能当有效数据用。
        assertEquals(1024L, window.fill(0, 8192, fake::read))
        assertEquals(2, fake.calls)
    }

    @Test
    fun `文件尾的短块算完整不再重读`() {
        val size = 1500L
        val fake = FakeRead(size)
        val window = SmbReadWindow(tuning)

        assertEquals(size, window.fill(1024, size, fake::read))
        assertEquals(1, fake.calls)

        assertEquals(size, window.fill(1100, size, fake::read))
        assertEquals(1, fake.calls)
    }

    @Test
    fun `命中返回的字节与写入一致`() {
        val size = 2048L
        val fake = FakeRead(size)
        val window = SmbReadWindow(tuning)
        window.fill(0, size, fake::read)

        val out = ByteArray(16)
        val hit = window.copyIfPresent(1000, out, 0, 16)
        assertEquals(true, hit)
        assertEquals(0x2A.toByte(), out[0])
        assertEquals(0x2A.toByte(), out[15])
    }
}
