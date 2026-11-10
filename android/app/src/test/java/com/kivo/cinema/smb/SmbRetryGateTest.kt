package com.kivo.cinema.smb

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * 退避状态机（250 S1）。
 *
 * 243 的 r1 版本在这里错过一次：把"重连失败"当终态，于是断网 8 秒、网络第 3 秒
 * 就恢复，剩下 5 秒的读全落在一个死掉的判断上。当时的修法是"改成有退避的持续重试"，
 * 但那之后它和读取逻辑缠在一起，**没法单独证明它对**。
 *
 * 现在它是一张时间表：给定失败时刻，`canAttempt` 的布尔序列是确定的。
 */
class SmbRetryGateTest {

    @Test
    fun `first attempt is allowed`() {
        val gate = SmbRetryGate()
        assertTrue(gate.canAttempt(0))
        assertEquals(0L, gate.backoffMs)
        assertNull(gate.lastFailure)
    }

    @Test
    fun `failure backs off from base and doubles up to the cap`() {
        val gate = SmbRetryGate()
        val expected = longArrayOf(400, 800, 1_600, 3_200, 4_000, 4_000)
        var now = 0L
        for (step in expected) {
            gate.noteFailure(now)
            assertEquals("backoff after failure at t=$now", step, gate.backoffMs)
            assertEquals(now + step, gate.nextAttemptAtMs)
            assertFalse("must wait until the deadline (t=$now)", gate.canAttempt(now + step - 1))
            assertTrue("must be allowed at the deadline (t=$now)", gate.canAttempt(now + step))
            now += step
        }
    }

    @Test
    fun `success clears backoff and the recorded reason`() {
        val gate = SmbRetryGate()
        gate.noteFailure(0, "boom")
        gate.noteSuccess()
        assertEquals(0L, gate.backoffMs)
        assertEquals(0L, gate.nextAttemptAtMs)
        assertNull(gate.lastFailure)
        assertTrue(gate.canAttempt(0))
    }

    @Test
    fun `offline blocks attempts and does not escalate the backoff`() {
        var online = true
        val gate = SmbRetryGate(online = { online })
        gate.noteFailure(0)
        val whileOnline = gate.backoffMs
        assertEquals(SmbRetryGate.BASE_MS, whileOnline)

        online = false
        assertFalse(gate.canAttempt(1_000))
        gate.noteFailure(1_000)
        assertEquals(
            "指数退避是保护服务器的，不是惩罚用户的；离线时翻倍只会拖慢恢复",
            whileOnline,
            gate.backoffMs,
        )
        assertEquals(SmbRetryGate.OFFLINE_REASON, gate.denyReason())
    }

    @Test
    fun `link recovery allows an immediate attempt`() {
        var online = false
        val gate = SmbRetryGate(online = { online })
        gate.noteFailure(0)
        assertFalse(gate.canAttempt(10_000))

        online = true
        assertTrue(
            "网络回来了还要等旧退避，用户看到的是「网回来了但画面还停着」",
            gate.canAttempt(10_000),
        )
        assertEquals(0L, gate.backoffMs)
    }

    @Test
    fun `deny reason prefers the recorded failure over the generic line`() {
        val gate = SmbRetryGate()
        gate.noteFailure(0, "SMB 连接已断开，重连失败：超时")
        assertEquals("SMB 连接已断开，重连失败：超时", gate.denyReason())

        gate.noteSuccess()
        assertEquals(SmbRetryGate.WAITING_REASON, gate.denyReason())
    }
}
