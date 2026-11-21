package com.kivo.cinema.playback.subtitle

import android.content.Intent

/**
 * 片源的**稳定键**（P9 · C 的延迟记忆用）。
 *
 * 与 `PlayerSessionBridge.watchKey()` 同一套口径：
 * 1. URL 片源 → `url:<playurl>`（Emby 的临时代理 URL 就是它自己的身份）；
 * 2. SMB → `smb://<源id>/<共享>/<路径>`（三元组缺一不可，否则所有 NAS 片共用一个键）；
 * 3. 本地 → `file:<绝对路径>`。
 *
 * 【为什么不用"当前代理 URL"或播放会话 id】它们是每次起播新生成的临时身份，
 * 用它当键 ⇒ 延迟永远读不回、且会串到别的片源上。
 */
internal object SubtitleSourceKey {

    fun fromIntent(intent: Intent?): String? {
        if (intent == null) return null
        intent.getStringExtra("kivo.playurl")?.takeIf { it.isNotBlank() }?.let { return "url:$it" }
        val sourceId = intent.getStringExtra("kivo.sourceId")
        val path = intent.getStringExtra("kivo.smbPath")
        if (!sourceId.isNullOrBlank() && !path.isNullOrBlank()) {
            val share = intent.getStringExtra("kivo.smbShare").orEmpty()
            return "smb://$sourceId/$share/$path"
        }
        intent.getStringExtra("kivo.media")?.takeIf { it.isNotBlank() }?.let { return "file:$it" }
        return null
    }
}
