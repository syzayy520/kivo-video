package com.kivo.cinema.ui.compose

/**
 * 后台 / 恢复生命周期（262）：设置页"后台继续播放音频""后台恢复时自动播放"的消费端。
 *
 * 从 Activity 分出来：Activity 只留两个委托 override（守 ≤300 行），
 * "哪条链路怎么暂停/恢复"集中在这里。自动进小窗不在此处 ——
 * 它由 [buildPipParams] 的 setAutoEnterEnabled 按 autoPip 门控，系统级触发。
 *
 * 【小窗例外】PiP 期间 onStop 也会来，但小窗里要继续播（系统控件接管），
 * 所以 isInPictureInPictureMode / pipActive 任一为真就不暂停。
 */

internal fun PlayerComposeActivity.onPlayerStop() {
    if (isInPictureInPictureMode || pipActive) return
    if (playbackPrefs.backgroundAudio) return
    if (!isPlaying) return
    setPlaybackPaused(true)
    pausedByBackground = true
    smbTrace("background pause (backgroundAudio=false)")
}

internal fun PlayerComposeActivity.onPlayerResume() {
    if (!pausedByBackground) return
    pausedByBackground = false
    if (playbackPrefs.autoResume) {
        setPlaybackPaused(false)
        smbTrace("foreground auto-resume")
    }
}

/** 显式暂停/恢复（四条链路），与 togglePlayPause 同源但方向确定，供生命周期调用。 */
private fun PlayerComposeActivity.setPlaybackPaused(paused: Boolean) {
    smbSoftSession?.let { soft ->
        if (soft.setPaused(paused) == 0) {
            smbSoftAudio?.setPaused(paused)
            smbSoftAudioGated = paused
            isPlaying = !paused
        }
        return
    }
    smbNetSession?.let { net ->
        net.setPaused(paused)
        isPlaying = !paused
        return
    }
    mpvNetSession?.let { mpv ->
        Thread { mpv.setPaused(paused) }.start()
        isPlaying = !paused
        return
    }
    val session = playbackSession ?: return
    val rc = if (paused) session.pause() else session.resume()
    if (rc == 0) isPlaying = !paused
}