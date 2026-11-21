package com.kivo.cinema.ui.compose

import java.util.concurrent.Executor
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

/** 纯逻辑验证：同步/队列 executor + 假 Host/Port，不依赖 Android 与真实内核。 */
class PlayerSpeedGestureControllerTest {

    private class FakePort(var speed: Double? = 1.0) : PlayerSpeedGestureController.Port {
        override val identity = Any()
        override var closed = false
        var writeFails = false
        var onRead: (() -> Unit)? = null
        val writes = mutableListOf<Double>()
        override fun read(): Double? { onRead?.invoke(); return speed }
        override fun write(value: Double): Boolean {
            if (writeFails) return false
            writes += value; speed = value; return true
        }
        override fun failure(): String = "内核拒绝"
    }

    private class FakeHost(var port: FakePort?) : PlayerSpeedGestureController.Host {
        var identity: Any? = port?.identity
        val renders = mutableListOf<Triple<Boolean, Boolean, Double?>>()
        val reports = mutableListOf<String>()
        var unlocks = 0
        override fun capture(): PlayerSpeedGestureController.Port? = port
        override fun currentIdentity(): Any? = identity
        override fun post(action: () -> Unit) = action()
        override fun render(active: Boolean, locked: Boolean, speed: Double?) {
            renders += Triple(active, locked, speed)
        }
        override fun feedback(started: Boolean, locked: Boolean) {}
        override fun report(identity: Any?, reason: String) { reports += reason }
        override fun unlocked() { unlocks += 1 }
    }

    private class QueueExecutor : Executor {
        val tasks = ArrayDeque<() -> Unit>()
        override fun execute(command: Runnable) { tasks += { command.run() } }
        fun runAll() { while (tasks.isNotEmpty()) tasks.removeFirst().invoke() }
    }

    private fun controller(port: FakePort, worker: Executor = Executor { it.run() }) =
        PlayerSpeedGestureController(FakeHost(port), worker)

    @Test
    fun `start never slows below original speed`() {
        val port = FakePort(speed = 2.5)
        controller(port).start(0f, 0f, base = 2.0)
        assertEquals(2.5, port.writes.last(), 0.0)
    }

    @Test
    fun `start raises to base when original is slower`() {
        val port = FakePort(speed = 1.0)
        val host = FakeHost(port)
        PlayerSpeedGestureController(host, Executor { it.run() }).start(0f, 0f, base = 2.0)
        assertEquals(2.0, port.writes.last(), 0.0)
        assertEquals(Triple(true, false, 2.0), host.renders.last())
    }

    @Test
    fun `unreadable original speed fails visibly without writing`() {
        val port = FakePort(speed = null)
        val host = FakeHost(port)
        PlayerSpeedGestureController(host, Executor { it.run() }).start(0f, 0f, base = 2.0)
        assertTrue(port.writes.isEmpty())
        assertTrue(host.reports.any { it.contains("启动失败") })
        assertTrue(host.renders.none { it.first })
    }

    @Test
    fun `invalid base fails visibly`() {
        val port = FakePort()
        val host = FakeHost(port)
        PlayerSpeedGestureController(host, Executor { it.run() }).start(0f, 0f, base = 0.1)
        assertTrue(port.writes.isEmpty())
        assertTrue(host.reports.any { it.contains("启动失败") })
    }

    @Test
    fun `release during initial read never publishes or writes`() {
        val port = FakePort()
        val c = controller(port)
        port.onRead = { c.end(keep = false) }
        c.start(0f, 0f, base = 2.0)
        assertTrue(port.writes.isEmpty())
    }

    @Test
    fun `keep before shown still restores`() {
        val port = FakePort()
        val c = controller(port)
        port.onRead = { c.end(keep = true) }
        c.start(0f, 0f, base = 2.0)
        assertTrue(port.writes.isEmpty())
    }

    @Test
    fun `drag quantizes to twentieths and clamps`() {
        val port = FakePort(speed = 1.0)
        val host = FakeHost(port)
        val c = PlayerSpeedGestureController(host, Executor { it.run() })
        c.start(0f, 0f, base = 2.0)
        c.drag(160f, 0f, allowSpeed = true, allowLock = false, lockPx = 100f)
        assertEquals(2.5, port.writes.last(), 0.0)
        c.drag(3200f, 0f, allowSpeed = true, allowLock = false, lockPx = 100f)
        assertEquals(3.0, port.writes.last(), 0.0)
        assertEquals(3.0, host.renders.last().third!!, 0.0)
    }

