package com.kivo.cinema.ui

import android.graphics.Color
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.util.TypedValue
import android.view.Gravity
import android.view.SurfaceHolder
import android.view.WindowManager
import android.widget.FrameLayout
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.kivo.cinema.bridge.FfmpegPlaybackSession
import com.kivo.cinema.bridge.FfmpegRouting
import com.kivo.cinema.bridge.MediaTrackProbe
import com.kivo.cinema.bridge.NativePlayer
import com.kivo.cinema.bridge.PlaybackSession
import com.kivo.cinema.evidence.EvidencePaths
import com.kivo.cinema.evidence.FailureEvidence
import com.kivo.cinema.evidence.SessionFinalizer
import java.io.File

/**
 * 宿主 Activity。
 *
 * 职责：生命周期、Surface 挂载、把 native 统计投影到屏幕用于现场核对。
 * 明确不做：解码循环、渲染线程、A/V 同步 —— 这些全部在 C++ 核心。
 */
class PlayerActivity : AppCompatActivity(), PlayerSurfaceView.Listener {

    private lateinit var surfaceView: PlayerSurfaceView
    private lateinit var statusView: TextView

    /**
     * 本轮会话（P12-D4 起由 [PlaybackSession] 独占 handle 与两个 fd）。
     *
     * 这里刻意不再单独存 `sessionHandle` / `mediaDescriptor` / `audioDescriptor`：
     * 一旦 Activity 手里还留着一份句柄副本，就又有"两处各自释放"的可能 ——
     * 而那正是这次收敛要消掉的东西。
     */
    private var session: PlaybackSession? = null
    private var surfaceReady = false

    /**
     * 软解兜底会话（211）。与硬解 [session] 互斥——路由判定决定走哪条，
     * 同一时刻只有一条活跃。路由在 `maybeStartPlayback` 里、起播之前做（D1）。
     */
    private var ffmpegSession: FfmpegPlaybackSession? = null

    /**
     * 会话是否已经起跑（P12-D1）。
     *
     * 与 `session != null` 不是一回事：会话在 `openVideo` 之后就存在了，
     * 但只有 `start()` 之后才"起跑"。换面只对起跑中的会话有意义 ——
     * 没起跑的会话直接重建更省事，也不会产生假的恢复数字。
     */
    private var playbackRunning = false

    /**
     * 证据落盘（P14-R）。**收尾顺序与封口标志都由它管**，Activity 只说"现在收尾"。
     *
     * 搬出去的理由不是行数：本卡给收尾加了第二个入口（被打断），
     * 而两个入口的顺序规则必须一致 —— 一个已经扛着八件事的 Activity
     * 里再加一个入口，就是第 N 次"顺手加一点"。
     *
     * `lateinit` 而不是可空：`installEvidenceSinks()` 在 `onCreate` 里**无条件**执行，
     * 而所有使用者（`onStop` / `finishPlayback` / 失败证据）都在它之后。
     * 写成可空只会给每个调用点添一次 `?.`，那层保护永远不会触发。
     */
    private lateinit var finalizer: SessionFinalizer
    private lateinit var evidencePaths: EvidencePaths

    /**
     * 把"证据写到哪、怎么收尾"两件事装起来（P14-R）。
     *
     * 必须在 [readProbeExtras] **之后**调用：文件名与"要不要写合并证据"都来自 Intent，
     * 在此之前构造会用一份错误的缺省值把这一轮写进别的文件里。
     */
    private fun installEvidenceSinks() {
        evidencePaths = EvidencePaths(filesDir)
        finalizer = SessionFinalizer(
            paths = evidencePaths,
            statsName = evidenceName,
            syncName = syncEvidenceName,
            // 没开节拍就没有偏差样本：写一份全 0 的合并证据比不写更容易被误读成"零偏差"。
            writesSync = pacingEnabled,
        )
    }

    /**
     * 中断收尾（P14-R，探针档）。
     *
     * `true` 时 Activity 进入后台（`onStop`）会把这一轮**当成一次收尾**：
     * 停会话、写两份证据。默认 `false` —— 产品行为仍然是"后台继续播"
     * （P12-D1 的 `rebuilds=1` / `frames_discarded ≈ 空窗×30` 就是按那条行为量的）。
     *
     * 为什么不能默认开：把"写一份闭合的账"和"后台继续播"放在一起是矛盾的 ——
     * 闭合要求先 `stop()`，而 `stop()` 就结束了会话。要不要后台停播是**产品决策**，
     * 取证档不替它做决定。
     */
    private var finalizeOnStop = false

