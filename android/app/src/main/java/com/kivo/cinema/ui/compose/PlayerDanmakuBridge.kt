package com.kivo.cinema.ui.compose

import android.util.Log

/**
 * 弹幕侧载（223）：起播时按片名找同目录 XML。
 *
 * 【为什么找不到也要写日志】"弹幕没出来"有两种病因：文件不在，或解析失败。
 * 两者都不写日志时，界面上看起来一模一样，排障只能靠猜。
 */
internal fun PlayerComposeActivity.loadDanmaku(mediaPath: String) {
    // 换片先清旧弹幕：不清的话上一部的弹幕会叠在新片上（hasItems 也要跟着归零）。
    danmakuItems = emptyList()
    danmakuOn = false
    danmakuSettings = danmakuSettings.copy(hasItems = false)
    try {
        val baseName = mediaPath.substringAfterLast('/').substringBeforeLast('.')
        val localFile = java.io.File(getExternalFilesDir(null), "danmaku/$baseName.xml")
        smbTrace("danmaku try load: ${localFile.absolutePath} exists=${localFile.exists()}")
        if (!localFile.exists()) {
            smbTrace("danmaku XML not found: ${localFile.absolutePath}")
            return
        }
        val parsed = com.kivo.cinema.danmaku.DanmakuParser.parse(localFile.bufferedReader())
        danmakuItems = parsed
        danmakuSettings = danmakuSettings.copy(hasItems = parsed.isNotEmpty())
        danmakuOn = parsed.isNotEmpty()
        smbTrace("danmaku loaded ${parsed.size} items, danmakuOn=$danmakuOn")
    } catch (e: Exception) {
        smbTrace("danmaku load error: ${e.message}")
        Log.w(PlayerComposeActivity.TAG, "danmaku load failed", e)
    }
}