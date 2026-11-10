package com.kivo.cinema.playback

import android.os.SystemClock

/** Numeric-only diagnostics: never attach a URL or an exception from the media source. */
internal object NetworkSteadyProgress {
    class Failure(val diagnostic: String) : IllegalStateException(diagnostic)

    fun verify(active: NetworkPlaybackSession, durationMs: Long): String {
        val samples = mutableListOf(SystemClock.elapsedRealtime() to active.positionUs)
        fun curve() = samples.joinToString(",") { "${it.first - samples.first().first}:${it.second}" }

        // 失败时必须能区分两种完全不同的原因：
        //   ① 播放真的停了 200 ms（音频会补静音 / xrun 增加 / lateness 爆表）；
        //   ② 采样器自己晚了（sleep(1000) 实际睡了 1200 ms，位置其实连续）。
        // 只给 rate 一个数字，这两种情况长得一模一样 —— 上一轮就是这样，
        // 差 0.004 被判失败，但音频侧全程零静音零 xrun，无法归因。
        // `tail` 给出最近四个采样点的墙钟与位置，`summary` 给出当时的节拍与音频账。
        fun fail(gate: String, rate: Double? = null): Nothing {
            val tail = samples.takeLast(4).joinToString(",") {
                "${it.first - samples.first().first}:${it.second}"
            }
            val summary = runCatching { active.lastSummary }.getOrDefault("unavailable")
            throw Failure("gate=$gate rate=${rate ?: "unavailable"} tail=$tail " +
                "summary=$summary samples_ms_us=${curve()}")
        }
        while (SystemClock.elapsedRealtime() - samples.first().first < durationMs) {
            SystemClock.sleep(1000)
            samples += SystemClock.elapsedRealtime() to active.positionUs
            if (active.state != NetworkPlaybackSession.State.PLAYING) fail("state-${active.state}")
            if (samples.size >= 3) {
                val earlier = samples[samples.lastIndex - 2]
                val latest = samples.last()
                val rate = (latest.second - earlier.second).toDouble() /
                    ((latest.first - earlier.first) * 1000)
                if (rate !in 0.90..1.10) fail("rolling-2s", rate)
            }
        }
        val rate = (samples.last().second - samples.first().second).toDouble() /
            ((samples.last().first - samples.first().first) * 1000)
        if (rate !in 0.98..1.02) fail("whole-window", rate)
        return "PASS steady wall_ms=${samples.last().first - samples.first().first} rate=$rate " +
            "samples_ms_us=${curve()}"
    }
}