    /**
     * 这条会话已经收尾、不再复活。
     *
     * 与 [playbackRunning] 不是一回事：后者说"在不在跑"，这个说"还准不准再起来"。
     * 中断收尾之后面会重新可用（用户切回来），若没有这一格，`maybeStartPlayback()`
     * 会从头再开一条会话 —— 而那条新会话播完时会**覆盖**刚写下的中断证据。
     */
    private var sessionEnded = false

    /**
     * seek 取证参数（P12-F1）。`-1` 表示不 seek。
     *
     * 为什么用 `postDelayed` 而不是"起播即 seek"：起播瞬间的定位会和首帧解码、
     * 节拍锚点混在一起，量出来的不是"seek 的时延"而是"冷启动 + seek"。
     * 默认等 1.5 s，让第一段先稳态跑起来。
     */
    /** P16-P1B31：定时 pause/resume，只用于真机验证（不接 UI）。 */
    private var pauseAtMs = -1L
    private var pauseForMs = 0L

    private var seekTargetMs = -1L
    private var seekAtMs = 1500L

    /**
     * 负向用例名（P12-F3）。来自 `--es kivo.case`，例如 `bad-file` / `no-video-track`。
     *
     * 为什么必须有它：失败证据原来只有一个固定文件名，四条负向路径会互相覆盖 ——
     * 跑完只剩最后一条，而**目录里看不出少了三条**。那是最坏的一种缺失。
     */
    private var caseName = ""

    /**
     * 探针的时钟台阶注入（P13-AB）。
     *
     * 为什么需要：P13-S 到 P13-AA 跑了 20 多轮负载实验去撞"时钟跑偏"，
     * 命中率约 1/3，于是"门到底管不管用"一直悬着 —— 撞不上就分不清
     * "门没用"和"这次没撞上"。这里把那个事件改成按需触发：媒体时间走到
     * [clockOffsetAtMs] 之后，时钟位置一次加上 [clockOffsetMs]。
     *
     * 0 = 不注入。不带这个参数时生产路径与之前逐字节相同，
     * 证据里的 `clock_injection.offset_ns` 会写 0。
     */
    private var clockOffsetMs = 0
    private var clockOffsetAtMs = 2_000

    /**
     * 速率注入（P13-AD）：正 = 时钟比墙快多少 ppm。
     *
     * 为什么还要这一格：P13-AB/AC 注入的都是**瞬时台阶**，可 P13-R/U/V/Z 的
     * 8 次真实事件**全是渐变**（8 秒里从 0 长到 73–165 ms）。台阶考验"当帧还多少"，
     * 速率考验"能不能一直追住"—— 这是两种不同的输入，不能互相代替。
     */
    private var clockRatePpm = 0

    /** P16-P1B17：迟滞带（ms）。默认 5 = 现有行为；只用于调参对照实验。 */
    private var driftBandMs = 5

    /** 漂移门开关（P13-AD）。false = 对照组；证据里 `drift_gate.enabled` 会写 false。 */
    private var driftGate = true

    /** 台阶还不还（P14-Z 探针）。false 只用于对照轮，证据里会写 `drift_gate.step_repay`。 */
    private var stepRepay = true

    /**
     * 长稳采样前缀（P12-D3）。非空时每 [soakIntervalMs] 写一份**中途快照**。
     *
     * 为什么中途快照是必须的：`frames_dropped` 的判据是"**无累积趋势**"，
     * 趋势需要时间序列。只留一份结束证据，只能看到总数，看不出它是
     * "一直 0" 还是 "前面 0、第 20 分钟开始涨"—— 而这两种含义完全不同。
     *
     * 复用既有渲染器写完整 `kivo.p12c.decode/6`，不新造 schema：
     * 中途快照的字段和结束证据一模一样，分析脚本不需要两套解析。
     */
    private var soakPrefix: String? = null
    private var soakIntervalMs = 60_000L
    private var soakIndex = 0
    private var soakStartedAtMs = 0L
    private var playbackRequested = false

    // ---- 探针参数（adb am start 传入，用于 C5 的三档帧率取证与 C6 的 split/merged 对照）----
    // 缺省行为与 P12-B 完全一致：播外部目录里第一个文件、不开节拍、写 p12b-decode.json。
    private var overrideMediaPath: String? = null
    private var pacingEnabled = false
    private var allowDrop = true
    private var targetFps = 0

