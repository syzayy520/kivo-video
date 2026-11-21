package com.kivo.cinema.ui.compose

import java.util.concurrent.CompletableFuture
import java.util.concurrent.Executor
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicReference
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.round

/** 单一倍速事务所有者。入口/Host 在主线程，Port 与恢复屏障仅在串行 worker。 */
internal class PlayerSpeedGestureController(
    private val host: Host,
    private val worker: Executor = Executors.newSingleThreadExecutor { Thread(it, "KivoSpeedGesture") },
) {
    interface Port {
        val identity: Any
        val closed: Boolean
        fun read(): Double?
        fun write(value: Double): Boolean
        fun failure(): String
    }
    interface Host {
        fun capture(): Port?
        fun currentIdentity(): Any?
        fun post(action: () -> Unit)
        fun render(active: Boolean, locked: Boolean, speed: Double?)
        fun feedback(started: Boolean, locked: Boolean)
        fun report(identity: Any?, reason: String)
        fun unlocked()
    }
    private data class Request(val offset: Double, val revision: Long)
    private class Gesture(val port: Port, val base: Double, val x: Float, val y: Float) {
        @Volatile var down = true
        @Volatile var forceRestore = false
        @Volatile var locked = false
        @Volatile var revision = 0L
        @Volatile var shown = false
        var lockShown = false // main only
        var offset = 0.0 // main only
        var original: Double? = null // remaining fields: worker only
        var initial = 0.0
        var dirty = false
        val pending = AtomicReference<Request?>()
        val queued = AtomicBoolean()
    }
    private var current: Gesture? = null // main only; never used by the worker
    private val debts = mutableListOf<Gesture>() // worker only; failed restores remain retryable
    private var stopped = false
    private val terminated = CompletableFuture<Unit>()

    fun start(x: Float, y: Float, base: Double) {
        if (stopped || current?.let { it.down || it.locked } == true) return
        val port = host.capture() ?: return host.report(null, "当前播放内核不支持长按倍速")
        val g = Gesture(port, base, x, y)
        current = g
        host.render(false, false, null)
        worker.execute {
            if (g.forceRestore || !g.down) return@execute
            if (!restoreAll()) {
                report(port, "长按倍速启动失败", IllegalStateException("上一次临时倍速尚未恢复"))
                finish(g)
                return@execute
            }
            val ok = attempt(g, "长按倍速启动失败") {
                require(base.isFinite() && base in 0.25..4.0) { "配置的起步倍速无效" }
                g.original = read(g.port)
                g.initial = max(g.original!!, base)
                // 松手可发生在 read 内；不允许迟到 enter 写入或弹出胶囊。
                if (!g.down || g.forceRestore) return@attempt
                change(g, g.initial)
                publish(g, 0L, g.initial)
            }
            if (!ok) abort(g)
        }
    }

    fun drag(x: Float, y: Float, allowSpeed: Boolean, allowLock: Boolean, lockPx: Float) {
        val g = current ?: return
        if (!g.down || !g.shown || g.locked || g.forceRestore) return
        if (allowLock && g.y - y >= lockPx) g.locked = true
        val offset = if (allowSpeed && !g.locked) round((x - g.x) / 320.0 * 20) / 20 else g.offset
        if (offset == g.offset && !g.locked) return
        g.offset = offset
        g.pending.set(Request(offset, ++g.revision))
        scheduleUpdate(g)
    }

    fun end(keep: Boolean) {
        val g = current ?: return
        if (!g.down) return
        g.down = false
        host.render(false, g.lockShown, null)
        worker.execute {
            if (g.locked && !g.forceRestore) return@execute
            // keep 只提交已经反馈成功的会话；起步读速途中松手仍须恢复。
            if (keep && g.shown && !g.forceRestore) {
                val ok = update(g, publish = false)
                if (ok && !g.forceRestore) { g.dirty = false; debts.remove(g) }
            }
            restore(g)
            finish(g)
        }
    }

    fun cancel() {
        val g = current ?: return
        invalidate(g)
        worker.execute { restore(g); finish(g) }
    }

    fun unlock() {
        val g = current ?: return
        if (!g.locked) return
        invalidate(g)
        worker.execute {
            val ok = restore(g)
            finish(g, unlocked = ok)
        }
    }

    /** 同一 worker 消费起播默认倍速；先处理临时恢复，不与手势写入争抢。 */
    fun applySpeed(value: Double) {
        if (stopped) return
        val port = host.capture() ?: return host.report(null, "当前播放内核不支持倍速")
        current?.let(::invalidate)
        current = null
        worker.execute {
            if (!restoreAll()) return@execute
            try {
                read(port) // 无法读真实状态时，不写入。
                writeVerified(port, value)
            } catch (e: Exception) { report(port, "倍速设置失败", e) }
        }
    }

    /** 回调在 worker，位于全部恢复尝试之后；在回调内 stop 捕获的旧 session，不能先 stop。 */
    fun clearSession(afterRestore: () -> Unit = {}) {
        if (stopped) { terminated.thenRun(afterRestore); return }
        current?.let(::invalidate)
        current = null
        host.render(false, false, null)
        worker.execute { restoreAll(); afterRestore() }
    }

    /** 不阻塞 UI、不 shutdownNow：已经开始的 native 写入必须经过恢复屏障。 */
    fun shutdown(afterRestore: () -> Unit = {}) {
        if (stopped) { terminated.thenRun(afterRestore); return }
        clearSession {
            try { afterRestore() } finally {
                (worker as? ExecutorService)?.shutdown()
                terminated.complete(Unit)
            }
        }
        stopped = true
    }

    private fun invalidate(g: Gesture) {
        markInvalid(g)
        host.render(false, false, null)
    }

    /** worker 侧失败收敛：标志立即生效（挡住主线程继续 drag），渲染回主线程。 */
    private fun abort(g: Gesture) {
        markInvalid(g)
        host.post { host.render(false, false, null) }
        restore(g)
        finish(g)
    }

    private fun markInvalid(g: Gesture) {
        g.forceRestore = true
        g.down = false
        ++g.revision
        g.pending.set(null)
    }

    private fun scheduleUpdate(g: Gesture) {
        if (!g.queued.compareAndSet(false, true)) return
        worker.execute {
            try {
                if (!g.forceRestore && (g.down || g.locked) && !update(g, publish = true)) abort(g)
            } finally {
                g.queued.set(false)
                if (g.pending.get() != null && !g.forceRestore && (g.down || g.locked)) scheduleUpdate(g)
            }
        }
    }

    private fun update(g: Gesture, publish: Boolean): Boolean {
        val request = g.pending.getAndSet(null) ?: return true
        return attempt(g, "长按调速失败") {
            val target = (g.initial + request.offset).coerceIn(0.5, max(3.0, g.initial))
            change(g, target)
            if (publish) publish(g, request.revision, target)
        }
    }

    private fun change(g: Gesture, target: Double) {
        checkNotNull(g.original) { "未读取长按前倍速" }
        g.dirty = true // 即使 write 抛异常或读回失败，仍可能已经写进内核。
        if (g !in debts) debts += g
        writeVerified(g.port, target)
    }

    private fun read(port: Port): Double {
        check(!port.closed) { "播放会话已关闭" }
        return port.read()?.takeIf { it.isFinite() && it in 0.25..4.0 }
            ?: error("无法读取有效的实际倍速")
    }

    private fun writeVerified(port: Port, target: Double) {
        require(target.isFinite() && target in 0.25..4.0) { "目标倍速无效" }
        check(!port.closed) { "播放会话已关闭" }
        check(port.write(target)) { port.failure().ifBlank { "内核拒绝设置倍速" } }
        val actual = read(port)
        check(abs(actual - target) < 0.001) { "内核实际倍速 $actual 与目标 $target 不一致" }
    }

    private fun restore(g: Gesture): Boolean {
        if (!g.dirty) return true
        if (g.port.closed) {
            report(g.port, "跳过倍速恢复", IllegalStateException("旧播放会话已关闭，无法恢复"))
            g.dirty = false
            debts.remove(g)
            return false
        }
        return attempt(g, "恢复长按前倍速失败") {
            writeVerified(g.port, checkNotNull(g.original))
            g.dirty = false
            debts.remove(g)
        }
    }

    private fun restoreAll(): Boolean {
        var ok = true
        debts.toList().forEach { if (!restore(it) && !it.port.closed) ok = false }
        return ok
    }

    private fun attempt(g: Gesture, prefix: String, action: () -> Unit): Boolean = try {
        action(); true
    } catch (e: Exception) { report(g.port, prefix, e); false }

    private fun report(port: Port, prefix: String, error: Exception) {
        val reason = "$prefix：${error.message?.takeIf { it.isNotBlank() } ?: error.javaClass.simpleName}"
        host.post { host.report(port.identity, reason) }
    }

    private fun publish(g: Gesture, revision: Long, speed: Double) {
        host.post {
            if (current !== g || g.forceRestore || (!g.down && !g.locked) ||
                revision != g.revision || host.currentIdentity() !== g.port.identity) return@post
            val started = !g.shown
            val locked = g.locked && !g.lockShown
            g.shown = true
            g.lockShown = g.locked
            host.render(g.down, g.locked, speed)
            host.feedback(started, locked)
        }
    }

    private fun finish(g: Gesture, unlocked: Boolean = false) {
        host.post {
            if (current !== g || host.currentIdentity() !== g.port.identity) return@post
            current = null
            host.render(false, false, null)
            if (unlocked) host.unlocked()
        }
    }
}
