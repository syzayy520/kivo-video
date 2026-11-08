package com.kivo.cinema.playback

import android.content.Context
import android.media.MediaExtractor
import android.media.MediaFormat
import android.os.Handler
import android.os.Looper
import android.view.Surface
import com.kivo.cinema.bridge.PlaybackSession
import com.kivo.cinema.net.HttpRangeFile
import java.io.File
import java.util.concurrent.CountDownLatch
import java.util.concurrent.ScheduledThreadPoolExecutor
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean

/** Serialized network adapter around the same native A/V session used for local media. */
class NetworkPlaybackSession(context: Context, private val url: String) {
    enum class State { IDLE, PREPARING, PLAYING, PAUSED, ENDED, FAILED, CLOSED }
    private val app = context.applicationContext
    private val main = Handler(Looper.getMainLooper())
    private val worker = ScheduledThreadPoolExecutor(1) { r -> Thread(r, "KivoNetworkSession") }
    private val closing = AtomicBoolean(false)
    private val closed = CountDownLatch(1)
    private var session: PlaybackSession? = null // worker owns every native operation
    @Volatile private var source: HttpRangeFile? = null
    @Volatile var state = State.IDLE; private set
    @Volatile var positionUs = 0L; private set
    @Volatile var durationUs = 0L; private set
    @Volatile var lastError = ""; private set
    @Volatile var width = 0; private set
    @Volatile var height = 0; private set
    @Volatile var hasAudio = false; private set
    @Volatile var codecName = ""; private set
    private val curve = mutableListOf<Pair<Long, Double>>()
    private var beganNs = 0L

    fun paceCurve(): List<Pair<Long, Double>> = synchronized(curve) { curve.toList() }

    /** Returns immediately. Callback is delivered on main only while this session remains owned. */
    fun start(surface: Surface, onReady: (Int) -> Unit) {
        check(state == State.IDLE && !closing.get())
        state = State.PREPARING
        worker.execute {
            try {
                val input = HttpRangeFile(app, url)
                source = input
                if (closing.get()) input.cancel()
                input.prepare()
                check(!closing.get()) { "播放已取消" }
                probe(input)
                check(!closing.get() && surface.isValid) { "播放窗口已关闭" }
                val native = PlaybackSession(0, File("network-media"), "network-media",
                    descriptorSource = { input.openDescriptor() }, sourceLength = input.size)
                session = native
                checked(native.openVideo(surface), native)
                if (hasAudio) checked(native.openAudio(), native)
                checked(native.configurePacing(0, true), native)
                checked(native.configurePipeline(false, true, true), native)
                check(!closing.get()) { "播放已取消" }
                checked(native.start(), native)
                state = State.PLAYING
                codecName = native.summary().substringAfter("codec=").substringBefore(" ")
                beganNs = System.nanoTime()
                main.post { if (!closing.get()) onReady(0) }
                worker.scheduleWithFixedDelay({ poll() }, 0, 100, TimeUnit.MILLISECONDS)
            } catch (e: Exception) {
                if (!closing.get()) fail(source?.failureReason?.takeIf { it.isNotBlank() }
                    ?: if (e is PlaybackFailure) e.message!! else "网络播放准备失败（${e.javaClass.simpleName}）")
                releaseNative()
                main.post { if (!closing.get()) onReady(-1) }
            }
        }
    }

    private fun probe(input: HttpRangeFile) {
        input.openDescriptor().use { fd ->
            val extractor = MediaExtractor()
            try {
                extractor.setDataSource(fd.fileDescriptor, 0, input.size)
                if (extractor.trackCount == 0) throw PlaybackFailure("系统无法识别片源容器或媒体轨道，请尝试其他片源")
                var video = false
                for (i in 0 until extractor.trackCount) {
                    val format = extractor.getTrackFormat(i)
                    val mime = format.getString(MediaFormat.KEY_MIME) ?: continue
                    if (mime.startsWith("audio/")) hasAudio = true
                    if (!video && mime.startsWith("video/")) {
                        video = true
                        width = format.getInteger(MediaFormat.KEY_WIDTH)
                        height = format.getInteger(MediaFormat.KEY_HEIGHT)
                        durationUs = if (format.containsKey(MediaFormat.KEY_DURATION))
                            format.getLong(MediaFormat.KEY_DURATION) else 0L
                    }
                }
                if (!video) throw PlaybackFailure("片源没有可播放的视频轨道")
            } finally { extractor.release() }
        }
    }

