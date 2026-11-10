package com.kivo.cinema.playback

import android.os.ParcelFileDescriptor
import android.util.Log
import com.kivo.cinema.bridge.NativePlayer

/**
 * 软解音频门面（VC-1 这类片源的配套声道）。
 *
 * 为什么需要它：设备的硬解清单里只有 AAC / MP3 / FLAC / Opus / Vorbis，
 * TrueHD / AC3 / EAC3 / DTS 一个都没有（真机 `media_codecs` 为证）。
 * `AudioPlayback` 走的是 MediaExtractor + MediaCodec，对这类音轨在第一步
 * 就失败了，所以只能整条链路自己来：FFmpeg 解封装 → FFmpeg 解码 →
 * swresample → 环形缓冲 → AAudio。
 *
 * fd 约定与视频软解一致：调用方传进来一个 fd，本类**不持有**它
 * （`RangeFd` 的 PFD 由 Activity 持有），`close()` 也不会关它。
 */
class SoftAudioSession(
    private val displayName: String,
    private val fd: Int,
    private val length: Long,
) {
    private var handle = 0L
    private var started = false
    var lastError = ""
        private set

    /** 打开并起线程。返回 0 = 成功（线程已起，真正的打开在后台线程里）。 */
    fun start(): Int {
        if (handle != 0L) {
            lastError = "start called twice"
            return -1
        }
        handle = runCatching { NativePlayer.nativeSoftAudioCreate() }.getOrElse { 0L }
        if (handle == 0L) {
            lastError = "nativeSoftAudioCreate failed"
            return -1
        }
        val openRc = runCatching {
            NativePlayer.nativeSoftAudioOpen(handle, displayName, fd, length)
        }.getOrElse { -1 }
        if (openRc != 0) {
            lastError = "audio open rc=$openRc"
            runCatching { NativePlayer.nativeSoftAudioDestroy(handle) }
            handle = 0L
            return openRc
        }
        val startRc = runCatching { NativePlayer.nativeSoftAudioStart(handle) }.getOrElse { -1 }
        if (startRc != 0) {
            lastError = "audio start rc=$startRc"
            runCatching { NativePlayer.nativeSoftAudioDestroy(handle) }
            handle = 0L
            return startRc
        }
        started = true
        return 0
    }

    fun setPaused(paused: Boolean) {
        if (handle != 0L) {
            runCatching { NativePlayer.nativeSoftAudioSetPaused(handle, paused) }
        }
    }

    fun seekTo(targetUs: Long) {
        if (handle != 0L) {
            runCatching { NativePlayer.nativeSoftAudioSeekTo(handle, targetUs) }
        }
    }

    fun positionUs(): Long =
        if (handle == 0L) -1L else runCatching {
            NativePlayer.nativeSoftAudioPositionUs(handle)
        }.getOrElse { -1L }

    /**
     * 实际正在出声的媒体位置（µs）。
     *
     * 音画对齐必须用这个而不是 [positionUs]：后者领先实际出声最多 700 ms。
     */
    fun playedUs(): Long =
        if (handle == 0L) -1L else runCatching {
            NativePlayer.nativeSoftAudioPlayedUs(handle)
        }.getOrElse { -1L }

    fun durationUs(): Long =
        if (handle == 0L) -1L else runCatching {
            NativePlayer.nativeSoftAudioDurationUs(handle)
        }.getOrElse { -1L }

    fun isRunning(): Boolean = handle != 0L && runCatching {
        NativePlayer.nativeSoftAudioRunning(handle)
    }.getOrDefault(false)

    /** 是否真的出得了声（AAudio 流开成功）。用于把"片源没有音轨"与"设备打不开"分开。 */
    fun hasOutput(): Boolean = handle != 0L && runCatching {
        NativePlayer.nativeSoftAudioHasOutput(handle)
    }.getOrDefault(false)

    fun statsJson(): String =
        if (handle == 0L) "{}" else runCatching {
            NativePlayer.nativeSoftAudioStats(handle)
        }.getOrElse { "{}" }

    /** 停线程并释放句柄。**不关 fd**——那是 RangeFd 的所有权。 */
    fun close() {
        if (handle != 0L) {
            runCatching { NativePlayer.nativeSoftAudioStop(handle) }
            runCatching { NativePlayer.nativeSoftAudioDestroy(handle) }
            handle = 0L
        }
        started = false
    }

    /** 失败原因（从 native 统计里捞，避免只看一个返回码）。 */
    fun describeFailure(): String {
        val stats = statsJson()
        val error = Regex("\"error\":\"([^\"]*)\"").find(stats)?.groupValues?.get(1).orEmpty()
        return if (error.isNotBlank()) error else lastError.ifBlank { "音频无法打开" }
    }
}

/** 便利构造：从一个已经打开的 PFD 起会话。
 *
 * [length] 必须显式传：代理描述符的 `statSize` 并不保证被填充（实测可能是 -1），
 * 而 -1 会让 FFmpeg 拿不到文件长度，mkv 的 Cues 就定位不了。
 */
fun SoftAudioSession(
    displayName: String,
    descriptor: ParcelFileDescriptor,
    length: Long,
): SoftAudioSession {
    Log.i("KivoSoftAudio", "open $displayName fd=${descriptor.fd} len=$length")
    return SoftAudioSession(displayName, descriptor.fd, length)
}
