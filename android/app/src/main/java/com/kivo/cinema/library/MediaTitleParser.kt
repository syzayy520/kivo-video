package com.kivo.cinema.library

/**
 * 从 NAS 文件名解析展示用片名。
 *
 * 单一职责：把一个媒体文件名洗成"像电影的标题"，只做字符串处理，不碰 IO/网络。
 * 不刮削（那要 TMDB）——只去掉扩展名、片源标签与分辨率，按年份截断；
 * 拿不准就退回原文件名，宁可朴素也不猜错。
 *
 * 例：`A Beautiful Mind 2001 BluRay REMUX.mkv` → `A Beautiful Mind` + 2001
 */
object MediaTitleParser {

    data class Parsed(val title: String, val year: Int?)

    /** 常见片源/编码/音轨标签，命中即从标题里剔除。 */
    private val TAGS = Regex(
        """(?i)\b(blu-?ray|remux|web-?dl|webrip|hdtv|dvdrip|bdrip|brrip|hdr10\+?|dolby.?vision|\bdv\b|""".trimIndent() +
            """h\.?264|h\.?265|hevc|x\.?264|x\.?265|avc|aac|ac3|eac3|dts(-hd)?|truehd|atmos|""".trimIndent() +
            """10\s?bit|8\s?bit|4k|2160p|1080p|720p|480p|repack|proper|extended|unrated|remastered)\b""",
    )

    private val YEAR = Regex("""\b(19\d{2}|20\d{2})\b""")
    private val SPACES = Regex("""\s+""")

    fun parse(fileName: String): Parsed {
        val noExt = fileName.substringBeforeLast('.', fileName)
        val flat = noExt.replace('.', ' ').replace('_', ' ').replace('-', ' ')

        val yearMatch = YEAR.find(flat)
        val year = yearMatch?.value?.toIntOrNull()
        // 年份之后基本都是标签区，直接截掉，避免把 "2024 1080p WEB-DL" 全留在标题里。
        val head = if (yearMatch != null) flat.substring(0, yearMatch.range.first) else flat
        val stripped = TAGS.replace(head, " ")
        val cleaned = SPACES.replace(stripped, " ").trim()

        // 清洗后可能空掉（例如文件名本身就只有标签）⇒ 退回原始（去扩展名）名字。
        val title = if (cleaned.isBlank()) noExt.trim() else cleaned
        return Parsed(title, year)
    }
}
