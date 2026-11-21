package com.kivo.cinema.ui.compose

/**
 * P9 · I：**不支持字幕的播放模式**给一次明确提示。
 *
 * 软解（ffmpeg 兜底）、平台硬解（NetworkPlaybackSession）与 legacy 本地链路
 * 都没有字幕渲染管线 —— 字幕面板照画但永远选不中任何轨，用户会以为"字幕坏了"。
 * 这里只做一件事：每次进入播放器提示**一次**，状态栏文案由调用方同步注明"无字幕"。
 * 不重复弹：同一会话里换了播放模式也不该再打断观看。
 */
internal fun PlayerComposeActivity.hintNoSubtitleSupport(mode: String) {
    if (subtitleHintShown) return
    subtitleHintShown = true
    smbTrace("subtitle unsupported: mode=$mode")
    showToast("当前播放模式（$mode）不支持字幕", durationMs = 2_600)
}
