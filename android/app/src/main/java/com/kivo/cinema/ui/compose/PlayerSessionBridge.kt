package com.kivo.cinema.ui.compose

import android.util.Log
import com.kivo.cinema.playback.MpvPlaybackSession
import com.kivo.cinema.playback.NetworkPlaybackSession
import com.kivo.cinema.ui.compose.player.PlayerPhase

/** 会话状态回显：位置轮询 + 网速采样 + mpv 属性读写 + 观看时长累计。 */

internal fun PlayerComposeActivity.smbTrace(msg: String) {
    Log.i(PlayerComposeActivity.TAG, msg)
    try {
        // 内部目录而非 external：MIUI 上 getExternalFilesDir 可能为 null，静默失败等于没日志。
        val dir = java.io.File(filesDir, "evidence").apply { mkdirs() }
        java.io.File(dir, "p220-status.log").appendText("${System.currentTimeMillis()} $msg\n")
    } catch (e: Exception) { }
}

/**
 * mpv 属性的 null-safe 读写 —— 全项目唯一入口。
 *
 * 【为什么是 nativeOrNull】轮询与 Compose 重组都可能早于会话建立，
 * 会抛异常的 native() 会让整个界面崩掉。
 * 【为什么读不到返回 null 而不是默认值】面板每一项都必须是内核真值；
 * 编一个默认值就是"面板说 1.5x、实际在 1x"这种骗人的界面。
 */
internal fun MpvPlaybackSession?.readProperty(name: String): String? {
    val native = this?.nativeOrNull() ?: return null
    return runCatching { native.property(name) }.getOrNull()
}

internal fun MpvPlaybackSession.writeProperty(name: String, value: String): Boolean {
    val native = nativeOrNull() ?: return false
    return runCatching { native.setProperty(name, value) }.getOrDefault(false)
}

/** 网速 + 缓冲比例 1s 节流：cache-speed 自身就是 1s 窗口，更快读只是抢 mpv 核心锁。 */
internal fun PlayerComposeActivity.sampleTelemetry() {
    val now = System.currentTimeMillis()
    if (now - lastSpeedSampleMs < 1_000) return
    lastSpeedSampleMs = now
    val bytesPerSec = mpvNetSession.readProperty("cache-speed")?.toLongOrNull() ?: 0L
    speedText = when {
        bytesPerSec <= 0L -> ""
        bytesPerSec >= 1_000_000L -> "%.1f MB/s".format(bytesPerSec / 1_000_000.0)
        else -> "%.0f KB/s".format(bytesPerSec / 1_000.0)
    }
    // 缓冲段 = 已播位置 + 解复用器领先量；读不到领先量就只画已播段（不编）。
    val aheadSec = mpvNetSession.readProperty("demuxer-cache-time")?.toDoubleOrNull() ?: 0.0
    bufferedFraction = if (durationMs > 0) {
        ((positionMs / 1000.0 + aheadSec) / (durationMs / 1000.0)).toFloat().coerceIn(0f, 1f)
    } else {
        0f
    }
}

internal class PlayerPositionPoll(private val a: PlayerComposeActivity) : Runnable {
    override fun run() {
        val intervalMs = pollOnce()
        android.os.Handler(a.mainLooper).postDelayed(this, intervalMs)
    }

