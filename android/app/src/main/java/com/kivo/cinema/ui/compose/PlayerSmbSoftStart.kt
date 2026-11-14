package com.kivo.cinema.ui.compose

import android.view.SurfaceHolder
import com.kivo.cinema.smb.SmbAddress
import com.kivo.cinema.smb.SmbByteSource
import com.kivo.cinema.smb.SmbCredential
import com.kivo.cinema.ui.compose.player.PlayerPhase

/**
 * SMB 软解链路：软解起播 + 软解音频挂载（从 PlayerSmbStart 抽出，控行数）。
 * 唯一职责 = 平台解不了时的兜底播放（ffmpeg）与它自己的音频字节源；
 * 平台解/硬解与 mpv 内核链路仍在 PlayerSmbStart.kt。
 */

/** 软解起播：open 会读文件头（SMB 一次往返），整段后台线程，结果回主线程落状态。 */
internal fun PlayerComposeActivity.startSoftPlayback(
    holder: SurfaceHolder,
    range: com.kivo.cinema.net.RangeFd,
    routing: com.kivo.cinema.bridge.FfmpegRouting.Decision,
    filePath: String,
    generation: Int,
    cancelled: java.util.concurrent.atomic.AtomicBoolean,
    address: SmbAddress,
    credential: SmbCredential,
) {
    playbackStarted = true
    smbPlayback = true
    phase = PlayerPhase.Buffering
    statusText = "正在用软解打开…"
    smbSoftRange = range
    val surface = holder.surface
    if (surface == null || !surface.isValid) {
        statusText = "播放窗口还没准备好"
        phase = PlayerPhase.Failed("播放窗口还没准备好")
        isPlaying = false
        controlsVisible = true
        return
    }
    Thread({
        val label = filePath.substringAfterLast('/').ifBlank { "smb-media" }
        val session = com.kivo.cinema.bridge.FfmpegPlaybackSession(
            label, { range.openDescriptor() }, range.size, routing.decision, routing.reason)
        var failure: String? = null
        try {
            val openRc = session.openVideo(surface)
            if (openRc != 0) {
                failure = "软解打不开这个片源：${session.nativeError().ifBlank { "rc=$openRc" }}"
            } else {
                val startRc = session.start()
                if (startRc != 0) failure = "软解启动失败：${session.nativeError().ifBlank { "rc=$startRc" }}"
            }
        } catch (e: Exception) {
            failure = "软解启动异常（${e.javaClass.simpleName}）"
        }
        val reason = failure
        if (reason != null) {
            runCatching { session.close() }
            runOnUiThread {
                if (cancelled.get() || finishRequested || isDestroyed || generation != networkGeneration) return@runOnUiThread
                networkPreparationCancelled = null
                statusText = reason
                phase = PlayerPhase.Failed(reason)
                isPlaying = false
                controlsVisible = true
            }
            return@Thread
        }
        runOnUiThread {
            if (cancelled.get() || finishRequested || isDestroyed || generation != networkGeneration) {
                Thread { runCatching { session.close() } }.start()
                return@runOnUiThread
            }
            networkPreparationCancelled = null
            smbSoftSession = session
            smbSoftAudioGated = false
            softRateMarkMs = 0L; softRateMarkPosUs = -1L; softAudioMarkUs = -1L
            val dur = session.durationUs()
            durationMs = if (dur > 0) dur / 1000 else 0L
            statusText = "正在播放 · 软解"
            phase = PlayerPhase.Playing
            isPlaying = true
            lastTouchMs = System.currentTimeMillis()
            android.os.Handler(mainLooper).post(positionPoll)
            Thread({ mountSoftAudio(label, address, credential, filePath, generation) }, "KivoSmbSoftAudio").start()
        }
    }, "KivoSmbSoftPrepare").start()
}

/** 软解音频用自己的 SmbByteSource/RangeFd/fd：共用会让 1MiB 预读窗口互相顶掉→PPT。 */
internal fun PlayerComposeActivity.mountSoftAudio(
    label: String, address: SmbAddress, credential: SmbCredential, filePath: String, generation: Int,
) {
    var src: SmbByteSource? = null
    var audioRange: com.kivo.cinema.net.RangeFd? = null
    var fd: android.os.ParcelFileDescriptor? = null
    try {
        src = SmbByteSource.open(address, credential, filePath,
            online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) })
        src.setPrefetchEnabled(true)   // 顺序流式读；关掉每 1MiB 同步等往返（fill_avg 213ms）
        audioRange = com.kivo.cinema.net.RangeFd(
            src, getSystemService(android.os.storage.StorageManager::class.java),
            online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) })
        fd = audioRange.openDescriptor()
        val audio = com.kivo.cinema.playback.SoftAudioSession(label, fd, audioRange.size)
        if (audio.start() != 0) {
            smbTrace("soft audio start failed: ${audio.describeFailure()}")
            runCatching { audio.close() }; runCatching { fd.close() }
            runCatching { audioRange.close() }; runCatching { src.close() }
            return
        }
        val keepSrc = src; val keepRange = audioRange; val keepFd = fd
        runOnUiThread {
            if (generation != networkGeneration || smbSoftSession == null) {
                Thread({
                    runCatching { audio.close() }; runCatching { keepFd.close() }
                    runCatching { keepRange.close() }; runCatching { keepSrc.close() }
                }, "KivoSmbSoftAudioDrop").start()
                return@runOnUiThread
            }
            smbSoftAudio = audio
            smbSoftAudioPfd = keepFd
            smbSoftAudioRange = keepRange
            smbSoftAudioSource = keepSrc
            smbTrace("soft audio mounted: own source + own window")
        }
    } catch (e: Exception) {
        smbTrace("soft audio source failed: ${e.message?.take(120)}")
        runCatching { fd?.close() }; runCatching { audioRange?.close() }; runCatching { src?.close() }
    }
}
