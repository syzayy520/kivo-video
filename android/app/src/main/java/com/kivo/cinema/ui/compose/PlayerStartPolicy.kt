package com.kivo.cinema.ui.compose

import com.kivo.cinema.ui.compose.player.resolutionTier

/**
 * 起播策略（262）：续播位 + 起播规格展示，设置页编辑、起播就绪后消费。
 *
 * 从起播链路分出来：起播文件只管"把会话拉起来"，
 * "拉到哪一秒、起播闪什么规格"是独立的一件事，且只读 [PlaybackPrefs]。
 */

/**
 * 续播：非"每次从头播放"时，落到上次观看位。
 * 键与 [PlayerWatchPoll] 上报用的 [watchKey] 同一个 ⇒ 存哪取哪，天然一致；
 * 接近片尾（≥95%）由 [com.kivo.cinema.stats.WatchStore.progressOf] 判 null，从头播。
 */
internal fun PlayerComposeActivity.applyResumePosition() {
    if (playbackPrefs.alwaysFromStart) return
    val progress = watchStore.progressOf(watchKey()) ?: return
    val pos = progress.positionMs
    if (pos <= 0L) return
    if (durationMs > 0 && pos >= durationMs) return
    seekTo(pos)
    positionMs = pos
    smbTrace("resume from ${pos}ms")
}

/**
 * 起播规格：按设置停留 N 秒，内容全部取内核真值（video-params / video-codec / video-bitrate）。
 * 读不到的项不拼进去 —— 不许编一个"1080P"骗用户；一项都读不到就不显示。
 */
internal fun PlayerComposeActivity.showStartupSpec() {
    val seconds = playbackPrefs.startupSpecSeconds
    if (seconds <= 0) return
    val native = mpvNetSession?.nativeOrNull() ?: return
    val w = runCatching { native.property("video-params/w")?.toIntOrNull() }.getOrNull() ?: 0
    val h = runCatching { native.property("video-params/h")?.toIntOrNull() }.getOrNull() ?: 0
    val tier = if (w > 0 && h > 0) resolutionTier(w, h) else ""
    val codec = runCatching { native.property("video-codec") }.getOrNull()
        ?.substringBefore(' ')?.uppercase()?.takeIf { it.isNotBlank() }
    val bitrate = runCatching { native.property("video-bitrate")?.toLongOrNull() }.getOrNull()
    val parts = buildList {
        if (tier.isNotBlank()) add(tier)
        if (codec != null) add(codec)
        if (bitrate != null && bitrate > 0) add("%.1f Mbps".format(java.util.Locale.US, bitrate / 1_000_000.0))
    }
    if (parts.isEmpty()) return
    showToast(parts.joinToString(" · "), seconds * 1000L)
}