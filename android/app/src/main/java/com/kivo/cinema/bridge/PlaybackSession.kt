package com.kivo.cinema.bridge

import android.os.ParcelFileDescriptor
import android.view.Surface
import java.io.File

/**
 * 一条播放会话的生命周期包装（P12-D2 建立，P12-D4 收敛为唯一入口）。
 *
 * 职责边界：**一个 handle + 最多两个 fd**，以及打开/配置/起播/换面/写证据/停止/释放。
 * 不碰 UI、不管布局、不做轮询、不认识 `Intent` —— 那些属于 Activity。
 *
 * ## 为什么它必须存在（而不是各 Activity 内联写）
 *
 * `PlayerActivity` 与 `MultiPlayerActivity` 各自内联过一遍同样的十步调用序列，
 * 而这段序列里有两条只能靠"记住"维持的规矩：
 *
 * 1. **fd 必须在 native 侧释放之后才关。** 反过来是 extractor 读到已关闭的描述符。
 * 2. **音频必须是另开的一个 fd。** 两个 `AMediaExtractor` 共用一个 fd 会让读位置互相打架，
 *    表现是其中一路随机解不出包。
 *
 * 两条都已出过事故。写在两个地方就是两份记忆，收在这里就是一份。
 *
 * ## 为什么音频与视频分开 `openVideo` / `openAudio`
 *
 * 因为它们的失败语义不同：视频开不起来是**故障**（中止这一轮），
 * 而没有音轨是**片源属性**（显式降级到 system 时钟）。把"要不要开音频"的判断
 * 留在调用方，这个区别才看得见；塞进来就会变成"内部猜"。
 *
 * ## 顺序约定（改之前先看 `31-p12d4-design.md`）
 *
 * `openVideo` → `openAudio` → `configurePacing` → `configurePipeline` → `start`。
 * 音频必须在 `configurePacing` 之前起流：native 侧只在音频已起流时才接音频时钟，
 * 反了会静默退回 `system` 时钟，而证据里这一轮本该是音频档。
 */
