package com.kivo.cinema.playback

import android.content.Context
import android.media.MediaExtractor
import android.media.MediaFormat
import android.os.Handler
import android.os.Looper
import android.system.Os
import android.util.Log
import android.view.Surface
import com.kivo.cinema.bridge.PlaybackSession
import com.kivo.cinema.net.HttpRangeFile
import com.kivo.cinema.net.RangeSource
import com.kivo.cinema.net.NetworkReadTrace
import java.io.File
import java.io.FileDescriptor
import java.util.concurrent.CountDownLatch
import java.util.concurrent.ScheduledThreadPoolExecutor
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicLong

/** Serialized network adapter around the same native A/V session used for local media. */
class NetworkPlaybackSession(context: Context, private val input: RangeSource) {
    /** 便利构造：HTTP Range 片源（Emby / WebDAV / 通用 http）。 */
    constructor(context: Context, url: String) :
        this(context, HttpRangeFile(context.applicationContext, url))
    enum class State { IDLE, PREPARING, PLAYING, PAUSED, ENDED, FAILED, CLOSED }
    private val app = context.applicationContext
    private val main = Handler(Looper.getMainLooper())
    private val worker = ScheduledThreadPoolExecutor(1) { r -> Thread(r, "KivoNetworkSession") }
    private val closing = AtomicBoolean(false)
    private val closed = CountDownLatch(1)
    private val closeLock = Any()
    private val closeListeners = mutableListOf<() -> Unit>()
    private var closeComplete = false
    private var session: PlaybackSession? = null // worker owns every native operation
    @Volatile private var source: RangeSource? = null
    @Volatile var state = State.IDLE; private set
    @Volatile var positionUs = 0L; private set
    @Volatile var durationUs = 0L; private set
    @Volatile var lastError = ""; private set
    @Volatile var width = 0; private set
    @Volatile var height = 0; private set
    @Volatile var hasAudio = false; private set
    /**
     * 最近一次轮询到的原生会话摘要（codec/帧账/静音账/节拍 p99）。
     *
     * 暴露它的理由：长稳失败时要能立刻判断"是播放停了还是采样器晚了"，
     * 而音频补静音、xrun、逐帧节拍这些数字只有 summary 里有。
     * `poll()` 每 100 ms 已经在读它，缓存下来不增加任何 IO。
     */
    @Volatile var lastSummary = ""
        private set
    /** 容器是否为 MKV/WebM：这类片源由 native FFmpeg 解封装，平台 probe 不再参与判定。 */
    @Volatile private var matroskaSource = false
    /**
     * 片源正在等网络回来（由 [RangeSource.stalled] 转发）。
     *
     * 与 [lastError] 的区别：这是**暂时**状态，界面应显示"正在重连"而不是"播放失败"。
     * 只有超过片源层的耐心窗口，才会转成 [State.FAILED]。
     */
    @Volatile var buffering = false; private set
    @Volatile var codecName = ""; private set
    data class SeekResult(val requestId: Long, val targetUs: Long, val landingUs: Long,
                          val error: String = "")
    private val seekIds = AtomicLong()
    @Volatile var lastSeekResult: SeekResult? = null; private set
    private val curve = mutableListOf<Pair<Long, Double>>()
    private var beganNs = 0L
    /**
     * 硬解节拍日志上一次输出的时刻（ns）。
     *
     * 存在的理由：硬解这条链路此前**一行运行期日志都没有** —— `summary()` 里
     * 明明已经有 `fps / speed / dropped / pace_lateness_p99`，但只在出错时才被读，
     * 结果是"用户说卡、我这边看不到任何数字"，只能靠肉眼和截图互相猜。
     * 每 2 s 打一行，量级与一次 `dumpsys` 相当，不影响播放。
     */
    private var lastRateLogNs = 0L
    private val readTrace = if (com.kivo.cinema.BuildConfig.DEBUG) NetworkReadTrace() else null

    fun paceCurve(): List<Pair<Long, Double>> = synchronized(curve) { curve.toList() }
    fun ioTrace(): List<String> = readTrace?.snapshot() ?: emptyList()

