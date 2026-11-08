package com.kivo.cinema.library

/**
 * 标题匹配（240 §9.2）。
 *
 * 唯一职责：把「本地文件名」与「TMDB 标题」归一化后判定是否同一部片。
 * 纯 Kotlin，**不 import Android**，可单测。
 *
 * 口径（240 §9.1）：宁可空态，不给错资源 —— 要求归一化后的标题完整包含于文件名。
 * 已知局限：子串匹配无法区分续集（「生化危机」会命中「生化危机2」），
 * 故归一化时保留数字，续集标题通常自带数字，可在多数情况下区分。
 */
object TitleMatch {

    /** 发布规格噪音词：出现在文件名里但不属于片名。 */
    private val NOISE = listOf(
        "2160p", "1080p", "720p", "480p", "4k", "8k", "uhd",
        "hdr10", "hdr", "dolbyvision", "dolby", "dv", "atmos",
        "webrip", "web-dl", "webdl", "web", "bluray", "bdrip", "brrip", "remux",
        "hdtv", "dvdrip", "hdrip", "proper", "repack", "extended", "uncut",
        "x264", "x265", "h264", "h265", "hevc", "avc", "10bit", "8bit",
        "aac", "ac3", "dts", "truehd", "flac", "remastered",
    )

    /** 归一化：小写 → 去扩展名 → 去噪音词 → 只留文字与数字。 */
    fun normalize(raw: String): String {
        var s = raw.lowercase()
        val dot = s.lastIndexOf('.')
        if (dot > 0 && s.length - dot <= 5) s = s.substring(0, dot)
        for (n in NOISE) s = s.replace(n, "")
        val sb = StringBuilder(s.length)
        for (ch in s) if (ch.isLetterOrDigit()) sb.append(ch)
        return sb.toString()
    }

    /** 文件名是否指向该标题。 */
    fun matches(fileName: String, title: String): Boolean {
        val t = normalize(title)
        if (t.isEmpty()) return false
        return normalize(fileName).contains(t)
    }
}
