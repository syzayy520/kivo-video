package com.kivo.cinema.bridge

import android.os.ParcelFileDescriptor
import android.view.Surface
import java.io.File

/**
 * 软解兜底会话（211）的 Kotlin 门面。
 *
 * 与 [PlaybackSession] 平行、刻意不共用：句柄、生命周期、证据 schema 都是另一套
 * （`kivo.p211.decode/1`，`decoder.backend = "ffmpeg"`）。fd 约定相同——
 * 本类持有 [ParcelFileDescriptor]，native 借用；close 顺序由 [close] 一处负责。
 *
 * 路由判定不在这里做：[FfmpegRouting.probe] 在 Activity 起播前调，
 * 判定结果连同理由一起传进 native，原样出版进证据（AC5）。
 */
class FfmpegPlaybackSession(
    private val mediaFile: File,
    private val displayName: String,
    private val routingDecision: String,
    private val routingReason: String,
    /**
     * 网络片源（SMB / WebDAV / HTTP）：每次调用须返回**新** fd，由本类持有到 [close]。
     * 为空时走 [mediaFile] 本地路径。fd 必须活过整个会话——native 只借用不持有。
     */
    private val descriptorSource: (() -> ParcelFileDescriptor)? = null,
    private val sourceLength: Long = -1L
) {
    /**
     * 网络片源的便利构造：没有本地文件，但走完全同一条 fd 软解路径。
     *
     * 存在的理由：SMB 片源自始至终只有一个 `ParcelFileDescriptor`（`RangeFd` 产出），
     * 而软解会话要的是"能随机读的 fd + 长度"——两者本就是同一个东西。
     * 为了凑一个 `File` 而把片子落到磁盘是倒退，所以这里显式区分两种输入。
     */
    constructor(
        displayName: String,
        descriptorSource: () -> ParcelFileDescriptor,
        sourceLength: Long,
        routingDecision: String,
        routingReason: String
    ) : this(
        File("network-media"), displayName, routingDecision, routingReason,
        descriptorSource, sourceLength
    )

    private var handle = 0L
    private var videoFd: ParcelFileDescriptor? = null
    var lastError = ""
        private set

    /** 打开软解会话并绑定 Surface。成功返回 0。 */
    fun openVideo(surface: Surface): Int {
        if (handle != 0L) {
            lastError = "openVideo called twice"
            return -1
        }
        val fd = runCatching {
            descriptorSource?.invoke()
                ?: ParcelFileDescriptor.open(mediaFile, ParcelFileDescriptor.MODE_READ_ONLY)
        }.getOrNull()
        if (fd == null) {
            lastError = "ParcelFileDescriptor.open failed"
            return -1
        }
        videoFd = fd
        val length = if (sourceLength >= 0) sourceLength else mediaFile.length()

        handle = runCatching { NativePlayer.nativeFfmpegCreate() }.getOrElse { 0L }
        if (handle == 0L) {
            lastError = "nativeFfmpegCreate failed"
            return -1
        }
        val rc = runCatching {
            NativePlayer.nativeFfmpegOpen(
                handle,
                displayName,
                fd.fd,
                length,
                surface,
                routingDecision,
                routingReason
            )
        }.getOrElse { -1 }
        if (rc != 0) {
            lastError = runCatching {
                NativePlayer.nativeFfmpegLastError(handle)
            }.getOrElse { "open failed rc=$rc" }
            return rc
        }
        return 0
    }

    /**
     * URL 输入（220）：本地 Range 代理（http://127.0.0.1:port/...）走 FFmpeg
     * network+http 协议直开。fd 相关逻辑全部不参与。
     */
    fun openUrlVideo(url: String, surface: Surface): Int {
        if (handle != 0L) {
            lastError = "openUrlVideo called twice"
            return -1
        }
        handle = NativePlayer.nativeFfmpegCreate()
        if (handle == 0L) {
            lastError = "nativeFfmpegCreate failed"
            return -1
        }
        val rc = NativePlayer.nativeFfmpegOpenUrl(
            handle, displayName, url, surface, routingDecision, routingReason
        )
        if (rc != 0) {
            lastError = runCatching {
                NativePlayer.nativeFfmpegLastError(handle)
            }.getOrElse { "openUrlVideo failed rc=$rc" }
            NativePlayer.nativeFfmpegDestroy(handle)
            handle = 0L
        }
        return rc
    }

    /** seek（220）：异步请求——worker 线程内执行（avformat_seek_file + flush + 重锚）。 */
    fun seekTo(targetUs: Long) {
        runCatching { NativePlayer.nativeFfmpegSeekTo(handle, targetUs) }
    }

    /**
     * 把"正在出声的音频位置"交给视频会话做主时钟（µs）。传 -1 退回自身 pts 节拍。
     *
     * 这是音画同步的唯一开关：不推的话，视频只守自己的表，两条 1.0x 时间轴一旦
     * 起点错开（音频要 demux + 开 AAudio，实测晚 1.4 s）就会**永远**错开。
     */
    fun setAudioClockUs(us: Long) {
        if (handle == 0L) return
        runCatching { NativePlayer.nativeFfmpegSetAudioClockUs(handle, us) }
    }

    private var pausedFlag = false

    /**
     * 暂停 / 恢复。返回 0 = 成功。
     *
     * 语义是"会话内阻塞取包与呈现"，不是关闭会话——解码器、SMB 连接、表面都保留，
     * 因此恢复没有重开代价。暂停中 [seekTo] 依然生效。
     */
    fun setPaused(paused: Boolean): Int {
        val rc = runCatching {
            NativePlayer.nativeFfmpegSetPaused(handle, paused)
        }.getOrElse { -1 }
        if (rc == 0) pausedFlag = paused
        return rc
    }

    fun isPaused(): Boolean = pausedFlag

    /** 当前媒体位置（µs）；-1 = 未知。 */
    fun positionUs(): Long = runCatching {
        NativePlayer.nativeFfmpegPositionUs(handle)
    }.getOrElse { -1L }

    /** 容器时长（µs）；-1 = 未知。 */
    fun durationUs(): Long = runCatching {
        NativePlayer.nativeFfmpegDurationUs(handle)
    }.getOrElse { -1L }

    /** 起解线程。 */
    fun start(): Int = runCatching {
        NativePlayer.nativeFfmpegStart(handle)
    }.getOrElse { -1 }

    /** 线程是否还在跑（自然 EOS 后为 false）。 */
    fun isRunning(): Boolean = handle != 0L && runCatching {
        NativePlayer.nativeFfmpegRunning(handle)
    }.getOrDefault(false)

    /** 停线程并拿证据 JSON（在 [close] 之前调）。 */
    fun stopAndCollectEvidence(): String = runCatching {
        NativePlayer.nativeFfmpegStop(handle)
    }.getOrElse { "{}" }

    /** 最近一次失败原因（JNI 通道）。 */
    fun nativeError(): String = runCatching {
        NativePlayer.nativeFfmpegLastError(handle)
    }.getOrElse { lastError }

    /** 关闭顺序：先 stop（证据已拿走）再销毁句柄、最后关 fd。 */
    fun close() {
        if (handle != 0L) {
            runCatching { NativePlayer.nativeFfmpegDestroy(handle) }
            handle = 0L
        }
        runCatching { videoFd?.close() }
        videoFd = null
    }
}