    /** 轮询一次并把内核真值写成 [PlayerPhase]；返回下一次轮询间隔（ms）。 */
    private fun pollOnce(): Long {
        val soft = a.smbSoftSession
        if (soft != null) {
            val dur = soft.durationUs()
            if (dur > 0) a.durationMs = dur / 1000
            val pos = soft.positionUs()
            if (pos >= 0) {
                a.positionMs = (pos / 1000).let { if (a.durationMs > 0) it.coerceAtMost(a.durationMs) else it }
                val nowMs = System.currentTimeMillis()
                // played=真正在响的位置；position=解码写环位置。只打后者会把排队误当已响。
                val aPlayed = a.smbSoftAudio?.playedUs() ?: -1L
                soft.setAudioClockUs(aPlayed)
                if (a.softRateMarkMs == 0L) {
                    a.softRateMarkMs = nowMs; a.softRateMarkPosUs = pos; a.softAudioMarkUs = aPlayed
                } else if (nowMs - a.softRateMarkMs >= 2000) {
                    val dMediaUs = pos - a.softRateMarkPosUs
                    val dWallUs = (nowMs - a.softRateMarkMs) * 1000
                    val ratio = if (dWallUs > 0) dMediaUs.toDouble() / dWallUs else 0.0
                    val dAudioUs = if (aPlayed >= 0 && a.softAudioMarkUs >= 0) aPlayed - a.softAudioMarkUs else -1L
                    val aRate = if (dAudioUs > 0 && dWallUs > 0) dAudioUs.toDouble() / dWallUs else 0.0
                    val avDiffUs = if (aPlayed >= 0) aPlayed - pos else 0L
                    a.smbTrace("soft rate media=+${dMediaUs / 1000}ms wall=+${dWallUs / 1000}ms " +
                        "ratio=${"%.2f".format(java.util.Locale.US, ratio)} " +
                        "arate=${"%.2f".format(java.util.Locale.US, aRate)} " +
                        "avdiff=${avDiffUs / 1000}ms gated=${a.smbSoftAudioGated}")
                    a.softRateMarkMs = nowMs; a.softRateMarkPosUs = pos; a.softAudioMarkUs = aPlayed
                }
            }
            // 兜底闸门（非常规同步）：视频彻底解不动时音频会跑前几十秒，必须停流等。
            // 阈值 5s：800ms 是各自守时时代的凑数值，现在会打断对齐造成振荡。
            val audioSync = a.smbSoftAudio
            if (audioSync != null && a.isPlaying && pos >= 0) {
                val audioPos = audioSync.positionUs()
                if (audioPos >= 0) {
                    val leadUs = audioPos - pos
                    if (!a.smbSoftAudioGated && leadUs > 5_000_000) {
                        audioSync.setPaused(true); a.smbSoftAudioGated = true
                        a.smbTrace("soft audio gated: lead=$leadUs us")
                    } else if (a.smbSoftAudioGated && leadUs < 1_000_000) {
                        audioSync.setPaused(false); a.smbSoftAudioGated = false
                        a.smbTrace("soft audio resumed: lead=$leadUs us")
                    }
                }
            }
            if (!soft.isRunning()) {
                // 位置是否到头是客户端唯一可分辨 EOS/失败的判据。
                val ended = a.durationMs > 0 && a.positionMs >= a.durationMs - 1000
                a.phase = if (ended) PlayerPhase.Ended else PlayerPhase.Failed("软解中断：解码线程已停止")
                a.statusText = if (ended) "播放结束" else "软解中断"
                a.isPlaying = false
                a.controlsVisible = true
                return 200L
            }
            if (System.currentTimeMillis() - a.lastTouchMs > 3000) a.controlsVisible = false
            return 200L
        }
        val mpvSession = a.mpvNetSession
        if (mpvSession != null) {
            if (mpvSession.durationUs > 0) a.durationMs = mpvSession.durationUs / 1000
            a.positionMs = (mpvSession.positionUs / 1000).coerceAtLeast(0)
                .let { if (a.durationMs > 0) it.coerceAtMost(a.durationMs) else it }
            a.isPlaying = mpvSession.state == MpvPlaybackSession.State.PLAYING
            when (mpvSession.state) {
                MpvPlaybackSession.State.FAILED -> {
                    a.phase = PlayerPhase.Failed("网络播放失败：${mpvSession.lastError}")
                    a.statusText = "网络播放失败：${mpvSession.lastError}"
                    a.controlsVisible = true
                    return 100L
                }
                MpvPlaybackSession.State.ENDED -> {
                    a.phase = PlayerPhase.Ended
                    a.statusText = "播放结束"
                    a.controlsVisible = true
                    return 100L
                }
                MpvPlaybackSession.State.PREPARING, MpvPlaybackSession.State.IDLE -> {
                    a.phase = PlayerPhase.Buffering
                }
                else -> {
                    if (a.phase is PlayerPhase.Buffering || a.phase is PlayerPhase.Reconnecting) {
                        a.phase = PlayerPhase.Playing
                    }
                    if (a.statusText.startsWith("网络中断")) a.statusText = "正在播放"
                }
            }
            // 必须重新排期：提前 return 会跳过 postDelayed，位置恒 0（真机踩过）。
            // 自动隐藏：控制条常驻会盖住底部字幕（真机取证曾误判"字幕没渲染"）。
            if (System.currentTimeMillis() - a.lastTouchMs > 3000) a.controlsVisible = false
            a.sampleTelemetry()
            return 100L
        }
        val netSession = a.smbNetSession
        if (netSession != null) {
            a.durationMs = netSession.durationUs / 1000
            a.positionMs = (netSession.positionUs / 1000).coerceAtLeast(0)
                .let { if (a.durationMs > 0) it.coerceAtMost(a.durationMs) else it }
            a.isPlaying = netSession.state == NetworkPlaybackSession.State.PLAYING
            when (netSession.state) {
                NetworkPlaybackSession.State.FAILED -> {
                    a.phase = PlayerPhase.Failed("网络播放失败：${netSession.lastError}")
                    a.statusText = "网络播放失败：${netSession.lastError}"
                    a.controlsVisible = true
                    return 100L
                }
                NetworkPlaybackSession.State.ENDED -> {
                    a.phase = PlayerPhase.Ended
                    a.statusText = "播放结束"
                    a.controlsVisible = true
                    return 100L
                }
                else -> when {
                    netSession.buffering -> {
                        a.phase = PlayerPhase.Reconnecting
                        a.statusText = "网络中断，正在重连…"
                        a.controlsVisible = true
                    }
                    netSession.lastError.isNotBlank() -> {
                        a.phase = PlayerPhase.Failed(netSession.lastError)
                        a.statusText = netSession.lastError
                        a.controlsVisible = true
                    }
                    else -> {
                        if (a.phase is PlayerPhase.Reconnecting || a.phase is PlayerPhase.Buffering) {
                            a.phase = PlayerPhase.Playing
                        }
                        if (a.statusText.startsWith("网络中断")) {
                            a.statusText = if (netSession.hasAudio) "正在播放" else "正在播放 · 片源没有音轨"
                        }
                        if (System.currentTimeMillis() - a.lastTouchMs > 3000) a.controlsVisible = false
                    }
                }
            }
            return 100L
        }
        val session = a.playbackSession ?: return 100L
        // EOS 后时钟外推会越过片长（真机 205s/60s 片）——clamp。
        a.positionMs = (session.positionUs() / 1000).coerceAtMost(a.durationMs)
        if (com.kivo.cinema.BuildConfig.DEBUG) {
            val now = System.currentTimeMillis()
            if (now - a.lastSummaryLogMs >= 1000) {
                a.lastSummaryLogMs = now
                Log.i(PlayerComposeActivity.TAG, "summary: ${runCatching { session.summary() }.getOrElse { "n/a" }}")
            }
        }
        if (a.previewMs < 0 && System.currentTimeMillis() - a.lastTouchMs > 3000) a.controlsVisible = false
        // 本地软解会话没有公开的 state 枚举，只能用两个真值判据：
        // 播完（isFinished）与明确报错（lastError 非空且已停）。
        when {
            session.isFinished() -> a.phase = PlayerPhase.Ended
            !session.isRunning() && session.lastError.isNotBlank() ->
                a.phase = PlayerPhase.Failed("播放失败：${session.lastError}")
            a.phase is PlayerPhase.Buffering || a.phase is PlayerPhase.Reconnecting ->
                a.phase = PlayerPhase.Playing
        }
        return 100L
    }
}

