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
    private val routingReason: String
) {

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
            ParcelFileDescriptor.open(mediaFile, ParcelFileDescriptor.MODE_READ_ONLY)
        }.getOrNull()
        if (fd == null) {
            lastError = "ParcelFileDescriptor.open failed"
            return -1
        }
        videoFd = fd

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
                mediaFile.length(),
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
            if (format == null) {
                Decision(
                    true, "ffmpeg",
                    "MediaExtractor found no video track (container likely unsupported)"
                )
            } else {
                // 判据 2（解码可解）：平台找不到解码器（含软解解码器）→ 软解。
                val mime = format.getString(android.media.MediaFormat.KEY_MIME) ?: "?"
                val decoderName = runCatching {
                    android.media.MediaCodecList(android.media.MediaCodecList.REGULAR_CODECS)
                        .findDecoderForFormat(format)
                }.getOrNull()
                if (decoderName == null) {
                    Decision(true, "ffmpeg", "no MediaCodec decoder for $mime")
                } else {
                    Decision(false, "mediacodec", "decoder '$decoderName' available for $mime")
                }
            }
        } finally {
            runCatching { extractor.release() }
        }
    }
}