    @Test
    fun `queued drags collapse to latest offset`() {
        val port = FakePort(speed = 1.0)
        val worker = QueueExecutor()
        val c = controller(port, worker)
        c.start(0f, 0f, base = 2.0)
        worker.runAll()
        c.drag(320f, 0f, allowSpeed = true, allowLock = false, lockPx = 100f)
        c.drag(640f, 0f, allowSpeed = true, allowLock = false, lockPx = 100f)
        worker.runAll()
        assertEquals(listOf(2.0, 3.0), port.writes)
    }

    @Test
    fun `release restores original by default`() {
        val port = FakePort(speed = 1.0)
        val c = controller(port)
        c.start(0f, 0f, base = 2.0)
        c.drag(320f, 0f, allowSpeed = true, allowLock = false, lockPx = 100f)
        c.end(keep = false)
        assertEquals(1.0, port.writes.last(), 0.0)
    }

    @Test
    fun `keep retains confirmed speed`() {
        val port = FakePort(speed = 1.0)
        val c = controller(port)
        c.start(0f, 0f, base = 2.0)
        c.drag(320f, 0f, allowSpeed = true, allowLock = false, lockPx = 100f)
        c.end(keep = true)
        assertEquals(3.0, port.writes.last(), 0.0)
    }

    @Test
    fun `cancel forces restore despite keep`() {
        val port = FakePort(speed = 1.0)
        val c = controller(port)
        c.start(0f, 0f, base = 2.0)
        c.drag(320f, 0f, allowSpeed = true, allowLock = false, lockPx = 100f)
        c.cancel()
        assertEquals(1.0, port.writes.last(), 0.0)
    }

    @Test
    fun `swipe up locks and unlock restores`() {
        val port = FakePort(speed = 1.0)
        val host = FakeHost(port)
        val c = PlayerSpeedGestureController(host, Executor { it.run() })
        c.start(0f, 500f, base = 2.0)
        c.drag(0f, 300f, allowSpeed = true, allowLock = true, lockPx = 100f)
        c.end(keep = false)
        assertEquals(2.0, port.writes.last(), 0.0)
        c.unlock()
        assertEquals(1.0, port.writes.last(), 0.0)
        assertEquals(1, host.unlocks)
    }

    @Test
    fun `failed restore stays retryable on next start`() {
        val port = FakePort(speed = 1.0)
        val host = FakeHost(port)
        val c = PlayerSpeedGestureController(host, Executor { it.run() })
        c.start(0f, 0f, base = 2.0)
        port.writeFails = true
        c.end(keep = false)
        assertTrue(host.reports.any { it.contains("恢复") })
        port.writeFails = false
        c.start(0f, 0f, base = 2.0)
        assertTrue(port.writes.contains(1.0))
    }

    @Test
    fun `closed session skips restore with visible reason`() {
        val port = FakePort(speed = 1.0)
        val host = FakeHost(port)
        val c = PlayerSpeedGestureController(host, Executor { it.run() })
        c.start(0f, 0f, base = 2.0)
        port.closed = true
        c.end(keep = false)
        assertEquals(2.0, port.writes.last(), 0.0)
        assertTrue(host.reports.any { it.contains("跳过倍速恢复") })
    }

    @Test
    fun `stale session publish is rejected after identity switch`() {
        val port = FakePort(speed = 1.0)
        val host = FakeHost(port)
        val c = PlayerSpeedGestureController(host, Executor { it.run() })
        c.start(0f, 0f, base = 2.0)
        host.identity = Any()
        c.drag(320f, 0f, allowSpeed = true, allowLock = false, lockPx = 100f)
        assertEquals(3.0, port.writes.last(), 0.0)
        assertEquals(2.0, host.renders.last().third!!, 0.0)
    }

    @Test
    fun `applySpeed restores temporary speed first`() {
        val port = FakePort(speed = 1.0)
        val c = controller(port)
        c.start(0f, 0f, base = 2.0)
        c.drag(320f, 0f, allowSpeed = true, allowLock = false, lockPx = 100f)
        c.applySpeed(1.5)
        assertEquals(listOf(2.0, 3.0, 1.0, 1.5), port.writes)
    }

    @Test
    fun `clearSession runs callback only after restore barrier`() {
        val port = FakePort(speed = 1.0)
        val worker = QueueExecutor()
        val c = controller(port, worker)
        c.start(0f, 0f, base = 2.0)
        worker.runAll()
        val order = mutableListOf<String>()
        port.onRead = { order += "read" }
        c.clearSession { order += "after" }
        worker.runAll()
        assertEquals("after", order.last())
        assertEquals(1.0, port.writes.last(), 0.0)
    }

    @Test
    fun `shutdown is idempotent and still runs callback`() {
        val port = FakePort()
        val c = controller(port)
        var count = 0
        c.shutdown { count += 1 }
        c.shutdown { count += 1 }
        assertEquals(2, count)
    }
}