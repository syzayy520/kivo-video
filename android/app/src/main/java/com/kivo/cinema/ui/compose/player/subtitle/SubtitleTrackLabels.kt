package com.kivo.cinema.ui.compose.player.subtitle

import com.kivo.cinema.mpv.MpvTrackCatalog

/**
 * 轨道行的显示文案（P9 · G）。纯函数，无内核、无 UI。
 *
 * 【为什么要友好语言名】mpv 回的是 ISO 639-2（`chi` / `eng` / `jpn`），
 * 直接显示像技术日志；用户要看到「中文 / 英语 / 日语」。
 * 覆盖常用语言，未收录的保留原码（**不猜**）。
 */
internal object SubtitleTrackLabels {

    private val LANGUAGES = mapOf(
        "zh" to "中文", "chi" to "中文", "zho" to "中文",
        "en" to "英语", "eng" to "英语",
        "ja" to "日语", "jpn" to "日语",
        "ko" to "韩语", "kor" to "韩语",
        "fr" to "法语", "fre" to "法语", "fra" to "法语",
        "de" to "德语", "ger" to "德语", "deu" to "德语",
        "es" to "西班牙语", "spa" to "西班牙语",
        "ru" to "俄语", "rus" to "俄语",
        "ar" to "阿拉伯语", "ara" to "阿拉伯语",
        "pt" to "葡萄牙语", "por" to "葡萄牙语",
        "it" to "意大利语", "ita" to "意大利语",
        "th" to "泰语", "tha" to "泰语",
        "vi" to "越南语", "vie" to "越南语",
    )

    fun language(lang: String): String =
        LANGUAGES[lang.trim().lowercase()] ?: lang.trim()

    /** 行标题：标题 → 友好语言 → `字幕 #id`。 */
    fun title(track: MpvTrackCatalog.Track): String {
        if (track.title.isNotBlank()) return track.title
        val lang = language(track.lang)
        return lang.ifBlank { "字幕 #${track.id}" }
    }

    /** 行副题：语言 · 编码 · 强制 · 听障 · 外挂（有哪个写哪个）。 */
    fun subtitle(track: MpvTrackCatalog.Track, includeExternal: Boolean): String {
        val parts = ArrayList<String>(4)
        language(track.lang).takeIf { it.isNotBlank() }?.let { parts += it }
        track.codec.takeIf { it.isNotBlank() }?.let { parts += it }
        if (track.forced) parts += "强制"
        if (track.hearingImpaired) parts += "听障"
        if (includeExternal && track.external) parts += "外挂"
        return parts.joinToString(" · ")
    }
}
