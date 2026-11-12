package com.kivo.cinema.ui.compose

import android.view.SurfaceHolder
import com.kivo.cinema.smb.SmbAddress
import com.kivo.cinema.smb.SmbByteSource
import com.kivo.cinema.smb.SmbCredential

/** SMB 起播：mpv / 硬解 / 软解三链路互斥；软解音频独立字节源。 */

internal fun PlayerComposeActivity.startSmbPlayback(
    holder: SurfaceHolder, sourceId: String, filePath: String, shareOverride: String = "",
) {
    playbackStarted = true
    statusText = "正在连接SMB…"
    val generation = ++networkGeneration
    val cancelled = java.util.concurrent.atomic.AtomicBoolean(false)
    networkPreparationCancelled = cancelled
    Thread({
        var source: SmbByteSource? = null
        var thumbSource: SmbByteSource? = null
        try {
            val record = com.kivo.cinema.sources.storage.SourceStore(this)
                .read().getOrNull()?.firstOrNull { it.id == sourceId }
                ?: throw IllegalStateException("找不到这个资源库，可能已被移除")
            val parsed = SmbAddress.parse(record.address)
            val address = if (shareOverride.isNotBlank()) parsed.copy(share = shareOverride) else parsed
            check(!address.needsShare) { "这个资源库还没有选择共享" }
            val opened = SmbByteSource.open(
                address, SmbCredential(username = record.username, password = record.password), filePath,
                online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) },
                // MIUI 过滤应用 logcat：读画像必须落盘才看得到（252 实测 50s 只剩 74 行）。
                reportSink = { line -> smbTrace("smbread $line") },
            )
            source = opened
            smbTrace("prepared open size=${opened.size()}")
            // v3.1：缩略图第二读路径（随机读、预读关）。与主读共用会互顶预读窗口（252）。
            // 打不开不致命：预览浮窗保持形状、内容留空，取证记 decode_failures。
            thumbSource = runCatching {
                SmbByteSource.open(
                    address, SmbCredential(username = record.username, password = record.password), filePath,
                    online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) },
                )
            }.getOrNull()
            // v3.1c：seek 后 demux 是顺序读簇，预读把 8MiB 回填从几十次 SMB
            // 往返（真机 1.0–2.9s/帧）压成一次；本源独立于主读，不互顶窗口。
            thumbSource?.setPrefetchEnabled(true)
            check(!cancelled.get())
            val range = com.kivo.cinema.net.RangeFd(
                opened, getSystemService(android.os.storage.StorageManager::class.java),
                online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) },
            )
            // 网络片源不做容器探测（252）：Shelter 12GB 无索引，线性扫全片 13 分钟不返回。
            // 先按平台解码走，解不了第一帧再退软解。
            val routing = com.kivo.cinema.bridge.FfmpegRouting.Decision(
                useFfmpeg = false, decision = "mediacodec",
                reason = "网络片源跳过容器探测（252）；平台解不了再退软解",
            )
            smbTrace("routing=${routing.decision} reason=${routing.reason}")
            runOnUiThread {
                if (cancelled.get() || finishRequested || isDestroyed || generation != networkGeneration || !surfaceReady) {
                    Thread {
                        runCatching { range.close() }
                        runCatching { opened.close() }
                        runCatching { thumbSource?.close() }
                    }.start()
                    return@runOnUiThread
                }
                networkPreparationCancelled = null
                // 缩略图与弹幕和内核引擎无关，必须在引擎分支**之前**挂：
                // mpv 分支会 return，放在分支后等于日常路径永远不挂（真机实测）。
                if (thumbSource != null) mountNetworkThumbnails(thumbSource)
                loadDanmaku(filePath)
                val engine = com.kivo.cinema.playback.PlaybackEngine.select()
                if (engine == com.kivo.cinema.playback.PlaybackEngine.Engine.MPV) {
                    smbTrace("engine=mpv")
                    startMpvPlayback(holder, opened)
                    return@runOnUiThread
                }
                if (routing.useFfmpeg) {
                    opened.setPrefetchEnabled(true)   // 软解顺序流式读，预读命中实测 100%
                    startSoftPlayback(holder, range, routing, filePath, generation, cancelled,
                        address, SmbCredential(username = record.username, password = record.password))
                } else {
                    // 硬解是突发读：预读会跟主读抢 SMB 链路（实测 24→20.27fps），关掉。
                    opened.setPrefetchEnabled(false)
                    val net = com.kivo.cinema.playback.NetworkPlaybackSession(this, range)
                    startNetworkPlayback(holder, net, "smb") {
                        // 平台解不了（VC-1）→ 重开源走软解。不能复用原 range：会话关闭会连 source 一起关。
                        smbTrace("platform decode failed (${net.lastError}) -> ffmpeg (rebind)")
                        runCatching { net.stop() }
                        val credentials = SmbCredential(record.username, record.password)
                        Thread {
                            try {
                                val fresh = SmbByteSource.open(address, credentials, filePath,
                                    online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) })
                                fresh.setPrefetchEnabled(true)
                                val freshRange = com.kivo.cinema.net.RangeFd(
                                    fresh, getSystemService(android.os.storage.StorageManager::class.java),
                                    online = { com.kivo.cinema.net.NetworkLinkMonitor.isOnline(applicationContext) })
                                runOnUiThread {
                                    if (cancelled.get() || finishRequested || isDestroyed) {
                                        runCatching { freshRange.close() }
                                        return@runOnUiThread
                                    }
                                    startSoftPlayback(holder, freshRange,
                                        com.kivo.cinema.bridge.FfmpegRouting.Decision(
                                            true, "ffmpeg", "platform decode failed; fallback (252)"),
                                        filePath, generation, cancelled, address, credentials)
                                }
                            } catch (e: Exception) {
                                runOnUiThread {
                                    statusText = "这个片源无法播放：${e.message?.take(80)}"
                                    controlsVisible = true
                                }
                            }
                        }.start()
                    }
                }
            }
        } catch (e: Exception) {
            runCatching { source?.close() }
            runCatching { thumbSource?.close() }
            runOnUiThread {
                if (!cancelled.get() && !finishRequested && !isDestroyed && generation == networkGeneration) {
                    networkPreparationCancelled = null
                    statusText = e.message?.takeIf { it.isNotBlank() } ?: "SMB连接失败（${e.javaClass.simpleName}）"
                    isPlaying = false
                    controlsVisible = true
                }
            }
        }
    }, "KivoSmbPrepare").start()
}

