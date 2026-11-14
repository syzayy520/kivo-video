package com.kivo.cinema.playback

import android.content.Context
import android.os.Handler
import android.os.Looper
import android.view.Surface
import com.kivo.cinema.mpv.LocalRangeServer
import com.kivo.cinema.mpv.MpvSession
import com.kivo.cinema.mpv.MpvSpeedController
import com.kivo.cinema.mpv.MpvTrackCatalog
import com.kivo.cinema.net.ByteSource
import java.io.File
import java.util.concurrent.CountDownLatch
import java.util.concurrent.ScheduledThreadPoolExecutor
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean

/**
 * 253-P3：**libmpv 内核**的播放会话。
 *
 * ## 它是什么
 * 旧内核（自研 FFmpeg demux + MediaCodec + AAudio 拼装）在真机上暴露过三个问题：
 * 软解路径的媒体时钟乱跳（用户说的"卡"）、音视频**两套** demux 各自 seek、
 * 平台没有的解码器要自己软解自己管时钟。mpv 用"单一 demux + 自己的时钟 +
 * 全 FFmpeg 解码覆盖"一次解决。
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
 * ## 职责边界（273 v16 拆出来的家族树）
 * · **起播流水线** → [MpvStartSequence]（代理 → 内核 → 闸门 → 第一跳 → 首帧）。
 * · **首帧口径** → [MpvFirstFrameWait]（画面真上屏才算，不看时钟起跳）。
 * · **seek 落点** → [MpvSeekControl]（mpv 的 seek 是异步的，落点要读回来）。
 * · **字幕语义** → [MpvSubtitleControl]（mpv 的字幕规则与会话生命周期无关）。
 * · 本类只管**状态**（播放中/暂停/进度/时长）与**释放**。
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
    private val prefs by lazy { PlaybackPrefs(app) }
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

    /**
     * 内核能力门面（273 v16 拆出来的）。
     *
     * 轨道/倍速/字幕/原始命令的**语义**都在 [MpvKernelApi] 里 ——
     * 它们跟着 mpv 的属性定义走，与会话生命周期无关。
     */
    private val kernel = MpvKernelApi({ session }) { lastError = it }

    /** 253-P6-3：轨道目录（音轨/字幕轨枚举与选轨）。 */
    val tracks: MpvTrackCatalog get() = kernel.tracks

    /** 253-P6-3：倍速控制。 */
    val speed: MpvSpeedController get() = kernel.speed

    /**
     * 253-P6-2 诊断：**直接读 mpv 的原生属性**（会话没起时抛异常）。
     *
     * 【为什么需要】结构化视图（轨道/倍速）看不出内核的真实状态，
     * 排查问题只能读原始属性（`sub-start` / `sub-end` / `osd-dimensions` 的子属性）。
     */
    fun native(): MpvSession = kernel.native()

    /** 原生会话的**安全**版本：会话还没起时返回 null（UI 轮询用，会抛会崩 Activity）。 */
    fun nativeOrNull(): MpvSession? = kernel.nativeOrNull()

    /** 轨道目录的**安全**版本：会话还没起时返回 null（面板随时可能被打开）。 */
    fun tracksOrNull(): MpvTrackCatalog? = kernel.tracksOrNull()

    /** 253-P6-2：字幕是否已启用（mpv 的 `sid` 有值即为启用）。 */
    val subtitlesEnabled: Boolean get() = kernel.subtitlesEnabled

    /** 上一次 seek 的落点，供上层显示"实际跳到哪"。 */
    @Volatile var lastSeekLandingUs = -1L; private set

    /** 上一次 seek 的结果（落点或失败原因）。 */
    @Volatile var lastSeekResult = MpvSeekControl.Result(0, 0, -1); private set

    /** seek 语义代理（273 v16 拆出来的）。 */
    private val seeks = MpvSeekControl { session }

    /**
     * 启动播放。
     * @param onReady 回调：0 = 成功，-1 = 失败（原因在 [lastError]）。
     * @param startSeconds > 0 = 续播位（秒）；<= 0 = 从头播。
     * @param loadGate 起播**放行闸门**（null = 不设闸门）。
     *   【为什么在 loadFile 之前才等】内核创建（真机 39 ms）要跟源站握手**并行**，
     *   等握手这件事只挡"发第一跳"，不挡"把内核建起来"（273 v15）。
     */
    fun start(
        surface: Surface,
        onReady: (Int) -> Unit,
        startSeconds: Double = -1.0,
        loadGate: PlaybackLoadGate? = null,
    ) {
        check(state == State.IDLE && !closing.get())
        state = State.PREPARING
        worker.execute {
            try {
                check(!closing.get()) { "播放已取消" }
                val prepared = MpvStartSequence(app, input, prefs).prepare(
                    surface = surface,
                    startSeconds = startSeconds,
                    loadGate = loadGate,
                    isCancelled = { closing.get() },
                )
                session = prepared.native
                server = prepared.proxy
                registeredId = prepared.registeredId

                state = State.PLAYING
                positionUs = (prepared.positionSeconds * 1_000_000).toLong()
                // 时长：mpv 的 `duration` 属性在媒体加载完成后才有值。
                // 不回填的话进度条会一直显示 0:00:00 / 0:00:00（用户没法拖进度）。
                val duration = prepared.durationSeconds
                if (duration > 0.0) durationUs = (duration * 1_000_000).toLong()
                // 有没有音轨：问内核（`aid` = 当前音轨 id，"no" = 没有音轨）。
                // 不猜 —— 猜错了上层会显示"正在播放 · 片源没有音轨"这种假状态。
                hasAudio = prepared.native.property("aid")?.trim()
                    ?.let { it.isNotEmpty() && it != "no" } ?: false
                main.post { if (!closing.get()) onReady(0) }

                // 轮询进度。
                worker.scheduleWithFixedDelay({ poll() }, 0, 200, TimeUnit.MILLISECONDS)
            } catch (e: Exception) {
                if (!closing.get()) {
                    // 【为什么要带 e.message】只报异常类名等于什么都没说。
                    // mpv 侧的关键原因（例如"哪个选项被拒"）都在 message 里，
                    // 而 mpv 自己不会打印被拒选项的消息 —— 丢了就再也查不到。
                    // 【判决要原样透出】源站拒绝（401/403/404）不是"内核准备失败"，
                    // 包一层"内核播放准备失败（…）"会把真实原因埋掉。
                    val reason = if (e is PlaybackRejected) {
                        e.message ?: "源站拒绝了这个视频"
                    } else {
                        "内核播放准备失败（${e.javaClass.simpleName}: ${e.message}）"
                    }
                    fail(reason)
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

    /** 跳到指定位置（微秒）。exact=精确到帧；返回请求 id，实际落点看 [lastSeekLandingUs]。 */
    fun seekTo(targetUs: Long, exact: Boolean = false): Long {
        val native = session
        if (native == null || state !in setOf(State.PLAYING, State.PAUSED)) {
            val reason = "当前播放状态无法调整进度"
            lastError = reason
            lastSeekResult = MpvSeekControl.Result(seeks.nextRequestId(), targetUs, -1, reason)
            return lastSeekResult.requestId
        }
        val result = seeks.seek(targetUs, exact)
        lastSeekResult = result
        if (result.error.isEmpty()) {
            lastSeekLandingUs = result.landingUs
            positionUs = result.landingUs
            lastError = ""
        } else {
            lastError = result.error
        }
        return result.requestId
    }

    fun rebind(surface: Surface?) {
        val native = session ?: return
        if (surface == null) return
        // 不许静默吞：Surface 重建失败就是黑屏，必须留下原因（铁律：禁止静默降级）。
        try {
            native.attachSurface(surface)
            lastError = ""
        } catch (e: Exception) {
            lastError = "重建画面失败：${e.message}"
            android.util.Log.e("MpvPlaybackSession", "rebind failed", e)
        }
    }

    // ---- 253-P6-2：字幕（规则在 [MpvKernelApi] / [MpvSubtitleControl]） ----

    /** 挂载一条外挂字幕文件（.srt / .ass / .ssa）。 */
    fun loadExternalSubtitle(file: File): Boolean = kernel.loadExternalSubtitle(file)

    /** 选中一条字幕轨（轨道 id 来自 [MpvTrackCatalog.tracks]）。 */
    fun selectSubtitle(trackId: Int): Boolean = kernel.selectSubtitle(trackId)

    /** 关闭字幕。 */
    fun disableSubtitles(): Boolean = kernel.disableSubtitles()

    /** 当前字幕轨是否在**显示**（挂了 ≠ 选了 ≠ 可见，三态分开）。 */
    fun isSubtitleVisible(): Boolean = kernel.isSubtitleVisible()

    // ---- 253-P6-3：倍速与内核命令 ----

    /** 下发一条内核命令（如 `screenshot-to-file`）。 */
    fun command(name: String, args: String = ""): Boolean = kernel.command(name, args)

    /** 设置倍速（会被夹到 0.25~4.0）。 */
    fun setSpeed(ratio: Double): Boolean = kernel.setSpeed(ratio)

    /** 当前倍速。 */
    fun currentSpeed(): Double = kernel.currentSpeed()

    /** 当前倍速的**安全**版本：会话还没起时返回 null。 */
    fun currentSpeedOrNull(): Double? = kernel.currentSpeedOrNull()

    /** 内核状态摘要，供诊断用。 */
    fun currentSummary(): String = kernel.currentSummary()

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