    /** Returns immediately. Callback is delivered on main only while this session remains owned. */
    fun start(surface: Surface, onReady: (Int) -> Unit) {
        check(state == State.IDLE && !closing.get())
        state = State.PREPARING
        worker.execute {
            try {
                val pending = input
                source = pending
                if (closing.get()) pending.cancel()
                pending.open()
                check(!closing.get()) { "播放已取消" }
                probe(pending)
                check(!closing.get() && surface.isValid) { "播放窗口已关闭" }
                val native = PlaybackSession(0, File("network-media"), "network-media",
                    descriptorSource = { pending.openDescriptor() }, sourceLength = pending.size)
                session = native
                checked(native.openVideo(surface), native)
                // MKV 由 native 解封装，平台 probe 拿不到时长。不回填的话
                // 播放器会一直显示 `0:00:00 / 0:00:00`，进度条不可用。
                if (durationUs <= 0) {
                    native.summary().substringAfter("dur=", "").substringBefore(" ")
                        .toLongOrNull()?.takeIf { it > 0 }?.let { durationUs = it }
                }
                if (hasAudio || matroskaSource) {
                    val audioCode = native.openAudio(prefillTimeoutMs = 8000)
                    if (audioCode == 0) {
                        // MKV 路径没有平台 probe，音频事实只能由 native 打开结果反推：
                        // 打开成功就是有音轨，不把 hasAudio 留成 false 让上层以为无声。
                        hasAudio = true
                    } else {
                        // MKV 没有平台 probe 兜底，只能用 native 的结论区分两种情形：
                        // 「片源本来就没有音轨」是属性，可以无声播放；
                        // 「有音轨但打不开」是故障，不能静默继续。
                        val noTrack = native.lastError.contains("no audio track", ignoreCase = true)
                        if (hasAudio || !noTrack) {
                            checked(audioCode, native)
                        }
                        lastError = native.lastError
                    }
                }
                checked(native.configurePacing(0, true), native)
                checked(native.configurePipeline(false, true, true, asyncInput = true), native)
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

    /**
     * EBML 魔数 `1A 45 DF A3`。只读 4 字节，不依赖平台解析能力。
     *
     * 判错的代价只是"该走 FFmpeg 的没走"，会立刻在轨道数上暴露；
     * 反过来把非 MKV 判成 MKV 才会真的播错，所以这里只认确定的魔数。
     */
    private fun isMatroska(fd: FileDescriptor): Boolean {
        return try {
            val magic = ByteArray(4)
            val got = Os.pread(fd, magic, 0, magic.size, 0)
            got == magic.size && magic[0] == 0x1A.toByte() && magic[1] == 0x45.toByte() &&
                magic[2] == 0xDF.toByte() && magic[3] == 0xA3.toByte()
        } catch (e: Exception) {
            false
        }
    }

    private fun probe(input: RangeSource) {
        input.openDescriptor().use { fd ->
            // MKV/WebM：Android 13 的平台 MediaExtractor 不解析 SeekHead→Cues，
            // 对 7.37 GB 片源线性扫描 45 秒后报 0 轨（证据 fd-semantics-r1.md）。
            // 这里不再用它预判——容器判定交给 native 的 FFmpeg 解封装路径，
            // 否则会在真正能播的片源上先被判死。
            if (isMatroska(fd.fileDescriptor)) {
                matroskaSource = true
                return
            }
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
            if (summary.isNotEmpty()) lastSummary = summary
            // 每 2 s 一行硬解实测节拍：fps / speed / dropped / pace 迟到 p99。
            // 只在 DEBUG 打，生产包不产生任何日志开销。
            if (com.kivo.cinema.BuildConfig.DEBUG) {
                val nowNs = System.nanoTime()
                if (nowNs - lastRateLogNs >= 2_000_000_000L) {
                    lastRateLogNs = nowNs
                    Log.i("KivoHardRate", summary)
                }
            }
            buffering = source?.stalled == true
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

    fun seekTo(targetUs: Long): Long {
        val requestId = seekIds.incrementAndGet()
        command(onRejected = {
            lastSeekResult = SeekResult(requestId, targetUs, -1, "当前播放状态无法调整进度")
        }) {
            // The native API rejects seek while paused; never resume sound implicitly.
            if (state == State.PAUSED) {
                lastError = "请先继续播放再调整进度"
                lastSeekResult = SeekResult(requestId, targetUs, -1, lastError)
                return@command
            }
            val target = targetUs.coerceIn(0, (durationUs - 1).coerceAtLeast(0))
            try {
                val native = session ?: throw PlaybackFailure("播放会话已关闭")
                checked(native.seekTo(target), native)
                check(native.lastSeekLandingUs >= 0) { "核心未返回实际进度落点" }
                positionUs = native.lastSeekLandingUs
                lastError = ""
                lastSeekResult = SeekResult(requestId, target, native.lastSeekLandingUs)
            } catch (e: Exception) {
                lastSeekResult = SeekResult(requestId, target, -1,
                    if (e is PlaybackFailure) e.message!! else "调整进度失败")
                throw e
            }
        }
        return requestId
    }

    fun rebind(surface: Surface?) = command {
        val native = session ?: return@command
        checked(native.rebind(surface), native)
    }

    private fun command(onRejected: () -> Unit = {}, action: () -> Unit) {
        if (closing.get()) { onRejected(); return }
        try { worker.execute {
            if (closing.get() || state !in setOf(State.PLAYING, State.PAUSED)) {
                onRejected()
                return@execute
            }
            try { action() } catch (e: Exception) {
                fail(if (e is PlaybackFailure) e.message!! else "播放控制失败（${e.javaClass.simpleName}）")
                releaseNative()
            }
        } } catch (_: java.util.concurrent.RejectedExecutionException) {
            // Teardown may shut down the executor between the ownership check and enqueue.
            onRejected()
            if (!closing.get()) throw IllegalStateException("播放工作队列已关闭")
        }
    }

    /** No UI join: cancel I/O first, then serialize native teardown before fd release. */
    fun stop(afterClose: () -> Unit = {}) {
        val completed = synchronized(closeLock) {
            if (closeComplete) true else { closeListeners += afterClose; false }
        }
        if (completed) { notifyCloseListener(afterClose); return }
        if (!closing.compareAndSet(false, true)) return
        Thread({
            cleanupStep("取消网络读取") { source?.cancel() }
            // Only this queued task shuts down worker, so it stays available until enqueue.
            worker.execute {
                try { releaseNative() } finally {
                    state = State.CLOSED
                    worker.shutdown()
                    val listeners = synchronized(closeLock) {
                        closeComplete = true
                        closeListeners.toList().also { closeListeners.clear() }
                    }
                    try {
                        listeners.forEach(::notifyCloseListener)
                    } finally { closed.countDown() }
                }
            }
        }, "KivoNetworkCancel").start()
    }

    fun awaitClosed(timeoutMs: Long): Boolean = closed.await(timeoutMs, TimeUnit.MILLISECONDS)

    private fun releaseNative() {
        // 取走并置空**必须在锁内**。
        //
        // 原来的 `val native = session; session = null` 不是原子的：`stop()` 的收尾
        // 线程与 Activity 的 `onDestroy` 可能同时进来，各自读到非空，于是对同一个
        // native 做两次 stop，第二次的 `writeSyncStats` 在已释放的句柄上返回非 0，
        // 被 `check(... == 0)` 抛成 `IllegalStateException`。
        // 20 秒轮次几乎不重叠所以看不见；8 分钟轮次稳定复现 teardown 失败。
        val native = synchronized(closeLock) { session.also { session = null } }
        // 注意：`native == null` **不能提前 return**。
        // 提前返回会跳过下面的 finally，`source?.close()` 就不再执行 ——
        // 那是一条 fd + 8 MiB 缓存 + 2 条响应环的泄漏，而且只在"重复收尾"时出现，
        // 20 秒轮次几乎撞不到。判空必须放在 try 内部。
        try {
            // Failure teardown must unblock proxy reads before native stop joins them.
            cleanupStep("取消网络读取") { source?.cancel() }
            if (native != null) {
                try {
                    cleanupStep("停止播放核心") { native.stop() }
                    cleanupStep("保存播放诊断") {
                        val evidence = File(app.filesDir, "evidence/network-native-sync.json")
                        evidence.parentFile?.mkdirs()
                        // 失败时把返回码和原生侧的原因一起留下。
                        // 原来只写 `check(rc == 0)`，异常消息被测试的隐私过滤吞掉，
                        // 于是长会话退出失败只能看到 "Check failed."，无法归因。
                        val rc = native.writeSyncStats(evidence.absolutePath)
                        if (rc != 0) {
                            lastError = "sync stats rc=$rc native=${native.lastError}"
                            error("writeSyncStats failed")
                        }
                    }
                } finally { cleanupStep("释放播放核心") { native.close() } }
            }
        } finally {
            cleanupStep("关闭网络文件") { source?.close() }
            source = null
            buffering = false
        }
    }

    private fun cleanupStep(operation: String, action: () -> Unit) {
        try { action() } catch (e: Exception) {
            val reason = "${operation}失败（${e.javaClass.simpleName}）"
            synchronized(closeLock) {
                lastError = if (lastError.isBlank()) reason else "$lastError；$reason"
            }
            // Exception messages may contain authenticated URLs.
            android.util.Log.e("KivoNetworkSession", reason)
        }
    }

    private fun notifyCloseListener(listener: () -> Unit) {
        try { listener() } catch (e: Exception) {
            android.util.Log.e("KivoNetworkSession", "Close listener failed (${e.javaClass.simpleName})")
        }
    }

    private fun fail(reason: String) { lastError = reason; state = State.FAILED; buffering = false }
    private fun checked(code: Int, native: PlaybackSession) {
        if (code != 0) throw PlaybackFailure(native.lastError.ifBlank { "播放核心返回错误 $code" })
    }
    private class PlaybackFailure(message: String) : Exception(message)
}