/**
 * 路由判定（211 设计稿 D1）：**在起播之前**，两条判据按序：
 * 1. 容器可解——`MediaExtractor` 能给出视频轨；
 * 2. 解码可解——`MediaCodecList` 里有解码器（含软件解码器，MediaCodec 路径包含它们）。
 *
 * 两条都过 → 硬解路径（历史默认，行为零回归，AC4 的 4K 守门在这里）；
 * 任何一条不过 → 软解，理由原样出版（G9：不许静默兜底）。
 * 匹配过程抛异常 → 走 mediacodec（历史行为），路由必须**加法**，不吞错降级。
 */
object FfmpegRouting {

    data class Decision(val useFfmpeg: Boolean, val decision: String, val reason: String)

    /**
     * 路由探测的截止时间。
     *
     * 4 s 的依据：正常 MKV（头部 + 尾部索引）在网络 fd 上实测 <300 ms；
     * 给 4 s 是十倍余量，同时把"整片扫描"限制在 4 s 内（按 15 MB/s 约 60 MiB，
     * 相对 12 GB 可忽略）。到点关 fd，结论退回平台解码。
     */
    private const val PROBE_TIMEOUT_MS = 4_000L

    /**
     * 网络片源（SMB / WebDAV / HTTP）的判定入口。
     *
     * 与 [probe] 的唯一差别是"怎么把字节喂给平台探测"：本地文件给路径，
     * 网络片源只能给 fd。**判据本身一字不改**——两条路径必须给出同一个结论，
     * 否则同一个片源换个传输方式就换了解码后端。
     *
     * fd 打不开属于故障而不是能力缺失，但也不能阻断播放：判到软解至少还有机会播，
     * 判到硬解则必然失败。理由原文进证据。
     */
    fun probe(input: com.kivo.cinema.net.RangeSource): Decision {
        val pfd = runCatching { input.openDescriptor() }.getOrElse {
            return Decision(
                true, "ffmpeg",
                "cannot open source fd (${it.javaClass.simpleName}); assuming platform cannot decode"
            )
        }
        // 截止时间（252）：某些 MKV 头部没有可用索引，`MediaExtractor.setDataSource`
        // 会**从头线性扫完整片**。真机实测（Shelter，12.19 GB）：15.66 MB/s 连续下载、
        // 13 分钟不返回，播放页永远停在「正在连接SMB…」，既无错误也无解码器。
        //
        // 中断手段是 `cancel()` 而不是 `close()`：关闭代理 fd 会让阻塞中的读等不到
        // 回调线程（实测直接黑屏、连日志都没有）；cancel 只是把标志置上，
        // 下一次读立刻抛 EIO，解析器自己收尾退出。
        val timedOut = java.util.concurrent.atomic.AtomicBoolean(false)
        val watchdog = java.util.Timer("KivoRouteProbe", true).apply {
            schedule(object : java.util.TimerTask() {
                override fun run() {
                    timedOut.set(true)
                    runCatching { input.cancel() }
                }
            }, PROBE_TIMEOUT_MS)
        }
        return try {
            val extractor = android.media.MediaExtractor()
            val containerOk = runCatching {
                extractor.setDataSource(pfd.fileDescriptor, 0, input.size)
            }.isSuccess
            if (!containerOk) {
                runCatching { extractor.release() }
                if (timedOut.get()) {
                    return Decision(
                        false, "mediacodec",
                        "probe timed out after ${PROBE_TIMEOUT_MS} ms（容器需要整片扫描）；保持平台解码",
                    )
                }
                return Decision(
                    true, "ffmpeg",
                    "MediaExtractor.setDataSource failed over fd (container unsupported)",
                )
            }
            val decided = try {
                decide(extractor)
            } finally {
                runCatching { extractor.release() }
            }
            // 截止优先于任何结论：超时说明解析器没能在预算内把容器读明白，
            // 它此时给出的"没有视频轨"是**读不完**而不是**真没有**。
            // 真机（Shelter）：4.1 s 超时后返回 "no video track"，据此退软解，
            // 结果 0.37x 卡成 PPT；而平台解码实测 27 fps / 0 丢帧。
            if (timedOut.get()) {
                return Decision(
                    false, "mediacodec",
                    "probe timed out after ${PROBE_TIMEOUT_MS} ms（容器需要整片扫描）；保持平台解码",
                )
            }
            return decided
        } finally {
            watchdog.cancel()
            // 超时那条路**不关 fd**：解析器此时可能还阻塞在这个 fd 的读上，
            // 关掉它会把准备线程一起挂住（真机实测：黑屏、零流量、既不报错也不起播）。
            // 让它随探测对象一起被回收，代价是一个 fd，比卡死划算。
            if (!timedOut.get()) runCatching { pfd.close() }
        }
    }

