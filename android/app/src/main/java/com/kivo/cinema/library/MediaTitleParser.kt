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

    data class Parsed(val title: String, val year: Int?, val english: String? = null)

    /** 常见片源/编码/音轨标签，命中即从标题里剔除。 */
    private val TAGS = Regex(
        """(?i)\b(blu-?ray|remux|web-?dl|webrip|hdtv|dvdrip|bdrip|brrip|hdr10\+?|dolby.?vision|\bdv\b|""".trimIndent() +
            """h\.?264|h\.?265|hevc|x\.?264|x\.?265|avc|aac|ac3|eac3|dts(-hd)?|truehd|atmos|""".trimIndent() +
            """10\s?bit|8\s?bit|4k|2160p|1080p|720p|480p|repack|proper|extended|unrated|remastered)\b""",
    )

    private val YEAR = Regex("""\b(19\d{2}|20\d{2})\b""")
    private val SPACES = Regex("""\s+""")

    /** 季集标记（S01E01 / E05 / Season 2 / 1x03）：剧集搜索必须剥掉，否则 TMDB 一定搜不到。 */
    private val EPISODE = Regex(
        """(?i)(s\d{1,2}(?:[ ._-]?e\d{1,3})?|e\d{1,3}|season[ ._-]?\d{1,2}|episode[ ._-]?\d{1,3}|\d{1,2}x\d{1,3})""",
    )

    /** 形如 `[犯罪都市4]` 的中文方括号前缀（括号内必须含中文，否则当发布组标签忽略）。 */
    private val CN_BRACKET = Regex("""^\[\s*([^\]]*[\u4e00-\u9fa5][^\]]*)\s*]""")

    fun parse(fileName: String): Parsed {
        val noExt = fileName.substringBeforeLast('.', fileName)
        val year = YEAR.find(noExt)?.value?.toIntOrNull()

        // 中文名优先：`[犯罪都市4].The.Roundup…` ⇒ `犯罪都市4`。
        // 中文用户记的是中文名，把英文发行名一起摆出来只会把卡片标题撑到截断。
        CN_BRACKET.find(noExt)?.let { m ->
            val cn = m.groupValues[1].trim()
            if (cn.isNotBlank()) {
                // 括号后到年份前是英文发行名：TMDB 中文搜不到时用它兜底（剧集尤其常见）。
                val after = noExt.substring(m.range.last + 1)
                    .replace('.', ' ').replace('_', ' ').replace('-', ' ')
                val cut = YEAR.find(after)?.range?.first ?: after.length
                val en = SPACES.replace(TAGS.replace(after.substring(0, cut), " "), " ").trim()
                return Parsed(cn, year, en.ifBlank { null })
            }
        }

        val flat = noExt.replace('.', ' ').replace('_', ' ').replace('-', ' ')
        // 年份之后基本都是标签区，直接截掉，避免把 "2024 1080p WEB-DL" 全留在标题里。
        val cut = YEAR.find(flat)?.range?.first ?: flat.length
        val head = flat.substring(0, cut)
        val stripped = EPISODE.replace(TAGS.replace(head, " "), " ")
        val cleaned = SPACES.replace(stripped, " ").trim()

        // 清洗后可能空掉（例如文件名本身就只有标签）⇒ 退回原始（去扩展名）名字。
        val title = if (cleaned.isBlank()) noExt.trim() else cleaned
        return Parsed(title, year)
    }
}
