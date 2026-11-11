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

    /**
     * 253-P6-3：轨道目录（音轨/字幕轨枚举与选轨）。
     *
     * 【为什么是惰性的】
     * 它需要一个活着的 [MpvSession] 才能读属性，而会话在 [start] 之后才存在。
     * 所以每次访问时现取现用，不缓存 —— 轨道列表本身也会随外挂字幕变化。
     */
    val tracks: MpvTrackCatalog
        get() = MpvTrackCatalog(requireSession())

    /** 253-P6-3：倍速控制。 */
    val speed: MpvSpeedController
        get() = MpvSpeedController(requireSession())

    /**
     * 253-P6-2 诊断：**直接读 mpv 的原生属性**。
     *
     * 【为什么需要这一层】轨道目录与倍速控制器都是**面向 UI 的结构化视图**，
     * 但排查内核问题需要 mpv 的**原始属性**（`sub-start` / `sub-end` / `osd-dimensions` 的子属性）。
     * 没有这一层，就只能靠截图猜"字幕有没有内容"——
     * 而"该时刻本来就没有字幕"和"字幕没渲染出来"在截图上长得一模一样。
     *
     * 【典型用法】`native().property("sub-start")` 读字幕包的覆盖区间。
     *
     * @throws IllegalStateException 会话尚未启动或已释放。
     */
    fun native(): MpvSession = requireSession()

    /**
     * 原生会话的**安全**版本：会话还没起时返回 null 而不是抛异常。
     *
     * 【为什么需要】UI 的**轮询**会在会话建立之前就开始跑
     * （比如顶栏网速每 1 秒读一次 `cache-speed`）。
     * 用 [native]（会抛）会让轮询任务崩掉、整个 Activity 挂掉 ——
     * 这正是第一版播放页 UI 实机测试时"Activity 起来又退回桌面"的原因。
     */
    fun nativeOrNull(): MpvSession? = session

    /**
     * 轨道目录的**安全**版本：会话还没起时返回 null 而不是抛异常。
     *
     * 【为什么需要】UI 的面板是**随时可能被打开**的 ——
     * 用户在起播完成前点"音轨"就会走到这里。
     * 用 [tracks]（会抛）会让面板直接崩掉；用这个则只是"暂时没有可选项"。
     */
    fun tracksOrNull(): MpvTrackCatalog? =
        if (session != null) MpvTrackCatalog(requireSession()) else null

    /** 253-P6-2：字幕是否已启用（mpv 的 `sid` 有值即为启用）。 */
    val subtitlesEnabled: Boolean
        get() = tracks.currentTrackId("sub") >= 0

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
                // 时长：mpv 的 `duration` 属性在媒体加载完成后才有值。
                // 不回填的话进度条会一直显示 0:00:00 / 0:00:00（用户没法拖进度）。
                val duration = native.durationSeconds()
                if (duration > 0.0) {
                    durationUs = (duration * 1_000_000).toLong()
                    hasAudio = true
                }
                main.post { if (!closing.get()) onReady(0) }

                // 4) 轮询进度。
                worker.scheduleWithFixedDelay({ poll() }, 0, 200, TimeUnit.MILLISECONDS)
            } catch (e: Exception) {
                if (!closing.get()) {
                    // 【为什么要带 e.message】只报异常类名等于什么都没说。
                    // mpv 侧的关键原因（例如"哪个选项被拒"）都在 message 里，
                    // 而 mpv 自己不会打印被拒选项的消息 —— 丢了就再也查不到。
                    fail("内核播放准备失败（${e.javaClass.simpleName}: ${e.message}）")
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

    // ---- 253-P6-2：字幕 ----

    /**
     * 挂载一条外挂字幕文件。
     *
     * 【为什么交给 mpv 而不是自己画】
     * mpv 的 `vo=libmpv` 会把字幕**画进视频帧**（渲染后端调
     * `gl_video_set_osd_source(vo->osd)`，osd 内部走 libass）。
     * 我们若再解析一遍再叠一层，等于两套字幕同时显示、两个位置。
     * 所以外挂字幕的正确接法就是把文件交给 mpv（`sub-add`）。
     *
     * @param file 本地字幕文件（.srt / .ass / .ssa）。
     * @return true = 挂载成功。
     */
    fun loadExternalSubtitle(file: File): Boolean {
        if (!file.isFile) {
            lastError = "字幕文件不存在：${file.name}"
            return false
        }
        val ok = tracks.addExternalSubtitle(file.absolutePath)
        if (!ok) {
            lastError = "字幕挂载失败：${file.name}"
        }
        return ok
    }

    /**
     * 选中一条字幕轨。
     *
     * @param trackId 轨道 id（来自 [MpvTrackCatalog.tracks]）。
     */
    fun selectSubtitle(trackId: Int): Boolean {
        val ok = tracks.selectTrack("sub", trackId)
        if (!ok) lastError = "字幕轨切换失败（id=$trackId）"
        return ok
    }

    /** 关闭字幕。 */
    fun disableSubtitles(): Boolean {
        val ok = tracks.disableTrack("sub")
        if (!ok) lastError = "关闭字幕失败"
        return ok
    }

    /**
     * 当前字幕轨是否在**显示**。
     *
     * 【为什么不能只看"选了没有"】
     * mpv 有三种状态：没挂载字幕轨、挂了但没选、选了且可见。
     * 中间那一种（挂了但没选）在界面上和"完全没字幕"是一样的 ——
     * 都看不见，但前者可以一键打开，后者要先挂文件。
     */
    fun isSubtitleVisible(): Boolean {
        val id = tracks.currentTrackId("sub")
        if (id < 0) return false
        // mpv 的 `sub-visibility` 默认是 yes；显式问一次比假设更可靠。
        return session?.property("sub-visibility") != "no"
    }

    // ---- 253-P6-3：倍速 ----

    /**
     * 下发一条内核命令。
     *
     * @param name 命令名（如 `screenshot-to-file`）。
     * @param args 参数（已按 mpv 命令行格式序列化）。
     * @return true = 命令被内核接受。
     *
     * 【为什么会话要透出命令】
     * 上层偶尔需要做"没有对应属性"的动作：截图、加载外挂字幕、
     * 切到下一条字幕轨。这些都是 mpv 的命令而不是属性。
     * 把它们藏在会话里，各调用点就不必自己去摸内核句柄。
     */
    fun command(name: String, args: String = ""): Boolean =
        runCatching { requireSession().command(name, args) }.getOrDefault(false)

    /**
     * 设置倍速。
     *
     * @param ratio 倍速（会被夹到 0.25~4.0）。
     * @return true = 设置成功。
     */
    fun setSpeed(ratio: Double): Boolean {
        val ok = speed.setSpeed(ratio)
        if (!ok) lastError = "倍速设置失败（$ratio）"
        return ok
    }

    /** 当前倍速。 */
    fun currentSpeed(): Double = speed.speed()

    /**
     * 当前倍速的**安全**版本：会话还没起时返回 null。
     *
     * 【为什么需要】UI 的面板在会话建立前就可能被打开，
     * 用 [currentSpeed]（会抛）会让界面崩掉。
     */
    fun currentSpeedOrNull(): Double? {
        val native = session ?: return null
        return runCatching { native.property("speed")?.toDoubleOrNull() }.getOrNull()
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

    /**
     * 取活着的内核会话。
     *
     * @throws IllegalStateException 会话还没起或已经释放。
     *
     * 【为什么用异常而不是返回 null】
     * 调用方（轨道目录、倍速控制器）拿到 null 后仍要继续用，
     * 那就得到一串 null 传播；而"会话不存在"是一个**编程错误**
     * （在 start 之前就用轨道目录），早失败比晚失败好查。
     */
    private fun requireSession(): MpvSession =
        session ?: throw IllegalStateException("内核会话尚未启动或已释放")
}