/** mpv 内核起播：吃 ByteSource（SMB/HTTP，代理按 offset 读），与 fd 链路互斥。 */
internal fun PlayerComposeActivity.startMpvPlayback(holder: SurfaceHolder, source: com.kivo.cinema.net.ByteSource) {
    if (finishRequested || isDestroyed) return
    playbackStarted = true
    smbPlayback = true
    statusText = "正在准备内核播放…"
    val session = com.kivo.cinema.playback.MpvPlaybackSession(this, source)
    mpvNetSession = session
    session.start(holder.surface) { result ->
        if (finishRequested || isDestroyed || mpvNetSession !== session) return@start
        if (result == 0) {
            if (session.durationUs > 0) durationMs = session.durationUs / 1000
            statusText = "正在播放"
            isPlaying = true
            lastTouchMs = System.currentTimeMillis()
            android.os.Handler(mainLooper).post(positionPoll)
            applyProbeSubtitle(session)
            // 起播后才有 video-params：此时更新 PiP 宽高比才是真值。
            updatePipParams()
        } else {
            statusText = "网络视频无法播放：${session.lastError}"
            isPlaying = false
            controlsVisible = true
        }
    }
}

/** v3.1b：挂网络缩略图读取器（mpv 第二实例，SMB/HTTP 同构）。
 *  MediaExtractor 在无索引 MKV 上首请求即线性扫卡死（真机 handler busy 定论），
 *  网络源预览帧改走内核；本地索引源仍用 ThumbnailReader。 */
internal fun PlayerComposeActivity.mountNetworkThumbnails(source: com.kivo.cinema.net.ByteSource) {
    netThumbSource = source
    val reader = com.kivo.cinema.thumbnail.MpvThumbnailReader(applicationContext, source, cacheDir)
    thumbnailReader = reader
    // 预热期空框也要有字：内核起好前拖动，用户看到的是"准备中"而不是黑框猜谜。
    thumbnailUnavailableReason = "预览帧准备中…"
    reader.onThumbnailReady = { _, bitmap ->
        android.os.Handler(mainLooper).post {
            thumbnailBitmap = bitmap
            thumbnailUnavailableReason = ""
        }
    }
    reader.onUnavailable = { reason ->
        android.os.Handler(mainLooper).post { thumbnailUnavailableReason = reason }
    }
    reader.prewarm()
}

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
    statusText = "正在用软解打开…"
    smbSoftRange = range
    val surface = holder.surface
    if (surface == null || !surface.isValid) {
        statusText = "播放窗口还没准备好"
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