    fun probe(media: File): Decision {
        val extractor = android.media.MediaExtractor()
        // 判据 1（容器可解）：setDataSource 抛异常 = 平台明说"读不了" → 软解。
        // 首版这里把整个探测体包进一个大 catch、一律回 mediacodec——
        // 结果 rm 片（setDataSource 直接抛）被误路由进硬解路径然后 open 失败，
        // 真机当场暴露。判据的两层失败各有归属，不许合并成一个 catch。
        val containerOk = runCatching {
            extractor.setDataSource(media.absolutePath)
        }.isSuccess
        if (!containerOk) {
            runCatching { extractor.release() }
            return Decision(
                true, "ffmpeg",
                "MediaExtractor.setDataSource failed (container unsupported)"
            )
        }
        return try {
            decide(extractor)
        } finally {
            runCatching { extractor.release() }
        }
    }

    /**
     * 容器已打开之后的判据 2：有没有视频轨、平台有没有解码器。
     *
     * 抽出来是因为本地与网络两条入口只有"打开方式"不同，判据必须共用——
     * 复制一份就意味着以后改判定会漏掉一条路径。
     */
    private fun decide(extractor: android.media.MediaExtractor): Decision {
        var videoFormat: android.media.MediaFormat? = null
        for (i in 0 until extractor.trackCount) {
            val format = extractor.getTrackFormat(i)
            val mime = format.getString(android.media.MediaFormat.KEY_MIME) ?: continue
            if (mime.startsWith("video/")) {
                videoFormat = format
                break
            }
        }
        val format = videoFormat
            ?: return Decision(
                true, "ffmpeg",
                "MediaExtractor found no video track (container likely unsupported)"
            )
        // 判据 2（解码可解）：平台找不到解码器（含软解解码器）→ 软解。
        val mime = format.getString(android.media.MediaFormat.KEY_MIME) ?: "?"
        val decoderName = runCatching {
            android.media.MediaCodecList(android.media.MediaCodecList.REGULAR_CODECS)
                .findDecoderForFormat(format)
        }.getOrNull()
        if (decoderName != null) {
            return Decision(false, "mediacodec", "decoder '$decoderName' available for $mime")
        }

        // 兜底：findDecoderForFormat 会拿 MediaFormat 里的 profile/level 去严格比对设备
        // 声明的 profile 表，表里没列到就拒绝。实机证据（p249）：本机 c2.qti.hevc.decoder
        // 支持 4096x2304、1080p 实测 118~259 fps，但 profile 表未声明 Main10，
        // 于是 10bit HEVC 片源被判成"平台无解码器"、整片退到软解 —— 1080p 10bit 软解
        // 在这台设备上只有 20 fps 上下（sws 单帧就 21ms），观感就是卡成 PPT。
        // 同一个文件用系统播放器打开可正常硬解（MediaCodecList 只按 mime 匹配）。
        // 所以这里补一条"按 mime 找硬件解码器 + 校验尺寸"的兜底，只认硬件，不放软解进来。
        val hardware = runCatching { findHardwareDecoder(format, mime) }.getOrNull()
        return if (hardware != null) {
            Decision(
                false, "mediacodec",
                "hardware decoder '$hardware' via mime fallback for $mime (profile table lacks this profile)"
            )
        } else {
            Decision(true, "ffmpeg", "no MediaCodec decoder for $mime")
        }
    }

