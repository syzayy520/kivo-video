package com.kivo.cinema.ui.compose

import android.util.Log
import android.view.SurfaceHolder
import com.kivo.cinema.bridge.FfmpegRouting
import com.kivo.cinema.bridge.PlaybackSession

/** 起播入口：本地 / URL / 网络硬解三条链路的分发与启动。SMB 与软解见 PlayerSmbStart。 */

internal fun PlayerComposeActivity.maybeStartPlayback(holder: SurfaceHolder) {
    if (finishRequested || isDestroyed) return
    surfaceReady = true
    if (playbackStarted) {
        mpvNetSession?.rebind(holder.surface)
        smbNetSession?.rebind(holder.surface)
        playbackSession?.rebind(holder.surface)
        return
    }
    val smbSourceId = intent.getStringExtra("kivo.sourceId")
    val smbPath = intent.getStringExtra("kivo.smbPath")
    if (!smbSourceId.isNullOrBlank() && !smbPath.isNullOrBlank()) {
        startSmbPlayback(holder, smbSourceId, smbPath, intent.getStringExtra("kivo.smbShare").orEmpty())
        return
    }
    val playUrl = intent.getStringExtra("kivo.playurl")
    if (playUrl != null) {
        startUrlPlayback(holder, playUrl, intent.getStringExtra("kivo.label") ?: playUrl,
            intent.getStringExtra("kivo.routing") ?: "file")
        return
    }
    val media = mediaOverridePath.takeIf { it.isNotBlank() }
        ?.let { java.io.File(it) }?.takeIf { it.isFile }
        ?: locateTestMedia() ?: return
    playbackStarted = true
    statusText = "surface ok, probing..."
    val routing = FfmpegRouting.probe(media)
    statusText = "route=${routing.decision} reason=${routing.reason}"
    Log.i(PlayerComposeActivity.TAG, "routing: ${routing.decision} (${routing.reason})")
    val localSession = PlaybackSession(0, media, media.name)
    playbackSession = localSession
    statusText = "openVideo..."
    val rc = localSession.openVideo(holder.surface)
    statusText = "openVideo rc=$rc lastError=${localSession.lastError}"
    if (rc != 0) {
        Log.e(PlayerComposeActivity.TAG, "openVideo failed rc=$rc: ${localSession.lastError}")
        localSession.close(); playbackSession = null; finish(); return
    }
    // 音频必须先于节拍启动，native 才会选音频主时钟。
    val audioProbe = com.kivo.cinema.bridge.MediaTrackProbe.inspectAudioTrack(media)
    if (audioProbe.isFailure) {
        statusText = "无法检查音轨，已停止播放：${audioProbe.exceptionOrNull()?.javaClass?.simpleName}"
        localSession.close(); playbackSession = null; controlsVisible = true; return
    }
    val hasAudio = audioProbe.getOrThrow()
    if (hasAudio) {
        val audioRc = localSession.openAudio()
        if (audioRc != 0) {
            statusText = "音频无法启动：${localSession.lastError.ifBlank { "错误码 $audioRc" }}"
            localSession.close(); playbackSession = null; controlsVisible = true; return
        }
    }
    localSession.configurePacing(targetFps = 0, allowDrop = true)
    localSession.configurePipeline(split = false, present = true, atTime = true)
    statusText = "starting..."
    val startRc = localSession.start()
    statusText = "start rc=$startRc"
    if (startRc != 0) {
        Log.e(PlayerComposeActivity.TAG, "start failed rc=$startRc")
        localSession.close(); playbackSession = null; finish(); return
    }
    isPlaying = true
    lastTouchMs = System.currentTimeMillis()
    android.os.Handler(mainLooper).post(positionPoll)
    durationMs = probeDurationUs(media.absolutePath) / 1000
    statusText = if (hasAudio) "正在播放" else "正在播放 · 片源没有音轨"
    val reader = com.kivo.cinema.thumbnail.ThumbnailReader(media.absolutePath)
    thumbnailReader = reader
    reader.onThumbnailReady = { _, bitmap ->
        android.os.Handler(mainLooper).post { thumbnailBitmap = bitmap }
    }
    // 本地片源同样侧载弹幕（旧版只有 SMB 路径调它，本地片弹幕永远空）。
    loadDanmaku(media.absolutePath)
    updatePipParams()
    if (evidenceName == "p215") scheduleP215Stress()
    // 214：会话所有权转移给 Foreground Service，Activity 销毁后会话不死。
    com.kivo.cinema.playback.PlaybackSessionHolder.offer(localSession)
    startForegroundService(android.content.Intent(this, com.kivo.cinema.playback.PlaybackService::class.java))
    Log.i(PlayerComposeActivity.TAG, "playback started: ${media.absolutePath} (service adopted)")
}

internal fun PlayerComposeActivity.startUrlPlayback(holder: SurfaceHolder, url: String, label: String, routing: String) {
    startNetworkPlayback(holder, com.kivo.cinema.playback.NetworkPlaybackSession(this, url), routing)
}

internal fun PlayerComposeActivity.startNetworkPlayback(
    holder: SurfaceHolder,
    netSession: com.kivo.cinema.playback.NetworkPlaybackSession,
    routing: String,
    onPlatformFailed: (() -> Unit)? = null,
) {
    if (finishRequested || isDestroyed) return
    playbackStarted = true
    smbPlayback = routing == "smb"
    statusText = "正在准备网络播放…"
    smbNetSession = netSession
    netSession.start(holder.surface) { result ->
        if (finishRequested || isDestroyed || smbNetSession !== netSession) return@start
        if (result == 0) {
            durationMs = netSession.durationUs / 1000
            statusText = if (netSession.hasAudio) "正在播放" else "正在播放 · 片源没有音轨"
            isPlaying = true
            lastTouchMs = System.currentTimeMillis()
            android.os.Handler(mainLooper).post(positionPoll)
            updatePipParams()
        } else {
            val fallback = onPlatformFailed
            if (fallback != null) { fallback(); return@start }
            statusText = "网络视频无法播放：${netSession.lastError}"
            isPlaying = false
            controlsVisible = true
        }
    }
}

internal fun PlayerComposeActivity.readProbeExtras() {
    mediaOverridePath = intent.getStringExtra("kivo.media") ?: ""
    evidenceName = intent.getStringExtra("kivo.evidence") ?: ""
    playUrlOverride = intent.getStringExtra("kivo.playurl") ?: ""
}

internal fun PlayerComposeActivity.locateTestMedia(): java.io.File? {
    val dir = java.io.File(getExternalFilesDir(null), "media")
    return dir.listFiles()?.filter { it.isFile }?.minByOrNull { it.name }
}

internal fun PlayerComposeActivity.currentMediaPath(): String {
    mediaOverridePath.takeIf { it.isNotBlank() }?.let { return it }
    return locateTestMedia()?.absolutePath ?: ""
}

/** 片长：一次性 extractor 探测。不经 thumbnailReader——它的首次使用必须留给第一个 seekTo。 */
internal fun PlayerComposeActivity.probeDurationUs(path: String): Long {
    return try {
        val ex = android.media.MediaExtractor()
        ex.setDataSource(path)
        var dur = -1L
        for (i in 0 until ex.trackCount) {
            val f = ex.getTrackFormat(i)
            if (f.getString(android.media.MediaFormat.KEY_MIME)?.startsWith("video/") == true) {
                dur = f.getLong(android.media.MediaFormat.KEY_DURATION)
                break
            }
        }
        ex.release()
        dur
    } catch (e: Exception) {
        -1L
    }
}