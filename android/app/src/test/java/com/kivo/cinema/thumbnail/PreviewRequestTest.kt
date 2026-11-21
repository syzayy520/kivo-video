package com.kivo.cinema.thumbnail

import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class PreviewRequestTest {
    @Test
    fun `new request preserves its own target and supersedes old request`() {
        val gate = PreviewRequestGate()
        val first = gate.next(1_234_000)
        val second = gate.next(5_678_000)
        assertEquals(1_234_000L, first.positionUs)
        assertEquals(5_678_000L, second.positionUs)
        assertTrue(second.id > first.id)
        assertFalse(gate.isCurrent(first))
        assertTrue(gate.isCurrent(second))
    }

    @Test
    fun `same target still creates a new identity`() {
        val gate = PreviewRequestGate()
        val first = gate.next(2_000_000)
        val second = gate.next(first.positionUs)
        assertTrue(second.id > first.id)
        assertFalse(gate.isCurrent(first))
        assertTrue(gate.isCurrent(second))
    }

    @Test
    fun `invalidate blocks old results before any replacement seek`() {
        val gate = PreviewRequestGate()
        val first = gate.next(2_000_000)
        gate.invalidate()
        assertFalse(gate.isCurrent(first))
        gate.invalidate() // 新手势/取消可以连续触发，不需要先发 seek。
        assertFalse(gate.isCurrent(first))
    }

    @Test
    fun `returning to same target after cancellation cannot revive old request`() {
        val gate = PreviewRequestGate()
        val first = gate.next(2_000_000)
        gate.invalidate()
        val second = gate.next(first.positionUs)
        assertTrue(second.id > first.id)
        assertFalse(gate.isCurrent(first))
        assertTrue(gate.isCurrent(second))
    }

    @Test
    fun `reader replacement rejects identical ids and targets from old reader`() {
        val oldReader = PreviewRequestGate()
        val newReader = PreviewRequestGate()
        val old = oldReader.next(2_000_000)
        oldReader.invalidate() // release 同步作废，异步内核收尾不影响闸门。
        val fresh = newReader.next(old.positionUs)
        assertEquals(old.id, fresh.id)
        assertFalse(oldReader.isCurrent(old))
        assertFalse(newReader.isCurrent(old))
        assertTrue(newReader.isCurrent(fresh))
    }

    @Test
    fun `matching numeric fields cannot forge a current request`() {
        val gate = PreviewRequestGate()
        val request = gate.next(2_000_000)
        assertFalse(gate.isCurrent(PreviewRequest(request.id, request.positionUs)))
        assertTrue(gate.isCurrent(request))
    }

    @Test
    fun `queued ready cache and error callbacks recheck the captured identity`() {
        for (result in listOf("ready", "cache", "unavailable")) {
            val gate = PreviewRequestGate()
            val request = gate.next(2_000_000)
            val published = mutableListOf<String>()
            assertTrue(gate.isCurrent(request)) // worker 已通过第一次校验。
            val postedToMain = { if (gate.isCurrent(request)) published.add(result) }
            gate.invalidate()
            gate.next(request.positionUs) // 相同秒级缓存/目标不能放行旧 post。
            postedToMain()
            assertTrue("stale $result reached UI", published.isEmpty())
        }
    }

    @Test
    fun `current queued callback remains publishable`() {
        val gate = PreviewRequestGate()
        val request = gate.next(2_000_000)
        var published = false
        val postedToMain = { if (gate.isCurrent(request)) published = true }
        postedToMain()
        assertTrue(published)
    }

    @Test
    fun `in flight worker observes invalidation without interrupting decode`() {
        val gate = PreviewRequestGate()
        val request = gate.next(2_000_000)
        val started = CountDownLatch(1)
        val completeDecode = CountDownLatch(1)
        val done = CountDownLatch(1)
        val published = AtomicBoolean(true)
        val worker = Thread {
            started.countDown()
            try {
                if (completeDecode.await(2, TimeUnit.SECONDS)) {
                    published.set(gate.isCurrent(request))
                }
            } finally {
                done.countDown()
            }
        }
        worker.start()
        try {
            assertTrue(started.await(2, TimeUnit.SECONDS))
            gate.invalidate()
            gate.next(request.positionUs)
        } finally {
            completeDecode.countDown()
        }
        assertTrue(done.await(2, TimeUnit.SECONDS))
        assertFalse(published.get())
    }
}