    /**
     * 流水线模式（P12-C6）。
     *
     * false = 两段同线程交替（默认，也是产品行为）；
     * true = 送包与收帧各一个线程。**只在 A/B 取证与复测时用**：
     * 判据是"同一二进制、同一片源、交替跑"，编译期开关做不到交替。
     *
     * 默认值是 P12-C6 判据的回退结果：拆线程吞吐增益在噪声内（4K +0.5% <
     * 1080p10 +2.0%），见 docs/design/backend-core/20-p12c6-status.md。
     */
    private var splitPipeline = false

    /**
     * 是否把帧交给 Surface（诊断开关，默认 true）。
     *
     * false = "只解码不上屏"。它存在的唯一用途是回答"1080p 停在 ~175fps
     * 到底是解码慢还是呈现链路限速"，产品路径永远用 true。
     */
    private var presentFrames = true

    /**
     * 呈现排程由谁负责（P12-C7，默认 true = 交给合成器定时）。
     *
     * true  = `kSurfaceAtTime`：不睡，把 `due_at` 交给 `releaseOutputBufferAtTime`，
     *         由 SurfaceFlinger 在合成时按时间显示（**默认**）。
     * false = `kCpuPaced`：节拍器在解码线程里睡到 `due_at`，醒了立刻交出（C5 结构，回退路径）。
     *
     * 默认 true 的依据见 `docs/design/backend-core/23-p12c7-status.md` §五：
     * 加负载后 `kCpuPaced` 的交出时刻 max 从 0.18 ms 涨到 2.12 ms，
     * 而本路径的提交提前量在 12 次运行里 spread 全为 0。
     *
     * 两者必须留在同一个二进制里：A/B 取证要求交替跑，编译期开关做不到。
     */
    private var atTimePresent = true

    private var evidenceName = "p12b-decode.json"
    private var autoFinish = false

    /**
     * 是否启用音频主时钟（P12-C8）。
     *
     * true  = 若片源真有音轨，就开音轨、起 AAudio，节拍器改由音频时钟驱动，
     *         证据写 `kivo.p12c.sync/1`（`clock_source` 是音频档）。
     * false = 纯视频，行为与 C7 完全一致（`clock_source = system`）。
     *
     * **"片源有没有音轨"由 Kotlin 侧先探**（[MediaTrackProbe]），不由 native 猜：
     * 有音轨却开不起来是**故障**，必须中止；没有音轨是**片源属性**，显式降级。
     * 这两件事在 C++ 里长得一样（都是 `open_fd` 失败），混在一起就分不清了。
     */
    private var audioEnabled = true

    /** 本轮是否真的把音频接上了。false 时 `av_drift` 在证据里会带 unavailable_reason。 */
    private val audioAttached: Boolean
        get() = session?.audioAttached == true

    /** 节拍开着才写 A/V 合并证据：没节拍器就没有偏差样本，写了只是一堆 0。 */
    private var syncEvidenceName = "sync-p12c8.json"

    private val handler = Handler(Looper.getMainLooper())
    private val pollRunnable = object : Runnable {
        override fun run() {
            refreshStatus()
            if (session != null) {
                handler.postDelayed(this, POLL_INTERVAL_MS)
            }
        }
    }