    private fun poll() {
        if (closing.get() || state !in setOf(State.PLAYING, State.PAUSED)) return
        try {
            val native = session ?: return
            positionUs = native.positionUs().coerceAtLeast(0)
            val summary = native.summary()
            source?.failureReason?.takeIf { it.isNotBlank() }?.let { throw PlaybackFailure(it) }
            if (!summary.contains("error=-")) throw PlaybackFailure("网络解码失败：" + summary.substringAfter("error="))
            if (state == State.PLAYING && summary.contains("completed=1")) {
                state = State.ENDED
                releaseNative()
            }
            synchronized(curve) {
                if (curve.size < 600 && (curve.isEmpty() ||
                    (System.nanoTime() - beganNs) / 1_000_000 - curve.last().first >= 1000))
                    curve += (System.nanoTime() - beganNs) / 1_000_000 to positionUs / 1e6
            }
        } catch (e: Exception) {
            fail(if (e is PlaybackFailure) e.message!! else "网络播放失败（${e.javaClass.simpleName}）")
            releaseNative()
        }
    }

    fun setPaused(paused: Boolean) = command {
        if (state == State.PAUSED && paused || state == State.PLAYING && !paused) return@command
        val native = session ?: return@command
        checked(if (paused) native.pause() else native.resume(), native)
        state = if (paused) State.PAUSED else State.PLAYING
        lastError = ""
        positionUs = native.positionUs().coerceAtLeast(0)
    }

    fun seekTo(targetUs: Long) = command {
        // The current native API rejects seek while paused; report it instead of emitting a burst of sound.
        if (state == State.PAUSED) {
            lastError = "请先继续播放再调整进度"
            return@command
        }
        val native = session ?: return@command
        lastError = ""
        checked(native.seekTo(targetUs.coerceIn(0, (durationUs - 1).coerceAtLeast(0))), native)
        positionUs = native.positionUs().coerceAtLeast(0)
    }

    fun rebind(surface: Surface?) = command {
        val native = session ?: return@command
        checked(native.rebind(surface), native)
    }

    private fun command(action: () -> Unit) {
        if (closing.get()) return
        try { worker.execute {
            if (closing.get() || state !in setOf(State.PLAYING, State.PAUSED)) return@execute
            try { action() } catch (e: Exception) {
                fail(if (e is PlaybackFailure) e.message!! else "播放控制失败（${e.javaClass.simpleName}）")
                releaseNative()
            }
        } } catch (_: java.util.concurrent.RejectedExecutionException) {
            // Teardown may shut down the executor between the ownership check and enqueue.
            if (!closing.get()) throw IllegalStateException("播放工作队列已关闭")
        }
    }

    /** No UI join: cancel I/O first, then serialize native teardown before fd release. */
    fun stop(afterClose: () -> Unit = {}) {
        if (!closing.compareAndSet(false, true)) return
        Thread({
            source?.cancel()
            worker.execute {
                try { releaseNative() } finally {
                    state = State.CLOSED
                    worker.shutdown()
                    try { afterClose() } finally { closed.countDown() }
                }
            }
        }, "KivoNetworkCancel").start()
    }

    fun awaitClosed(timeoutMs: Long): Boolean = closed.await(timeoutMs, TimeUnit.MILLISECONDS)

    private fun releaseNative() {
        val native = session
        session = null
        try {
            native?.let {
                try {
                    it.stop()
                    val evidence = File(app.filesDir, "evidence/network-native-sync.json")
                    evidence.parentFile?.mkdirs()
                    it.writeSyncStats(evidence.absolutePath)
                } finally { it.close() }
            }
        } finally {
            source?.close()
            source = null
        }
    }

    private fun fail(reason: String) { lastError = reason; state = State.FAILED }
    private fun checked(code: Int, native: PlaybackSession) {
        if (code != 0) throw PlaybackFailure(native.lastError.ifBlank { "播放核心返回错误 $code" })
    }
    private class PlaybackFailure(message: String) : Exception(message)
}
