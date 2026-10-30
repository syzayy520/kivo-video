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
import com.kivo.cinema.bridge.MediaTrackProbe
import com.kivo.cinema.bridge.NativePlayer
import com.kivo.cinema.bridge.PlaybackSession
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
     * 会话是否已经起跑（P12-D1）。
     *
     * 与 `session != null` 不是一回事：会话在 `openVideo` 之后就存在了，
     * 但只有 `start()` 之后才"起跑"。换面只对起跑中的会话有意义 ——
     * 没起跑的会话直接重建更省事，也不会产生假的恢复数字。
     */
    private var playbackRunning = false

    /**
     * 结束证据的落盘状态（P12-D4）。
     *
     * 两个标志分开，因为**失败要重试、成功不许重写**：
     * 之前的写法是"每轮询一次写一次"，同一个文件被重写十几遍 —— 数字没变所以没人发现，
     * 但"最后一次写"可能落在会话已经半停的时刻，那才是风险。
     * 改成"成功才封口"之后，写失败的那次会留在下一轮重试，证据不会因为一次瞬时失败丢掉。
     */
    private var statsWritten = false
    private var syncWritten = false

    /**
     * seek 取证参数（P12-F1）。`-1` 表示不 seek。
     *
     * 为什么用 `postDelayed` 而不是"起播即 seek"：起播瞬间的定位会和首帧解码、
     * 节拍锚点混在一起，量出来的不是"seek 的时延"而是"冷启动 + seek"。
     * 默认等 1.5 s，让第一段先稳态跑起来。
     */
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
            val rc = live.writeStats(File(evidenceDir(), name).absolutePath)
            Log.i(TAG, "soak sample #$soakIndex t=${elapsedSec}s rc=$rc -> $name")
            soakIndex += 1
            handler.postDelayed(this, soakIntervalMs)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        readProbeExtras()

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
            val pacingRc = live.configurePacing(targetFps, allowDrop)
            if (pacingRc != 0) {
                failStart("pacing", pacingRc, live, media)
                return
            }
            Log.i(TAG, "pacing on: targetFps=$targetFps allowDrop=$allowDrop")
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
        statsWritten = false
        syncWritten = false
        Log.i(TAG, "playback started: ${media.absolutePath}")
        handler.post(pollRunnable)

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

    private fun finishPlayback() {
        val live = session ?: return
        val needsSync = pacingEnabled && !syncWritten
        if (statsWritten && !needsSync) {
            return
        }
        handler.removeCallbacks(soakRunnable)

        if (!statsWritten) {
            val statsFile = File(evidenceDir(), evidenceName)
            val rc = live.writeStats(statsFile.absolutePath)
            Log.i(TAG, "stats written rc=$rc -> ${statsFile.absolutePath}")
            if (rc == 0) {
                statsWritten = true
                statusView.append("\nstats rc=$rc\n${statsFile.name}")
            }
        }

        // A/V 合并证据（P12-C8）。只在开了节拍时写：没有节拍器就没有偏差样本，
        // 写出来只是一份"全是 0"的文件，比不写更容易被误读成"零偏差"。
        // 必须在 stop 之后写 —— native 侧要读节拍器的完整直方图，
        // 而 `stop()` 是把它写成终值的唯一时机（`PlaybackSession::stop` 幂等）。
        if (needsSync) {
            live.stop()
            val syncFile = File(evidenceDir(), syncEvidenceName)
            val syncRc = live.writeSyncStats(syncFile.absolutePath)
            Log.i(TAG, "sync stats written rc=$syncRc -> ${syncFile.absolutePath}")
            if (syncRc == 0) {
                syncWritten = true
                statusView.append("\nsync rc=$syncRc audio=$audioAttached\n${syncFile.name}")
            }
        }

        // 取证模式下写完就退出：adb 侧不用猜"这一轮结束没有"。
        // 注意判据是**两份都写完**，否则 sync 写失败时会被 finish() 带走，证据就丢了。
        if (autoFinish && statsWritten && (!pacingEnabled || syncWritten)) {
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
        seekTargetMs = intent.getIntExtra("kivo.seek.ms", -1).toLong()
        seekAtMs = intent.getIntExtra("kivo.seek.at.ms", 1500).toLong().coerceAtLeast(0L)
        autoFinish = intent.hasExtra("kivo.evidence")
        if (overrideMediaPath != null || pacingEnabled) {
            Log.i(TAG, "probe extras: media=$overrideMediaPath pace=$pacingEnabled " +
                    "drop=$allowDrop fps=$targetFps split=$splitPipeline render=$presentFrames " +
                    "present=${if (atTimePresent) "at_time" else "cpu_paced"} " +
                    "audio=${if (audioEnabled) "auto" else "off"} " +
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
    private fun writeFailureEvidence(stage: String, rc: Int, reason: String, media: File) {
        // P12-F3：文件名带上用例名。没有用例名时退回旧名（保持 P12-B 起的既有约定）。
        val fileName = if (caseName.isBlank()) "p12b-failure.json" else "p12f3-$caseName-failure.json"
        val escaped = reason.replace("\\", "\\\\").replace("\"", "\\\"")
        val payload = buildString {
            appendLine("{")
            appendLine("  \"schema\": \"kivo.p12f.negative/1\",")
            // P12-F4 层 1：证据必须带实测时间戳，否则分不清真机产物与手写片段。
            appendLine("  \"generated_at_unix_ms\": ${System.currentTimeMillis()},")
            appendLine("  \"case\": \"${caseName.replace("\"", "\\\"")}\",")
            appendLine("  \"stage\": \"$stage\",")
            appendLine("  \"return_code\": $rc,")
            appendLine("  \"media\": \"${media.name}\",")
            appendLine("  \"reason\": \"$escaped\"")
            appendLine("}")
        }
        runCatching {
            File(evidenceDir(), fileName).writeText(payload)
        }
        Log.w(TAG, "failure evidence written: $fileName stage=$stage rc=$rc $reason")
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
        statsWritten = false
        syncWritten = false
    }

    override fun onDestroy() {
        teardownSession()
        super.onDestroy()
    }

    // ---- 测试片源定位 ------------------------------------------------------

    private fun evidenceDir(): File = File(filesDir, "evidence").apply { mkdirs() }

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
