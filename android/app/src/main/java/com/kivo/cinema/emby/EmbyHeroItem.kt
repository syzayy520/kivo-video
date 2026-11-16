package com.kivo.cinema.emby

/**
 * 317 · **服务器首页 hero 用的条目**（Rex 式大海报 + 元信息 + 简介 + 评分）。
 *
 * ## 为什么不复用 [EmbyItem]
 * [EmbyItem] 是**海报墙卡片**的最小信息集：480 宽海报、没有简介、没有大图。
 * hero 要的是：
 * · **大图**（铺满屏宽 ⇒ 需要 1080p 量级的 backdrop/primary，480 会糊）；
 * · **简介**（2–3 行，`Overview`）；
 * · **多评分**（豆瓣/TMDB/IMDb —— 我们目前只有 Emby 的 CommunityRating）。
 *
 * 两套字段集差异大（约 60% 不重叠），硬塞进一个类会让海报墙也带上永远用不到的字段，
 * 并且 `Overview` 是长文本、`Limit` 列表里几十项会显著变大。**分开放**。
 *
 * ## 大图取谁
 * 优先 `Backdrop`（横版剧照，天生适合铺屏），没有则退回 `Primary`（竖版海报）。
 * 两者都没有 ⇒ 空串，UI 走渐变兜底（不许留黑块）。
 */
data class EmbyHeroItem(
    val id: String,
    val name: String,
    /** 铺屏大图；空串 = 两种图都没有，UI 用渐变兜底。 */
    val heroUrl: String,
    val year: Int?,
    val countries: List<String>,
    val genres: List<String>,
    /** yyyy-MM-dd，空串 = 服务器没给。 */
    val premiereDate: String,
    /** 集数（电影为 null）。来自 `ChildCount` 或 `RecursiveItemCount`。 */
    val episodeCount: Int?,
    val overview: String,
    val rating: Double?,
    val isSeries: Boolean,
    /** 继续观看：已看位置（毫秒）。0 = 没看过。 */
    val resumeMs: Long = 0L,
    /** 该条目总时长（毫秒）。0 = 未知。 */
    val runtimeMs: Long = 0L,
) {
    /** 「韩国 · 剧情 · 2025-09-24 · 12集」——Rex 的元信息行格式。 */
    fun metaLine(): String {
        val parts = mutableListOf<String>()
        countries.firstOrNull()?.let { parts += it }
        if (genres.isNotEmpty()) parts += genres.take(2).joinToString(" / ")
        premiereDate.takeIf { it.isNotBlank() }?.let { parts += it }
        episodeCount?.takeIf { it > 0 }?.let { parts += "${it}集" }
        return parts.joinToString(" · ")
    }
}
