package com.kivo.cinema.smb

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * 读取参数推导（250 S1）。
 *
 * 两条不能破的性质：
 * 1. **大文件档必须逐位等于 247 实测通过的那组值**（6 × 2 MiB + 6 MiB + 350 ms）。
 *    那组数字不是上限，是 `Shelter` 的 2×TrueHD + PGS 四条轨常驻的下限。
 * 2. **任何档位下预读环都放得下两个 chunk**。只放得下一个时，消费者取走一部分，
 *    写指针立刻追尾读指针，预读线程会在"填满 → 立刻又满"之间空转。
 */
class SmbReadTuningTest {

    @Test
    fun `default matches the verified p247 budget`() {
        val tuning = SmbReadTuning.default
        assertEquals(2 shl 20, tuning.blockBytes)
        assertEquals(6, tuning.blockCount)
        assertEquals(6 shl 20, tuning.aheadBytes)
        assertEquals(1 shl 20, tuning.aheadChunk)
        assertEquals(350_000_000L, tuning.aheadWaitNs)
        assertEquals(18L shl 20, tuning.residentBytes)
    }

    @Test
    fun `small file drops to two blocks`() {
        val tuning = SmbReadTuning.forFile(100L shl 20)
        assertEquals(2, tuning.blockCount)
        assertEquals(2 shl 20, tuning.aheadBytes)
        assertEquals(6L shl 20, tuning.residentBytes)
    }

    @Test
    fun `medium file keeps four blocks`() {
        val tuning = SmbReadTuning.forFile(1L shl 30)
        assertEquals(4, tuning.blockCount)
        assertEquals(4 shl 20, tuning.aheadBytes)
    }

    @Test
    fun `large file keeps the default budget`() {
        assertEquals(SmbReadTuning.default, SmbReadTuning.forFile(20L shl 30))
    }

    @Test
    fun `unknown size falls back to default instead of guessing small`() {
        assertEquals(SmbReadTuning.default, SmbReadTuning.forFile(0L))
        assertEquals(SmbReadTuning.default, SmbReadTuning.forFile(-1L))
    }

    @Test
    fun `out of range values are clamped`() {
        val wild = SmbReadTuning(
            blockBytes = 64 shl 20,
            blockCount = 99,
            aheadBytes = 1,
            aheadChunk = 64 shl 20,
            aheadWaitNs = 0L,
        ).clamped()
        assertEquals(SmbReadTuning.MAX_BLOCK_BYTES, wild.blockBytes)
        assertEquals(SmbReadTuning.MAX_BLOCK_COUNT, wild.blockCount)
        assertEquals(SmbReadTuning.MAX_AHEAD_CHUNK, wild.aheadChunk)
        assertEquals(SmbReadTuning.MIN_AHEAD_BYTES, wild.aheadBytes)
        assertEquals(SmbReadTuning.MIN_AHEAD_WAIT_NS, wild.aheadWaitNs)
    }

    @Test
    fun `every tier keeps the ring able to hold two chunks`() {
        val sizes = longArrayOf(
            0L,
            64L shl 20,
            100L shl 20,
            255L shl 20,
            256L shl 20,
            1L shl 30,
            3L shl 30,
            20L shl 30,
            200L shl 30,
        )
        for (size in sizes) {
            val tuning = SmbReadTuning.forFile(size)
            assertTrue(
                "aheadBytes must hold two chunks at size=$size, got ${tuning.aheadBytes} vs ${tuning.aheadChunk}",
                tuning.aheadBytes >= tuning.aheadChunk * 2,
            )
            assertTrue("window must be positive at size=$size", tuning.windowBytes > 0)
            assertTrue(
                "wait must stay under one second at size=$size",
                tuning.aheadWaitNs <= SmbReadTuning.MAX_AHEAD_WAIT_NS,
            )
        }
    }
}