    /**
     * 长稳中途采样（P12-D3）。文件名带"起播后第几秒"，方便和主机侧的内存采样对齐。
     */
    private val soakRunnable = object : Runnable {
        override fun run() {
            val prefix = soakPrefix
            val live = session
            if (prefix == null || live == null) {
                return
            }
            val elapsedSec = ((System.currentTimeMillis() - soakStartedAtMs) / 1000).toInt()
            val name = prefix.replace(Regex("\\.json$"), "") +
                "-t%04d.json".format(elapsedSec)
            val rc = live.writeStats(evidencePaths.statsFile(name).absolutePath)
            Log.i(TAG, "soak sample #$soakIndex t=${elapsedSec}s rc=$rc -> $name")
            soakIndex += 1
            handler.postDelayed(this, soakIntervalMs)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        readProbeExtras()
        installEvidenceSinks()

        surfaceView = PlayerSurfaceView(this).apply { listener = this@PlayerActivity }

        statusView = TextView(this).apply {
            setTextColor(Color.WHITE)
            setBackgroundColor(0x99000000.toInt())
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 10f)
            setPadding(24, 24, 24, 24)
        }

        val root = FrameLayout(this)
        root.addView(
            surfaceView,
            FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
            )
        )
        root.addView(
            statusView,
            FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT,
                FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.START or Gravity.TOP
            )
        )
        setContentView(root)

        val version = runCatching { NativePlayer.nativeVersion() }
            .getOrElse { "load-failed: ${it.message}" }
        Log.i(TAG, "native version: $version")
        statusView.text = "native: $version\nwaiting for surface…"
    }

    override fun onSurfaceAvailable(holder: SurfaceHolder) {
        val frame = holder.surfaceFrame
        Log.i(TAG, "surface available ${frame.width()}x${frame.height()}")
        surfaceReady = true

        // P12-D1：会话还在跑 → 把新面挂回去，**不重开会话**。
        // 这就是"切后台再回来继续播"和"从头重播"的分界。
        val live = session
        if (playbackRunning && live != null && live.handle != 0L) {
            val rc = live.rebind(holder.surface)
            if (rc == 0) {
                Log.i(TAG, "surface rebound")
                statusView.append("\nsurface rebound")
                handler.post(pollRunnable)
                return
            }
            // 换面失败：退回"重开会话"。这时候不能假装还在恢复 ——
            // 旧会话已经指向一个不存在的面，继续跑只会一直丢帧。
            val reason = live.lastError.ifBlank { "unavailable" }
            Log.w(TAG, "surface rebind failed rc=$rc: $reason")
            writeFailureEvidence("rebind", rc, reason, live.mediaFile)
            teardownSession()
        }
        maybeStartPlayback()
    }

    override fun onSurfaceLost() {
        Log.i(TAG, "surface lost")
        surfaceReady = false

        // P12-D1：**不销毁会话**，只把面摘掉。
        // 旧行为是 teardownSession()，于是切后台回来会从第一帧重播 —— 那不是恢复。
        // 摘面必须立刻做：旧 ANativeWindow 已经不可用，继续往上交帧轻则丢帧、
        // 重则在厂商驱动里崩。
        session?.rebind(null)
    }

    /**
     * P12-F1 取证入口：到点做一次 seek。
     *
     * 失败不改控制流 —— 只把原因写到屏幕上，证据里 `seek.pass` 会是 false 且带 reason。
     * "seek 失败导致整轮作废"的判据在 `assess_seek()` 里，不在这里重复判断。
     */
    /**
     * P16-P1B31：定时 pause，等 [pauseForMs] 后 resume。只用于真机验证。
     *
     * 不用 UI 按钮：`--es kivo.pause.at.ms` / `--es kivo.pause.ms` 就能复算，
     * 且不会把"暂停"这件事绑到某一次手动点击上。
     */
    private val pauseRunnable = Runnable {
        val live = session ?: return@Runnable
        if (!playbackRunning) {
            Log.w(TAG, "pause skipped: playback is not running")
            return@Runnable
        }
        val rc = live.pause()
        Log.i(TAG, "pause rc=$rc")
        handler.postDelayed({
            val resumeRc = session?.resume() ?: -1
            Log.i(TAG, "resume rc=$resumeRc")
        }, pauseForMs)
    }

    private val seekRunnable = Runnable {
        val live = session ?: return@Runnable
        if (!playbackRunning) {
            Log.w(TAG, "seek skipped: playback is not running")
            return@Runnable
        }
        val targetUs = seekTargetMs * 1000L
        val rc = live.seekTo(targetUs)
        Log.i(TAG, "seek -> ${seekTargetMs}ms rc=$rc")
        if (rc != 0) {
            statusView.text = "seek failed rc=$rc\n${live.lastError}"
        }
    }

    private fun maybeStartPlayback() {
        // P14-R：中断收尾过的会话不再复活。否则切回前台会从头再开一条，
        // 而那条新会话播完时会把刚写下的中断证据覆盖掉。
        if (sessionEnded) {
            Log.i(TAG, "playback not restarted: session already finalized on interrupt")
            return
        }
        if (!surfaceReady || playbackRequested) {
            return
        }
        val media = overrideMediaPath?.let { File(it) }?.takeIf { it.isFile && it.length() > 0 }
            ?: locateTestMedia()
        if (media == null) {
            statusView.text = "no test clip found\npush a file to:\n${externalMediaDir().absolutePath}"
            return
        }
        playbackRequested = true

        // ---- 211 路由（D1）：起播之前判定，两条判据按序（容器可解 → 解码可解），
        // 理由原样出版进证据（AC5）。历史片源两条全过 → 走原路径，行为零回归。
        val routing = FfmpegRouting.probe(media)
        if (routing.useFfmpeg) {
            Log.i(TAG, "routing: ffmpeg (${routing.reason})")
            startFfmpegPlayback(media, routing)
            return
        }
        Log.i(TAG, "routing: mediacodec (${routing.reason})")

        // 从这一刻起，句柄与两个 fd 全归 `PlaybackSession` 管。
        // Activity 只决定"要不要开音频""挂不挂节拍""用哪种排程"，不碰释放。
        val live = PlaybackSession(0, media, media.name)
        session = live

        val openRc = live.openVideo(surfaceView.holder.surface)
        if (openRc != 0) {
            failStart("open", openRc, live, media)
            return
        }

        // ---- 音频（P12-C8）----
        // 顺序不能挪：音频必须在 configurePacing 之前起流。
        // native 侧 `session_set_pacing` 只在音频已起流时才接音频时钟，
        // 反过来排会静默退回 system 时钟 —— 证据里就会写 system，而这一轮本该是音频档。
        if (audioEnabled && MediaTrackProbe.hasAudioTrack(media)) {
            val audioRc = live.openAudio()
            if (audioRc != 0) {
                // 有音轨却开不起来 = 故障，中止。不退回 system 时钟继续播 ——
                // 那会产出一份"看起来有 av_drift、其实是节拍偏移"的假证据。
                failStart("audio", audioRc, live, media)
                return
            }
            Log.i(TAG, "audio attached: ${media.name}")
        } else if (audioEnabled) {
            // 片源本来就没有音轨：显式降级，不是失败。证据里 has_audio=false，
            // av_drift 会带 unavailable_reason，而不是写一堆 0 冒充"零偏差"。
            Log.i(TAG, "source has no audio track, falling back to system clock")
        }

        // 节拍必须在 start 之前挂上：start 之后就起解码线程了，再挂等于半路改规则。
        if (pacingEnabled) {
            val pacingRc = live.configurePacing(
                targetFps, allowDrop, clockOffsetMs, clockOffsetAtMs, clockRatePpm, driftGate,
                stepRepay, driftBandMs
            )
            if (pacingRc != 0) {
                failStart("pacing", pacingRc, live, media)
                return
            }
            Log.i(TAG, "pacing on: targetFps=$targetFps allowDrop=$allowDrop " +
                    "clockOffset=${clockOffsetMs}ms@${clockOffsetAtMs}ms " +
                    "rate=${clockRatePpm}ppm gate=$driftGate")
        }

        // 流水线模式同样必须在 start 之前定：start 之后就起线程了，再改等于半路换引擎。
        val pipelineRc = live.configurePipeline(splitPipeline, presentFrames, atTimePresent)
        if (pipelineRc != 0) {
            failStart("pipeline", pipelineRc, live, media)
            return
        }

        val startRc = live.start()
        if (startRc != 0) {
            failStart("start", startRc, live, media)
            return
        }

        playbackRunning = true
        finalizer.reset()
        Log.i(TAG, "playback started: ${media.absolutePath}")
        handler.post(pollRunnable)

        if (pauseAtMs >= 0 && pauseForMs > 0) {
            handler.postDelayed(pauseRunnable, pauseAtMs)
            Log.i(TAG, "pause scheduled: at=${pauseAtMs}ms for=${pauseForMs}ms")
        }

        if (seekTargetMs >= 0) {
            handler.postDelayed(seekRunnable, seekAtMs)
            Log.i(TAG, "seek scheduled: target=${seekTargetMs}ms at=${seekAtMs}ms")
        }

        if (soakPrefix != null) {
            soakStartedAtMs = System.currentTimeMillis()
            // 第一次采样放在一个间隔之后：t=0 的快照没有信息量，
            // 而且会和起播瞬间的冷启动内存混在一起。
            handler.postDelayed(soakRunnable, soakIntervalMs)
        }
    }

    /**
     * 起播链路上任何一步失败都走这里：落证据 → 拆会话。
     *
     * 抽出来的原因不是省几行，而是四个失败分支原本各自手写了一遍
     * "读 last_error、拼状态文字、写证据、teardown"，四处里任何一处漏一步
     * 都会让那一类失败变成静默失败（G9 禁止）。现在只有一处能漏。
     */
    private fun failStart(stage: String, rc: Int, live: PlaybackSession, media: File) {
        val reason = live.lastError.ifBlank { "unavailable" }
        statusView.text = "$stage failed rc=$rc\n${media.name}\n$reason"
        writeFailureEvidence(stage, rc, reason, media)
        teardownSession()
    }

    // ---- 211：软解兜底路径。与硬解路径刻意分开——生命周期、证据、轮询都是另一套，
    // 混进 startPlaybackIfNeeded 只会让两条互相缠住。 ----

    private val ffmpegPollRunnable = object : Runnable {
        override fun run() {
            val live = ffmpegSession ?: return
            if (live.isRunning()) {
                handler.postDelayed(this, POLL_INTERVAL_MS)
                return
            }
            // 自然 EOS / 中断：收证据 → 关会话 → 结束本轮。
            val evidence = live.stopAndCollectEvidence()
            val name = evidenceName.ifBlank { "p211-ffmpeg.json" }
            runCatching {
                java.io.File(evidencePaths.dir, name).writeText(evidence)
            }.onSuccess {
                Log.i(TAG, "ffmpeg evidence written: $name")
            }.onFailure { e ->
                Log.w(TAG, "ffmpeg evidence write failed: ${e.message}")
            }
            ffmpegSession?.close()
            ffmpegSession = null
            finish()
        }
    }

    private fun startFfmpegPlayback(media: File, routing: FfmpegRouting.Decision) {
        val surface = surfaceView.holder.surface
        if (surface == null || !surface.isValid) {
            statusView.text = "ffmpeg route: surface not ready"
            writeFailureEvidence("ffmpeg-surface", -1, "surface not ready", media)
            finish()
            return
        }
        val live = FfmpegPlaybackSession(media, media.name, routing.decision, routing.reason)
        ffmpegSession = live

        val openRc = live.openVideo(surface)
        if (openRc != 0) {
            statusView.text = "ffmpeg open failed rc=$openRc\n${live.nativeError()}"
            writeFailureEvidence("ffmpeg-open", openRc, live.nativeError(), media)
            ffmpegSession?.close()
            ffmpegSession = null
            finish()
            return
        }
        val startRc = live.start()
        if (startRc != 0) {
            statusView.text = "ffmpeg start failed rc=$startRc\n${live.nativeError()}"
            writeFailureEvidence("ffmpeg-start", startRc, live.nativeError(), media)
            ffmpegSession?.close()
            ffmpegSession = null
            finish()
            return
        }
        Log.i(TAG, "ffmpeg playback started: ${media.absolutePath}")
        statusView.text = "ffmpeg 软解播放中\n${media.name}\n${routing.reason}"
        handler.post(ffmpegPollRunnable)
    }

    private fun refreshStatus() {
        val live = session ?: return
        val summary = live.summary()

        statusView.text = summary
            .replace(" mime=", "\nmime=")
            .replace(" frames=", "\nframes=")
            .replace(" wall=", "\nwall=")
            .replace(" first_frame=", "\nfirst_frame=")
            .replace(" completed=", "\ncompleted=")

        if (summary.contains("completed=1") || !summary.contains("error=-")) {
            finishPlayback()
        }
    }

    /**
     * 自然收尾（跑到 EOS）或出错之后的收尾。
     *
     * P14-R 之前这里自己管顺序与封口标志；现在只说"收尾"，其余交给 [SessionFinalizer]。
     * **顺序变了**：从「写解码证据 → stop → 写合并证据」统一成「stop → 写两份」。
     * 理由见那个类的注释 —— 一句话：写之前先让所有线程停下，这个规则对"为什么写"不作假设。
     */
    private fun finishPlayback() {
        val live = session ?: return
        if (finalizer.sealed) {
            return
        }
        handler.removeCallbacks(soakRunnable)

        val outcome = finalizer.finalize(live)

        if (outcome.statsRc == 0) {
            statusView.append("\nstats rc=${outcome.statsRc}\n${outcome.statsFile.name}")
        }
        if (outcome.syncRc == 0 && outcome.syncFile != null) {
            statusView.append("\nsync rc=${outcome.syncRc} audio=$audioAttached\n${outcome.syncFile.name}")
        }

        // 取证模式下写完就退出：adb 侧不用猜"这一轮结束没有"。
        // 注意判据是**两份都写完**，否则 sync 写失败时会被 finish() 带走，证据就丢了。
        if (autoFinish && finalizer.sealed) {
            handler.postDelayed({ finish() }, 500L)
        }
    }

    /**
     * 读 `adb am start` 传来的探针参数。
     *
     * 一个都不传时行为与 P12-B 完全一致（播外部目录第一个文件、不开节拍）。
     * 这条兼容性是有意的：C5 的对照数据必须能和 P12-B 的基线直接比。
     */
    private fun readProbeExtras() {
        val intent = intent ?: return
        overrideMediaPath = intent.getStringExtra("kivo.media")
        pacingEnabled = intent.getIntExtra("kivo.pace", 0) != 0
        allowDrop = intent.getIntExtra("kivo.drop", 1) != 0
        targetFps = intent.getIntExtra("kivo.fps", 0)
        splitPipeline = intent.getIntExtra("kivo.threads", 0) != 0
        presentFrames = intent.getIntExtra("kivo.render", 1) != 0
        // 默认 surface_at_time（不睡，交给合成器定时）。传 `cpu_paced` 退回 C5 的睡眠结构。
        // 用字符串而不是 int：将来若出现第三种排程方式，多一个词比多一个整数好扩。
        atTimePresent = intent.getStringExtra("kivo.present") != "cpu_paced"
        // 传 `--es kivo.audio off` 强制纯视频（A/B 对照用）。不带 = 有音轨就接音频时钟。
        audioEnabled = intent.getStringExtra("kivo.audio") != "off"
        evidenceName = intent.getStringExtra("kivo.evidence") ?: "p12b-decode.json"
        // P12-D3：非空则每 kivo.soak.ms 毫秒写一份中途快照（默认 60 s）。
        soakPrefix = intent.getStringExtra("kivo.soak")
        soakIntervalMs = intent.getIntExtra("kivo.soak.ms", 60_000).toLong().coerceAtLeast(1_000L)
        syncEvidenceName = intent.getStringExtra("kivo.sync")
            ?: evidenceName.replace(Regex("\\.json$"), "") + "-sync.json"
        // P12-F1：非负则在该时刻做一次 seek（单位 ms），默认起播后 1.5 s 触发。
        caseName = intent.getStringExtra("kivo.case") ?: ""
        // P13-AB：探针的时钟台阶注入。非 0 时在媒体时间走到 clockOffsetAtMs 之后
        // 把时钟位置一次加上这么多毫秒 —— 用来把"时钟跑偏"从靠负载撞概率
        // （历史命中率约 1/3）变成可复现。0 = 不注入，生产路径不受影响。
        clockOffsetMs = intent.getIntExtra("kivo.clock_offset_ms", 0)
        clockOffsetAtMs = intent.getIntExtra("kivo.clock_offset_at_ms", 2_000)
        clockRatePpm = intent.getIntExtra("kivo.clock_rate_ppm", 0)
        driftBandMs = intent.getIntExtra("kivo.drift_band_ms", 5)
        // 传 `--es kivo.drift_gate off` 关掉漂移门（对照组）。
        driftGate = intent.getStringExtra("kivo.drift_gate") != "off"
        // 传 `--es kivo.step_repay off` 让台阶不进门（P14-Z 的 B 臂）。
        // 默认 true = P13-W 的行为；证据里 `drift_gate.step_repay` 会写出来。
        stepRepay = intent.getStringExtra("kivo.step_repay") != "off"
        pauseAtMs = intent.getIntExtra("kivo.pause.at.ms", -1).toLong()
        pauseForMs = intent.getIntExtra("kivo.pause.ms", 0).toLong()
        seekTargetMs = intent.getIntExtra("kivo.seek.ms", -1).toLong()
        seekAtMs = intent.getIntExtra("kivo.seek.at.ms", 1500).toLong().coerceAtLeast(0L)
        // P14-R：`--es kivo.finalize on_stop` 把"进入后台"当成一次收尾。
        // 默认不开 —— 产品行为仍然是"后台继续播"，见 `onStop()` 的注释。
        finalizeOnStop = intent.getStringExtra("kivo.finalize") == "on_stop"
        autoFinish = intent.hasExtra("kivo.evidence")
        if (overrideMediaPath != null || pacingEnabled) {
            Log.i(TAG, "probe extras: media=$overrideMediaPath pace=$pacingEnabled " +
                    "drop=$allowDrop fps=$targetFps split=$splitPipeline render=$presentFrames " +
                    "present=${if (atTimePresent) "at_time" else "cpu_paced"} " +
                    "audio=${if (audioEnabled) "auto" else "off"} " +
                    "finalize=${if (finalizeOnStop) "on_stop" else "never"} " +
                    "evidence=$evidenceName sync=$syncEvidenceName")
        }
    }

    /**
     * 失败也要留证据。
     *
     * 本机 HyperOS 对 adb 完全不开放 logcat（`adb logcat -d` 返回 0 行），
     * 屏幕上的一行字又会被下一次刷新覆盖。所以失败必须落成文件，
     * 否则等于「什么都没发生」——这正是 G9 禁止的静默失败。
     */
    /**
     * 失败也要留证据。
     *
     * 本机 HyperOS 对 adb 完全不开放 logcat（`adb logcat -d` 返回 0 行），
     * 屏幕上的一行字又会被下一次刷新覆盖。所以失败必须落成文件，
     * 否则等于「什么都没发生」——这正是 G9 禁止的静默失败。
     *
     * P14-R：写入细节搬到 `FailureEvidence`，与收尾落盘住同一格 ——
     * 两处各自拼文件名的日子结束了（`p12f3` 那四条负向用例就是被这个坑覆盖过）。
     */
    private fun writeFailureEvidence(stage: String, rc: Int, reason: String, media: File) {
        FailureEvidence.write(
            paths = evidencePaths,
            caseName = caseName,
            stage = stage,
            returnCode = rc,
            reason = reason,
            mediaName = media.name,
        )
    }

    private fun teardownSession() {
        handler.removeCallbacks(pollRunnable)
        handler.removeCallbacks(soakRunnable)
        // `close()` 里是"先 stop、再 destroy、最后关 fd" —— 顺序由它一个人负责。
        // 这里**不许**再单独关 fd：那样又会变成两处各自释放，而 fd 提前关掉
        // 会让 native 的 extractor 读到已关闭的描述符。
        session?.close()
        session = null
        playbackRunning = false
        playbackRequested = false
        if (::finalizer.isInitialized) {
            finalizer.reset()
        }
    }

    override fun onDestroy() {
        // 软解路径的收尾：拆轮询、停会话、关句柄（证据在正常轮询里已收走）。
        handler.removeCallbacks(ffmpegPollRunnable)
        ffmpegSession?.close()
        ffmpegSession = null
        teardownSession()
        super.onDestroy()
    }

    /**
     * 进入后台（P14-R）。
     *
     * ## 为什么需要它
     *
     * 收尾本来是 400 ms 轮询里的一拍，而轮询只在 `completed=1` 时才写。
     * 被打断的会话 `completed` 永远是 0 —— 于是**什么都不写**，
     * 而进程被回收之后连"没写"这件事都看不出来（上一张卡的取证轮就是这么空的）。
     *
     * ## 为什么默认不这么做
     *
     * 写一份**闭合**的账要求先 `stop()`，而 `stop()` 就结束了会话；
     * 产品行为是"后台继续播"（P12-D1 量的 `rebuilds=1` / `frames_discarded ≈ 空窗×30`）。
     * 两者互斥。要不要"后台停播"是产品决策，取证档只提供 `--es kivo.finalize on_stop`
     * 把这个形态**造出来**，不替产品做决定。
     */
    override fun onStop() {
        if (finalizeOnStop) {
            finalizeOnInterrupt()
        }
        super.onStop()
    }

    /**
     * 把"进后台"当成一次收尾：停会话 → 写两份证据 → 关掉这条会话，不再复活。
     *
     * 与 [finishPlayback] 走**同一个** [SessionFinalizer]，所以顺序规则只有一份。
     * 差别只在收尾之后：这条路径**不** `finish()`，也不允许 `maybeStartPlayback()`
     * 重开（那会把刚写下的中断证据覆盖掉）。
     */
    private fun finalizeOnInterrupt() {
        val live = session ?: return
        if (!playbackRunning || finalizer.sealed) {
            return
        }
        Log.i(TAG, "onStop: finalizing interrupted session")
        handler.removeCallbacks(pollRunnable)
        handler.removeCallbacks(soakRunnable)

        val outcome = finalizer.finalize(live)

        statusView.append(
            "\ninterrupted: stats rc=${outcome.statsRc} sync rc=${outcome.syncRc}" +
                "\n${outcome.statsFile.name}"
        )

        // 收尾之后这条会话已经没有下一天：关掉句柄与 fd，但保留 `playbackRequested`，
        // 让 `maybeStartPlayback()` 连"要不要重开"都不必判断（`sessionEnded` 是第二道闩）。
        sessionEnded = true
        playbackRunning = false
        live.close()
        session = null
    }

    // ---- 测试片源定位 ------------------------------------------------------

    // 证据目录已归 `EvidencePaths`（P14-R）：`filesDir` 只在这里出现一次。

    private fun externalMediaDir(): File =
        File(getExternalFilesDir(null) ?: filesDir, "media").apply { mkdirs() }

    /**
     * 优先取外部私有目录里的片源（adb push 方便）；
     * 没有则回落到内部私有目录。两者都需要用户无感权限，省掉存储授权流程。
     */
    private fun locateTestMedia(): File? {
        val candidates = listOf(externalMediaDir(), File(filesDir, "media"))
        for (dir in candidates) {
            val files = dir.listFiles { file -> file.isFile && file.length() > 0 }
            if (!files.isNullOrEmpty()) {
                return files.sortedBy { it.name }.first()
            }
        }
        return null
    }

    companion object {
        private const val TAG = "KivoCinema"
        private const val POLL_INTERVAL_MS = 400L
    }
}
