package com.kivo.cinema.ui.compose

import com.kivo.cinema.playback.MpvPlaybackSession

internal fun PlayerComposeActivity.seekTo(ms: Long, waitLandingFirstSlice: Boolean = false) {
    val clamped = if (durationMs > 0L) ms.coerceIn(0L, durationMs) else ms.coerceAtLeast(0L)
    val seq = landingSeek.begin()
    val waitLanding = waitLandingFirstSlice && intent.getIntExtra("kivo.noWaitLanding", 0) == 0
    val source = urlPrep?.source
    val landingOffset = source?.prefetchSeek(clamped, force = true) ?: -1L
    if (waitLanding && source != null && landingOffset >= 0L) {
        seekAfterLandingReady(clamped, source, landingOffset, seq)
    } else {
        performSeek(clamped)
    }
}

private fun PlayerComposeActivity.seekAfterLandingReady(
    ms: Long,
    source: com.kivo.cinema.net.HttpOriginSource,
    landingOffset: Long,
    seq: Long,
) {
    Thread({
        val budget = source.landingWaitBudgetMs(1200L)
        val ready = source.awaitLandingReady(landingOffset, budget)
        runOnUiThread {
            if (!landingSeek.isCurrent(seq) || finishRequested || isDestroyed) return@runOnUiThread
            com.kivo.cinema.net.NetDiag.log(
                if (ready) "[landing] 落点第一片已就绪 ⇒ 跳过去（预算 ${budget}ms）"
                else "[landing] 等落点第一片超时（${budget}ms）⇒ 照常跳",
            )
            performSeek(ms)
        }
    }, "KivoLandingSeek").apply { isDaemon = true }.start()
}

private fun PlayerComposeActivity.performSeek(ms: Long) {
    val soft = smbSoftSession
    when {
        soft != null -> {
            soft.seekTo(ms * 1000)
            smbSoftAudio?.seekTo(ms * 1000)
            positionMs = ms
        }
        mpvNetSession != null -> {
            val mpv = mpvNetSession!!
            // 内核只在 PLAYING/PAUSED 接受 seek；ENDED 等状态的拒绝必须可见，不静默。
            if (mpv.state == MpvPlaybackSession.State.PLAYING || mpv.state == MpvPlaybackSession.State.PAUSED) {
                mpv.seekToAsync(ms * 1000, exact = playbackPrefs.seekExact)
            } else {
                showToast("无法调整进度：尚未起播或会话已关闭")
            }
        }
        smbNetSession != null -> smbNetSession?.seekTo(ms * 1000)
        else -> playbackSession?.seekTo(ms * 1000)
    }
}
