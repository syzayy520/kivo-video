package com.kivo.cinema.playback

import android.content.Context
import android.os.Handler
import android.os.Looper
import android.view.Surface
import com.kivo.cinema.mpv.LocalRangeServer
import com.kivo.cinema.mpv.MpvSession
import com.kivo.cinema.net.ByteSource
import java.io.File
import java.util.concurrent.CountDownLatch
import java.util.concurrent.ScheduledThreadPoolExecutor
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicLong

/**
 * 253-P3：**libmpv 内核**的播放会话。
 *
 * ## 它解决什么
 * 旧内核（自研 FFmpeg demux + MediaCodec + AAudio 拼装）在真机上暴露过三个问题：
 *  · 软解路径的媒体时钟在 0.31×~1.33× 之间乱跳（用户说的"卡"）；
 *  · 视频与音频是**两套** demux，各自 seek、争用同一条读路径；
 *  · VC-1 / TrueHD / DoP 这些平台没有的解码器要自己软解 + 自己管时钟。
 *
 * mpv（SenPlayer / Rex 用的那颗）把这三件事一次解决：单一 demux 线程 +
 * 自己的时钟管理 + 全 FFmpeg 解码覆盖。
 *
 * ## 与旧内核的关系
 * **两条链路完全独立**，由播放页的引擎开关选择，互不引用。
 * 旧内核见 `com.kivo.cinema.playback.NetworkPlaybackSession`。
 *
 * ## 对外契约
 * 与 [NetworkPlaybackSession] **同名同义**（State 枚举、start/seekTo/setPaused/
 * stop/awaitClosed…），这样播放页可以用一个接口面对两种内核。
 * 本类**不含任何播放策略**（那是上层的事），也不含任何 UI。
 *
 * ## 传输层
 * mpv 通过 FFmpeg 的 `http` 协议读网络资源；而我们的 SMB 读层（块窗口/预读/
 * 断网恢复）比 FFmpeg 的 libsmbclient 强。两者用 [LocalRangeServer] 接起来：
 * ```
 * mpv ← http://127.0.0.1:port/<id> ← 代理 ← readAt ← RangeSource ← NAS
 * ```
 */
class MpvPlaybackSession(context: Context, private val input: ByteSource) {

    /** 与旧内核同名，便于播放页统一处理。 */
    enum class State { IDLE, PREPARING, PLAYING, PAUSED, ENDED, FAILED, CLOSED }

    private val app = context.applicationContext
    private val main = Handler(Looper.getMainLooper())
    private val worker = ScheduledThreadPoolExecutor(1) { r -> Thread(r, "KivoMpvSession") }
    private val closing = AtomicBoolean(false)
    private val closed = CountDownLatch(1)

    @Volatile var state = State.IDLE; private set
    @Volatile var lastError: String = ""; private set
    @Volatile var positionUs = 0L; private set
    @Volatile var durationUs = 0L; private set
    @Volatile var hasAudio = false; private set
    @Volatile var codecName = ""; private set

    private var session: MpvSession? = null
    private var server: LocalRangeServer? = null
    private var registeredId: String? = null

    /** 上一次 seek 的落点，供上层显示"实际跳到哪"。 */
    @Volatile var lastSeekLandingUs = -1L; private set

    private val seekIds = AtomicLong()

    data class SeekResult(
        val requestId: Long,
        val targetUs: Long,
        val landingUs: Long,
        val error: String = "",
    )

    @Volatile var lastSeekResult = SeekResult(0, 0, -1); private set

    /**
     * 启动播放。
     *
     * @param surface 视频输出目标。
     * @param onReady 回调：0 = 成功，-1 = 失败（原因在 [lastError]）。
     */
    fun start(surface: Surface, onReady: (Int) -> Unit) {
        check(state == State.IDLE && !closing.get())
        state = State.PREPARING
        worker.execute {
            try {
                check(!closing.get()) { "播放已取消" }

                // 1) 起本地代理，把 SMB 源变成 mpv 能读的 HTTP URL。
                //    【为什么不需要 open()】ByteSource 的语义是"打开即已就绪"
                //    （SmbByteSource.open() 里已经建好连接并拿到了大小），
                //    而 RangeSource 那种"分阶段 open"的形态是给 fd 路径用的。
                val proxy = LocalRangeServer.start()
                server = proxy
                val id = proxy.register(input)
                registeredId = id
                val url = proxy.urlFor(id)

                // 2) 起内核。
                val native = MpvSession.create(app)
                session = native
                native.initialize(
                    vo = "libmpv",
                    ao = "audiotrack",
                    logFile = File(app.filesDir, "mpv-kernel.log").absolutePath,
                )
                native.attachSurface(surface)
                native.loadFile(url)

                // 3) 等首帧出来（mpv 的时间位置从 0 开始推进）。
                val deadline = System.currentTimeMillis() + 30_000
                var position = -1.0
                while (System.currentTimeMillis() < deadline) {
                    if (closing.get()) throw IllegalStateException("播放已取消")
                    Thread.sleep(100)
                    position = native.positionSeconds()
                    if (position > 0.0) break
                }
                check(position > 0.0) {
                    "内核起播超时（position=$position）"
                }

                state = State.PLAYING
                positionUs = (position * 1_000_000).toLong()
                main.post { if (!closing.get()) onReady(0) }

                // 4) 轮询进度。
                worker.scheduleWithFixedDelay({ poll() }, 0, 200, TimeUnit.MILLISECONDS)
            } catch (e: Exception) {
                if (!closing.get()) {
                    fail("内核播放准备失败（${e.javaClass.simpleName}）")
                }
                releaseNative()
                main.post { if (!closing.get()) onReady(-1) }
            }
        }
    }