class PlaybackSession(
    /** 多路并发里的序号；单路场景恒为 0。只用于日志与证据文件名。 */
    val index: Int,
    /** 本轮播放的文件。失败取证要用真实路径，不能拿显示名凑。 */
    val mediaFile: File,
    /** 诊断标签（通常是文件名）。不参与任何 IO。 */
    val displayName: String
) {
    var handle: Long = 0L
        private set

    /** 非空表示这条会话已经失败，原因可直接进证据。 */
    var lastError: String = ""
        private set

    /** 本轮是否真的把音频接上了。false 时证据里的 `av_drift` 会带 unavailable_reason。 */
    var audioAttached: Boolean = false
        private set

    private var videoFd: ParcelFileDescriptor? = null
    private var audioFd: ParcelFileDescriptor? = null
    private var running = false
    private var stopped = false

    /**
     * 开文件 + 建会话 + 挂 Surface。**不起播、不配置。**
     *
     * 分成 open / configure / start 三段的原因：多路并发时，"全部打开"和"全部起播"
     * 必须是两轮 —— 一条一条地 open+start 会让第一路先播几十毫秒，
     * 测出来的不是并发而是接力。
     *
     * 失败返回非 0，[lastError] 里是原因；调用方负责写失败证据并 [close]。
     */
    /**
     * 定位到 [positionUs]（微秒）。**只能在 start 之后调用。**
     *
     * 失败时把原因记进 [lastError] 并返回非 0；调用方决定是中止还是继续播。
     * 这里不做重试：seek 失败通常是片源问题（目标超出范围），重试只会拖时间。
     */
    fun seekTo(positionUs: Long): Int {
        if (!running) {
            lastError = "seekTo called before start"
            return -1
        }
        val rc = NativePlayer.nativeSessionSeek(handle, positionUs)
        if (rc != 0) {
            lastError = NativePlayer.nativeSessionLastError(handle)
        }
        return rc
    }

    fun openVideo(surface: Surface): Int {
        if (handle != 0L) {
            lastError = "openVideo called twice"
            return -1
        }
        val fd = runCatching {
            ParcelFileDescriptor.open(mediaFile, ParcelFileDescriptor.MODE_READ_ONLY)
        }.getOrNull()
        if (fd == null) {
            lastError = "ParcelFileDescriptor.open failed"
            return -1
        }
        videoFd = fd

        // 先建句柄再开文件：`nativeSessionOpen` 失败时句柄还在，
        // 才能把它自己的错误原因读出来（否则只剩一个返回码）。
        handle = runCatching { NativePlayer.nativeSessionCreate() }.getOrElse { 0L }
        if (handle == 0L) {
            lastError = "nativeSessionCreate failed"
            return -1
        }

        val rc = runCatching {
            NativePlayer.nativeSessionOpen(
                handle,
                displayName,
                fd.fd,
                mediaFile.length(),
                surface
            )
        }.getOrElse { -1 }
        if (rc != 0) {
            lastError = nativeErrorOr("open failed rc=$rc")
            return rc
        }
        return 0
    }

    /**
     * 另开一个 fd 并起音频（AAudio + 音频主时钟）。
     *
     * 有音轨却开不起来是**故障**，调用方应当中止这一轮 —— 不能退回 system 时钟继续播，
     * 那会产出一份"看起来有 av_drift、其实是节拍偏移"的假证据。
     */
    fun openAudio(): Int {
        if (handle == 0L) {
            lastError = "openAudio before openVideo"
            return -1
        }
        if (audioAttached) {
            lastError = "openAudio called twice"
            return -1
        }
        val fd = runCatching {
            ParcelFileDescriptor.open(mediaFile, ParcelFileDescriptor.MODE_READ_ONLY)
        }.getOrNull()
        if (fd == null) {
            lastError = "audio ParcelFileDescriptor.open failed"
            return -1
        }
        audioFd = fd

        val rc = runCatching {
            NativePlayer.nativeSessionOpenAudio(handle, displayName, fd.fd, mediaFile.length())
        }.getOrElse { -1 }
        if (rc != 0) {
            lastError = nativeErrorOr("audio failed rc=$rc")
            return rc
        }
        audioAttached = true
        return 0
    }

    /**
     * P16-P1B31：暂停。保留 codec / extractor / surface，不清帧计数。
     *
     * native 侧复用 `stop()`（它只停线程），所以没有重建流的间隙。
     */
    fun pause(): Int = setPaused(true)

    /** P16-P1B31：从暂停处继续。会先把节拍器的 due 锚点推到 now。 */
    fun resume(): Int = setPaused(false)

    private fun setPaused(paused: Boolean): Int {
        if (handle == 0L) {
            lastError = "setPaused before openVideo"
            return -1
        }
        val rc = runCatching { NativePlayer.nativeSessionSetPaused(handle, paused) }.getOrElse { -1 }
        if (rc != 0) {
            lastError = nativeErrorOr("setPaused($paused) failed rc=$rc")
            return rc
        }
        return 0
    }

    /** 挂节拍。**必须在 [start] 之前**：起播之后再挂等于半路改规则。 */
    fun configurePacing(
        targetFps: Int,
        allowDrop: Boolean,
        clockOffsetMs: Int = 0,
        clockOffsetAtMs: Int = 2_000,
        clockRatePpm: Int = 0,
        driftGate: Boolean = true,
        stepRepay: Boolean = true,
        driftBandMs: Int = 5,
    ): Int {
        if (handle == 0L) {
            lastError = "configurePacing before openVideo"
            return -1
        }
        val rc = runCatching {
            NativePlayer.nativeSessionSetPacing(
                handle, targetFps, allowDrop, clockOffsetMs, clockOffsetAtMs,
                clockRatePpm, driftGate, stepRepay, driftBandMs
            )
        }.getOrElse { -1 }
        if (rc != 0) {
            lastError = nativeErrorOr("pacing failed rc=$rc")
            return rc
        }
        return 0
    }

    /** 定流水线结构与呈现排程。同样必须在 [start] 之前。 */
    fun configurePipeline(split: Boolean, present: Boolean, atTime: Boolean): Int {
        if (handle == 0L) {
            lastError = "configurePipeline before openVideo"
            return -1
        }
        val rc = runCatching {
            NativePlayer.nativeSessionSetPipeline(handle, split, present, atTime)
        }.getOrElse { -1 }
        if (rc != 0) {
            lastError = nativeErrorOr("pipeline failed rc=$rc")
            return rc
        }
        return 0
    }

    fun start(): Int {
        if (handle == 0L) {
            lastError = "start before openVideo"
            return -1
        }
        val rc = runCatching { NativePlayer.nativeSessionStart(handle) }.getOrElse { -1 }
        if (rc != 0) {
            lastError = nativeErrorOr("start failed rc=$rc")
            return rc
        }
        running = true
        stopped = false
        return 0
    }

    /**
     * 换输出 Surface（P12-D1）。`null` = 面没了，先摘掉。
     *
     * 换面**不动会话状态**（帧计数、节拍锚点、时钟全部连续），
     * 这是"切后台再回来继续播"与"从头重播"的分界。
     */
    fun rebind(surface: Surface?): Int {
        if (handle == 0L) {
            lastError = "rebind before openVideo"
            return -1
        }
        val rc = runCatching {
            NativePlayer.nativeSessionRebindSurface(handle, displayName, surface)
        }.getOrElse { -1 }
        if (rc != 0) {
            lastError = nativeErrorOr("rebind failed rc=$rc")
        }
        return rc
    }

    fun summary(): String =
        if (handle == 0L) "no-session" else runCatching {
            NativePlayer.nativeSessionSummary(handle)
        }.getOrElse { "summary-failed" }

    fun isFinished(): Boolean {
        val s = summary()
        return s.contains("completed=1") || !s.contains("error=-")
    }

    fun writeStats(path: String): Int =
        if (handle == 0L) -1 else runCatching {
            NativePlayer.nativeSessionWriteStats(handle, path)
        }.getOrElse { -1 }

    /**
     * 停播放但**不释放句柄**。可重复调用。
     *
     * 与 [close] 分开是因为 A/V 合并证据要在"已停、未释放"之间写：
     * native 侧读的是节拍器停下之后的完整直方图，句柄提前放掉就什么都没了。
     *
     * 这一句不能省 —— `session_stop` 才是把音频 `feed` / `output` / `clock`
     * 三组统计写成终值的地方。少了它，证据里的音频段全是 0、
     * `clock_source` 退回 `system`（P12-C8 第一轮就是这么写坏的）。
     */
    fun stop() {
        if (handle == 0L || stopped) {
            return
        }
        stopped = true
        runCatching { NativePlayer.nativeSessionStop(handle) }
        running = false
    }

    fun writeSyncStats(path: String): Int =
        if (handle == 0L) -1 else runCatching {
            NativePlayer.nativeSessionWriteSyncStats(handle, path)
        }.getOrElse { -1 }

    /** 停止并释放。可重复调用；fd 一定在 native 释放之后才关。 */
    fun close() {
        if (handle != 0L) {
            runCatching { NativePlayer.nativeSessionStop(handle) }
            runCatching { NativePlayer.nativeSessionDestroy(handle) }
            handle = 0L
        }
        stopped = true
        running = false
        // native 侧已经放掉 extractor 了，这时关 fd 才是安全的。
        runCatching { audioFd?.close() }
        audioFd = null
        runCatching { videoFd?.close() }
        videoFd = null
        audioAttached = false
    }

    fun isRunning(): Boolean = running

    private fun nativeErrorOr(fallback: String): String =
        runCatching { NativePlayer.nativeSessionLastError(handle) }.getOrNull()
            ?.takeIf { it.isNotBlank() } ?: fallback
}