    /**
     * 按 mime 找**硬件**视频解码器并校验尺寸。找不到返回 null。
     *
     * 只认硬件：软解解码器（OMX.google.* / c2.android.*）能创建不代表能实时播放，
     * 把它们放进硬解路径只会换来更差的软解 + 一条没走通的同步链路。
     */
    private fun findHardwareDecoder(
        format: android.media.MediaFormat,
        mime: String,
    ): String? {
        val width = runCatching { format.getInteger(android.media.MediaFormat.KEY_WIDTH) }.getOrNull() ?: -1
        val height = runCatching { format.getInteger(android.media.MediaFormat.KEY_HEIGHT) }.getOrNull() ?: -1
        val list = android.media.MediaCodecList(android.media.MediaCodecList.REGULAR_CODECS)
        for (info in list.codecInfos) {
            if (info.isEncoder) continue
            if (!info.supportedTypes.any { it.equals(mime, ignoreCase = true) }) continue
            if (!isHardwareCodec(info)) continue
            val video = runCatching { info.getCapabilitiesForType(mime).videoCapabilities }
                .getOrNull() ?: continue
            if (width > 0 && height > 0) {
                val sizeOk = runCatching { video.isSizeSupported(width, height) }.getOrNull() ?: false
                if (!sizeOk) continue
            }
            return info.name
        }
        return null
    }

    /** API 29 起有权威标记；更低版本退化为按名字排除已知软解实现。 */
    private fun isHardwareCodec(info: android.media.MediaCodecInfo): Boolean {
        if (android.os.Build.VERSION.SDK_INT >= 29) {
            return runCatching { info.isHardwareAccelerated }.getOrDefault(false)
        }
        val n = info.name.lowercase(java.util.Locale.US)
        return !n.startsWith("omx.google.") && !n.startsWith("c2.android.") &&
            !n.contains("software")
    }
}