internal class PlayerWatchPoll(private val a: PlayerComposeActivity) : Runnable {
    override fun run() {
        if (a.isPlaying && a.durationMs > 0) {
            val delta = a.positionMs - a.watchLastPosMs
            if (delta in 1..1500) a.watchAccumMs += delta   // seek 大跳不计观看时长
        }
        a.watchLastPosMs = a.positionMs
        if (a.watchAccumMs >= 10_000) {
            a.watchStore.report(a.watchKey(), a.positionMs, a.durationMs, a.watchAccumMs)
            a.watchAccumMs = 0
        }
        android.os.Handler(a.mainLooper).postDelayed(this, 1000)
    }
}

/**
 * 观看记录的键。
 *
 * 【URL 优先于本地路径】URL 片源（Emby）的观看记录必须挂在**它的 URL** 上：
 * 详情页也要用同一个键查续播位（273 v18 的恢复点预取）⇒ 键必须一致。
 * 旧写法（路径优先）会被设备上的本地测试片源顶掉 ⇒ URL 片源存的是本地路径的键，
 * 详情页按 URL 查不到 ⇒ 两边对不上。
 *
 * 【本地片源不受影响】`kivo.playurl` 为空 ⇒ 键仍是本地路径。
 */
internal fun PlayerComposeActivity.watchKey(): String =
    playUrlOverride.ifBlank { currentMediaPath() }

internal fun PlayerComposeActivity.flushWatch() {
    if (watchLastPosMs > 0 && durationMs > 0) {
        watchStore.report(watchKey(), positionMs, durationMs, watchAccumMs)
        watchAccumMs = 0
    }
}