    private fun poll() {
        if (closing.get() || state !in setOf(State.PLAYING, State.PAUSED)) return
        val native = session ?: return
        try {
            val seconds = native.positionSeconds()
            if (seconds >= 0.0) {
                positionUs = (seconds * 1_000_000).toLong()
            }
        } catch (e: Exception) {
            // 轮询失败不立即判死：内核可能刚好在 seek。等下一次。
        }
    }

    fun setPaused(paused: Boolean) {
        if (state == State.PAUSED && paused) return
        if (state == State.PLAYING && !paused) return
        val native = session ?: return
        try {
            native.command("set", if (paused) "pause yes" else "pause no")
            state = if (paused) State.PAUSED else State.PLAYING
        } catch (e: Exception) {
            fail("播放控制失败（${e.javaClass.simpleName}）")
        }
    }

    /** 跳到指定位置（微秒）。返回请求 id；实际落点看 [lastSeekLandingUs]。 */
    fun seekTo(targetUs: Long): Long {
        val requestId = seekIds.incrementAndGet()
        val native = session
        if (native == null || state !in setOf(State.PLAYING, State.PAUSED)) {
            lastError = "当前播放状态无法调整进度"
            lastSeekResult = SeekResult(requestId, targetUs, -1, lastError)
            return requestId
        }
        return try {
            native.seek(targetUs / 1_000_000.0, exact = false)
            // mpv 的 seek 是异步的：落点要等内核真的跳过去才能读到。
            // 这里给一小段等待，拿到实际落点供上层显示。
            val deadline = System.currentTimeMillis() + 1_500
            var landing = targetUs
            while (System.currentTimeMillis() < deadline) {
                Thread.sleep(100)
                val seconds = native.positionSeconds()
                if (seconds >= 0.0) {
                    landing = (seconds * 1_000_000).toLong()
                    if (kotlin.math.abs(landing - targetUs) < 3_000_000) break
                }
            }
            lastSeekLandingUs = landing
            positionUs = landing
            lastError = ""
            lastSeekResult = SeekResult(requestId, targetUs, landing)
            requestId
        } catch (e: Exception) {
            lastError = "调整进度失败（${e.javaClass.simpleName}）"
            lastSeekResult = SeekResult(requestId, targetUs, -1, lastError)
            requestId
        }
    }

    fun rebind(surface: Surface?) {
        val native = session ?: return
        if (surface == null) return
        runCatching { native.attachSurface(surface) }
    }

    /** 内核状态摘要，供诊断用。 */
    fun currentSummary(): String = session?.positionSeconds()?.toString().orEmpty()

    fun stop(afterClose: () -> Unit = {}) {
        if (!closing.compareAndSet(false, true)) return
        Thread({
            worker.execute {
                try {
                    releaseNative()
                } finally {
                    state = State.CLOSED
                    worker.shutdown()
                    closed.countDown()
                    afterClose()
                }
            }
        }, "KivoMpvCancel").start()
    }

    fun awaitClosed(timeoutMs: Long): Boolean = closed.await(timeoutMs, TimeUnit.MILLISECONDS)

    private fun releaseNative() {
        // 顺序要紧：先销毁内核（它会释放 mpv 渲染上下文），再关代理与源。
        runCatching { session?.close() }
        session = null
        registeredId?.let { id -> runCatching { server?.release(id) } }
        registeredId = null
        runCatching { server?.close() }
        server = null
        // ByteSource 的 close() 会关掉底层 SMB 会话；由持有它的调用方决定何时关，
        // 这里不关 —— 播放页还要用它做别的（比如投屏）。
    }

    private fun fail(reason: String) {
        lastError = reason
        state = State.FAILED
    }
}
