package com.kivo.cinema.smb

import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * 预读环形缓冲（250 S2）。
 *
 * 这些断言以前只能用真机播放来验：代际校验错了会花屏，边界算错了会读到别的位置。
 * 现在它是纯数据，毫秒级就能证明 —— 这正是把缓冲从 [SmbReadAhead] 拆出来的原因。
 */
class SmbReadRingTest {

    @Test
    fun `not usable before reset`() {
        val ring = SmbReadRing(64)
        assertFalse(ring.ready)
        assertEquals(-1, ring.indexOf(0, 4))
    }

    @Test
    fun `reset allocates and moves the base`() {
        val ring = SmbReadRing(64)
        ring.reset(1_000)
        assertTrue(ring.ready)
        assertEquals(1_000L, ring.base)
        assertEquals(0, ring.filled)
        assertEquals(64, ring.writable)
    }

    @Test
    fun `append copy and consume`() {
        val ring = SmbReadRing(16)
        ring.reset(0)
        assertTrue(ring.append(ring.epochValue, ByteArray(8) { it.toByte() }, 8))
        assertEquals(8, ring.filled)

        val index = ring.indexOf(0, 4)
        assertEquals(0, index)
        val out = ByteArray(4)
        ring.copyFrom(index, out, 0, 4)
        assertArrayEquals(byteArrayOf(0, 1, 2, 3), out)

        ring.consume(index, 4)
        assertEquals(4, ring.position)
        assertEquals(4, ring.indexOf(4, 4))
    }

    @Test
    fun `request crossing the filled end is not a hit`() {
        val ring = SmbReadRing(16)
        ring.reset(0)
        ring.append(ring.epochValue, ByteArray(8), 8)
        assertEquals(-1, ring.indexOf(6, 4))
    }

    @Test
    fun `request before the consumed point is not a hit`() {
        val ring = SmbReadRing(16)
        ring.reset(0)
        ring.append(ring.epochValue, ByteArray(8), 8)
        ring.consume(ring.indexOf(0, 4), 4)
        assertEquals(-1, ring.indexOf(0, 2))
    }

    @Test
    fun `stale epoch data is discarded`() {
        val ring = SmbReadRing(16)
        ring.reset(0)
        val stale = ring.epochValue
        ring.reset(500)
        assertFalse("消费者跳读后，旧位置的数据必须整批丢弃", ring.append(stale, ByteArray(4), 4))
        assertEquals(0, ring.filled)
    }

    @Test
    fun `compact moves the consumed prefix out`() {
        val ring = SmbReadRing(16)
        ring.reset(0)
        ring.append(ring.epochValue, ByteArray(16) { 1.toByte() }, 16)
        ring.consume(ring.indexOf(0, 8), 8)
        ring.compact()
        assertEquals(8L, ring.base)
        assertEquals(8, ring.filled)
        assertEquals(0, ring.position)
        assertEquals(8, ring.writable)
    }

    @Test
    fun `compact does nothing before half is consumed`() {
        val ring = SmbReadRing(16)
        ring.reset(0)
        ring.append(ring.epochValue, ByteArray(16) { 1.toByte() }, 16)
        ring.consume(ring.indexOf(0, 4), 4)
        ring.compact()
        assertEquals(0L, ring.base)
        assertEquals(16, ring.filled)
        assertEquals(4, ring.position)
    }

    @Test
    fun `append respects capacity`() {
        val ring = SmbReadRing(8)
        ring.reset(0)
        assertTrue(ring.append(ring.epochValue, ByteArray(8), 8))
        assertFalse(ring.append(ring.epochValue, ByteArray(1), 1))
    }

    @Test
    fun `invalidate drops content and bumps the epoch`() {
        val ring = SmbReadRing(8)
        ring.reset(0)
        ring.append(ring.epochValue, ByteArray(8), 8)
        val before = ring.epochValue
        ring.invalidate()
        assertEquals(before + 1, ring.epochValue)
        assertEquals(0, ring.filled)
        assertEquals(-1, ring.indexOf(0, 1))
    }
}
