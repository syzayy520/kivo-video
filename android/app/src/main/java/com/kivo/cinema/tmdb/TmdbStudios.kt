package com.kivo.cinema.tmdb

/**
 * 制片厂（B3 冻结模型 + B3R2 增字段）。
 *
 * - `logoUrl` 可为 null = 该 id 查询失败或无 logo，名称仍有效；
 * - `backdropUrl` 可为 null = 该公司**热门影片里没有可取剧照**（或请求失败）。
 *   默认 `null` 是为了不破坏既有 `TmdbStudio(id, name, logoUrl)` 构造点 ✅ 签名兼容。
 */
data class TmdbStudio(
    val id: Int,
    val name: String,
    val logoUrl: String?,
    val backdropUrl: String? = null,
)

/**
 * 「热门制片厂」横排（首页底部品牌行）。
 *
 * 为什么用静态候选 + 逐个补图：TMDB 没有「制片厂排行榜」公开端点（◐ 推断，官方
 * `/company/{id}` 只能按 id 查），所以候选 id 与兜底名硬编码在 [CANDIDATES]，
 * 名称/logo/剧照仍来自真实接口 —— 都不是编的。
 *
 * 两次请求口径（每家公司）：
 * 1. `GET /company/{id}` → `name` + `logo_path`（w300，公司 logo 横竖混合，取标准档）；
 * 2. `GET /discover/movie?with_companies={id}&sort_by=popularity.desc&language=${TmdbLocale.tag()}`
 *    → 取**首条有 `backdrop_path`** 的结果 → `w780`，供卡片做剧照底。
 *    为什么要「首条有图」而不是「字面首条」：`results[0]` 常缺 `backdrop_path`（TMDB 允许为空），
 *    取字面首条会得到 null，与「该公司没有剧照」不可区分 ◐ 推断（与 `TmdbGenres.backdropFor` 同口径）。
 *
 * 失败语义（逐条独立，不互相拖累）：无 key / 无网 / 404 → 该条 `logoUrl`/`backdropUrl`
 * 为 null，名称回落到候选名，其余条目照常返回；全部失败时仍是 6 条（名称 + 无图），
 * 由 UI 决定是否整段隐藏。**绝不拿别的公司图顶替**。
 *
 * 成本：冷启动 6×2 = 12 次请求；两次都命中 `TmdbHome.get` 的 12h 列表缓存（✅ 缓存键为完整 URL），
 * 二次进入 0 请求。须在调用方 IO 线程执行（同步阻塞）。
 */
object TmdbStudios {

    /** 候选 id → 兜底显示名（TMDB 官方命名，非营销名）。 */
    private val CANDIDATES = listOf(
        33 to "Universal Pictures",
        4 to "Paramount Pictures",
        2 to "Walt Disney Pictures",
        174 to "Warner Bros. Pictures",
        41077 to "A24",
        5 to "Columbia Pictures",
    )

    fun featured(client: TmdbClient, limit: Int = 6): List<TmdbStudio> {
        if (limit <= 0) return emptyList()
        val home = TmdbHome(client)
        return CANDIDATES.take(limit).map { (id, fallbackName) ->
            val json = home.get("/company/$id").first
            TmdbStudio(
                id = id,
                name = json?.optString("name")?.takeIf { it.isNotBlank() } ?: fallbackName,
                logoUrl = TmdbHome.imgPath(json, "logo_path")
                    ?.let { "${TmdbHome.IMG}/w300$it" },
                backdropUrl = backdropFor(home, id),
            )
        }
    }

    /** 该公司人气首位影片的剧照（w780）；取不到 → null（不造假）。 */
    private fun backdropFor(home: TmdbHome, companyId: Int): String? {
        val arr = home.get(
            "/discover/movie?with_companies=$companyId&sort_by=popularity.desc&language=${TmdbLocale.tag()}"
        ).first?.optJSONArray("results") ?: return null
        for (i in 0 until arr.length()) {
            val path = TmdbHome.imgPath(arr.optJSONObject(i), "backdrop_path") ?: continue
            return "${TmdbHome.IMG}/w780$path"
        }
        return null
    }
}