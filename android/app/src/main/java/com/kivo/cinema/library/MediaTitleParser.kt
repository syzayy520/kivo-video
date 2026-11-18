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

    data class Parsed(
        val title: String,
        val year: Int?,
        val english: String? = null,
        /** 季集标记（如 `S01E01`），只用于 UI 区分同剧不同集；搜索时已被剥掉。 */
        val episode: String? = null,
    )

    /**
     * 常见片源/编码/音轨/平台标签，命中即从标题里剔除。
     *
     * 【为什么平台名也要剔】NAS 上大量文件是 `片名.AMZN.DDP.5.1.H.264.CHDWEB.mkv` 这种命名，
     * 只剔编码不剔平台名，卡片标题就会拖一条尾巴（真机：`Cold Case Gleen AMZN DDP 2 0 H 264 CHDWEB`）。
     * 这些词在真实片名里出现概率极低，宁可清掉。
     */
    private val TAGS = Regex(
        """(?i)\b(blu-?ray|remux|web[ .-]?dl|webrip|hdtv|dvdrip|bdrip|brrip|hdr10\+?|dolby.?vision|\bdv\b|""" +
            """h[ .]?26[45]|hevc|x[ .]?26[45]|avc|aac|ac3|eac3|dts(-hd)?|truehd|atmos|ddp|dd\+|""" +
            """10\s?bit|8\s?bit|4k|2160p|1080p|720p|480p|repack|proper|extended|unrated|remastered|""" +
            """amzn|hulu|dsnp|hmax|atvp|pcok|itunes|chdweb|chd|[25][ .][01])\b""",
    )

    private val YEAR = Regex("""\b(19\d{2}|20\d{2})\b""")
    private val SPACES = Regex("""\s+""")

    /** 季集标记（S01E01 / E05 / Season 2 / 1x03）：剧集搜索必须剥掉，否则 TMDB 一定搜不到。 */
    private val EPISODE = Regex(
        """(?i)\b(s\d{1,2}(?:[ ._-]?e\d{1,3})?|e\d{1,3}|season[ ._-]?\d{1,2}|episode[ ._-]?\d{1,3}|\d{1,2}x\d{1,3})\b""",
    )

    /**
     * 中文集号：`第01集` / `第12话` / `第3期` / `第十二集`（阿拉伯数字即可，汉字数字太容易误判）。
     *
     * 【为什么单列】[EPISODE] 的 `\b` 与 latin 集号前缀在纯中文文件名上匹配不到，
     * 而 NAS 中文剧集绝大多数就是 `剧名.第01集.mkv` —— 认不出集号就聚不成"一部剧"。
     */
    private val CN_EPISODE = Regex("""第\s*(\d{1,4})\s*[集话期回]""")

    /** 形如 `[犯罪都市4]` 的中文方括号前缀（括号内必须含中文，否则当发布组标签忽略）。 */
    private val CN_BRACKET = Regex("""^\[\s*([^\]]*[\u4e00-\u9fa5][^\]]*)\s*]""")

    /**
     * 中文集号的数字部分（`某剧.第12集.mkv` → 12）；认不出返回 null。
     *
     * 供片库聚合排序用：同一部剧的集按这个数字升序，而不是按文件名字典序
     * （字典序会把 `第10集` 排到 `第2集` 前面）。
     */
    fun cnEpisodeIndex(raw: String): Int? =
        CN_EPISODE.find(raw)?.groupValues?.getOrNull(1)?.toIntOrNull()?.takeIf { it > 0 }

    /** 提取季集标记并标准化：`S01E01` / `S1E1` → `S01E01` 风格；`Season 2` → `S02`；拿不到返回 null。 */
    private fun episodeOf(raw: String): String? {
        // 中文集号优先：`某剧.第01集.mkv` ⇒ `E01`（聚合按数字排序，格式统一）。
        cnEpisodeIndex(raw)?.let { return "E%02d".format(it) }
        val rawHit = EPISODE.find(raw.replace('.', ' ').replace('_', ' ').replace('-', ' '))
            ?.value ?: return null
        val hit = rawHit.uppercase().replace(" ", "").replace("_", "").replace("-", "")
        val se = Regex("""^S(\d{1,2})E(\d{1,3})$""").find(hit)
        if (se != null) {
            return "S%02dE%02d".format(se.groupValues[1].toInt(), se.groupValues[2].toInt())
        }
        val season = Regex("""^SEASON(\d{1,2})$""").find(hit)
        if (season != null) return "S%02d".format(season.groupValues[1].toInt())
        return hit
    }

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
                return Parsed(cn, year, en.ifBlank { null }, episodeOf(noExt))
            }
        }

        val flat = noExt.replace('.', ' ').replace('_', ' ').replace('-', ' ')
        // 年份之后基本都是标签区，直接截掉，避免把 "2024 1080p WEB-DL" 全留在标题里。
        val cut = YEAR.find(flat)?.range?.first ?: flat.length
        val head = flat.substring(0, cut)
        // 中文集号与英文集号都要剥：`某剧 第01集` 和 `某剧 S01E01` 是同一种东西。
        val stripped = CN_EPISODE.replace(EPISODE.replace(TAGS.replace(head, " "), " "), " ")
        val cleaned = SPACES.replace(stripped, " ").trim()

        // 清洗后可能空掉（例如文件名本身就只有标签）⇒ 退回原始（去扩展名）名字。
        val title = if (cleaned.isBlank()) noExt.trim() else cleaned
        return Parsed(title, year, episode = episodeOf(noExt))
    }
}
