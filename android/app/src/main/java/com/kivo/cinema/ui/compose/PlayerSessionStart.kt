package com.kivo.cinema.ui.compose

import android.util.Log
import android.view.SurfaceHolder
import com.kivo.cinema.bridge.FfmpegRouting
import com.kivo.cinema.bridge.PlaybackSession
import com.kivo.cinema.ui.compose.player.PlayerPhase

/** 起播入口：本地 / URL / 网络硬解三条链路的分发与启动。SMB 与软解见 PlayerSmbStart。 */

internal fun PlayerComposeActivity.maybeStartPlayback(holder: SurfaceHolder) {
    if (finishRequested || isDestroyed) return
    surfaceReady = true
    // 280 · 指标起点必须在这里（**所有**起播路径的唯一入口）。
    // 踩过：begin 原来放在 startMpvPlayback 里，而本地/URL 路径在调用它**之前**
    // 就 mark(SOURCE_READY) ⇒ begin 把已打的标记清空了（source_ready 恒 not_measured）。
    // isActive 守卫：Surface 重建会再次进本函数，不能把同一轮记录重置掉。
    if (!com.kivo.cinema.playback.PlaybackMetrics.isActive()) {
        com.kivo.cinema.playback.PlaybackMetrics.begin(
            intent.getStringExtra("kivo.label") ?: "play",
            com.kivo.cinema.playback.PlaybackStack.resolve(this, intent).renderMode,
        )
    }
    // 278-P2：真 HDR 输出要在 **交给 mpv 之前** 把 Surface 配成 10bit + BT2020_PQ
    // （EGL 按当时的 buffer 格式挑 config ⇒ 事后改就晚了）。
    // 默认关闭（见 [HdrOutputSwitch]）；失败自动退回 8bit，不影响播放。
    // 【为什么在这里而不是 onSurfaceAvailable】root 走的是本函数
    // （PlayerComposeRoot 的 onSurfaceReady 直调 maybeStartPlayback）⇒ 这里是唯一入口。
    prepareHdrSurface(holder)
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
        startUrlPlayback(holder, playUrl, intent.getStringExtra("kivo.label") ?: playUrl)
        return
    }
    val media = mediaOverridePath.takeIf { it.isNotBlank() }
        ?.let { java.io.File(it) }?.takeIf { it.isFile }
        ?: locateTestMedia() ?: return
    playbackStarted = true
    // 280 · 本地文件分流：默认走 **mpv（gpu-next）**，返回 true = 已接走。
    // 落到下面的自研链路只有两种情况：内核选 legacy，或开了「后台继续播放音频」
    // （功能对等，见 [startLocalPlayback] 的文件头）。
    if (startLocalPlayback(holder, media)) return
    phase = PlayerPhase.Buffering
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
        // 不再直接 finish()：失败要**留在屏幕上**并给出原因与重试，
        // 否则用户只看到黑屏弹回上一层，不知道发生了什么（272 审计 D2）。
        localSession.close(); playbackSession = null
        phase = PlayerPhase.Failed("无法打开视频：${localSession.lastError.ifBlank { "错误码 $rc" }}")
        controlsVisible = true
        return
    }
    // 音频必须先于节拍启动，native 才会选音频主时钟。
    val audioProbe = com.kivo.cinema.bridge.MediaTrackProbe.inspectAudioTrack(media)
    if (audioProbe.isFailure) {
        statusText = "无法检查音轨，已停止播放：${audioProbe.exceptionOrNull()?.javaClass?.simpleName}"
        localSession.close(); playbackSession = null
        phase = PlayerPhase.Failed("无法检查音轨：${audioProbe.exceptionOrNull()?.javaClass?.simpleName ?: "未知错误"}")
        controlsVisible = true
        return
    }
    val hasAudio = audioProbe.getOrThrow()
    if (hasAudio) {
        val audioRc = localSession.openAudio()
        if (audioRc != 0) {
            statusText = "音频无法启动：${localSession.lastError.ifBlank { "错误码 $audioRc" }}"
            localSession.close(); playbackSession = null
            phase = PlayerPhase.Failed("音频无法启动：${localSession.lastError.ifBlank { "错误码 $audioRc" }}")
            controlsVisible = true
            return
        }
    }
    localSession.configurePacing(targetFps = 0, allowDrop = true)
    localSession.configurePipeline(split = false, present = true, atTime = true)
    statusText = "starting..."
    val startRc = localSession.start()
    statusText = "start rc=$startRc"
    if (startRc != 0) {
        Log.e(PlayerComposeActivity.TAG, "start failed rc=$startRc")
        localSession.close(); playbackSession = null
        phase = PlayerPhase.Failed("无法开始播放：${localSession.lastError.ifBlank { "错误码 $startRc" }}")
        controlsVisible = true
        return
    }
    isPlaying = true
    phase = PlayerPhase.Playing
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

internal fun PlayerComposeActivity.startNetworkPlayback(
    holder: SurfaceHolder,
    netSession: com.kivo.cinema.playback.NetworkPlaybackSession,
    routing: String,
    onPlatformFailed: (() -> Unit)? = null,
) {
    if (finishRequested || isDestroyed) return
    playbackStarted = true
    smbPlayback = routing == "smb" || routing == "emby"
    phase = PlayerPhase.Buffering
    statusText = "正在准备网络播放…"
    smbNetSession = netSession
    netSession.start(holder.surface) { result ->
        if (finishRequested || isDestroyed || smbNetSession !== netSession) return@start
        if (result == 0) {
            durationMs = netSession.durationUs / 1000
            statusText = if (netSession.hasAudio) "正在播放" else "正在播放 · 片源没有音轨"
            phase = PlayerPhase.Playing
            isPlaying = true
            lastTouchMs = System.currentTimeMillis()
            android.os.Handler(mainLooper).post(positionPoll)
            updatePipParams()
            // 273 v15：首帧之后才起保温池（URL 源；SMB 走原路径，这里是空操作）。
            startUrlWarmup()
            // 273 v15：预览内核同理（第二实例 = 第二条源站连接）。
            prewarmThumbnails()
        } else {
            val fallback = onPlatformFailed
            if (fallback != null) { fallback(); return@start }
            statusText = "网络视频无法播放：${netSession.lastError}"
            phase = PlayerPhase.Failed("网络视频无法播放：${netSession.lastError.ifBlank { "未知错误" }}")
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