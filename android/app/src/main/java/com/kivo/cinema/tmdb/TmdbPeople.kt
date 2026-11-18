package com.kivo.cinema.tmdb

/**
 * 演员（B3 冻结模型，消费方逐字使用）。
 * `profileUrl` 可为 null = 该演员在 TMDB 无头像，由 UI 占位，不是"加载失败"。
 * `knownFor` 是展示文案（Acting → 「演员」），不是原始 `known_for_department`。
 */
data class TmdbPerson(val id: Int, val name: String, val profileUrl: String?, val knownFor: String)

/**
 * 人气演员拉取器（首页「热门演员」横排的数据源）。
 *
 * 端点：`GET /person/popular?language=${TmdbLocale.tag()}&page=1`（✅ 实测 2026-11-17 返回 20 条）。
 * 取图：`profile_path` → `w185`（横排头像卡宽度 ≤ 92dp，w185 在 xxhdpi 上刚好够，不浪费流量）。
 * 失败语义：无 key / 无网 / 非 2xx / 返回体不可解析 → **空列表**，不补假数据、不降级。
 *
 * 设计取舍：
 * - 复用 [TmdbHome.get]（已含 8s 连接 / 15s 读取超时、12h 列表缓存、五类错误分类），
 *   本类只做「JSONObject → 模型」的映射，不重复写鉴权与出网逻辑。
 * - `knownFor` 只把 `known_for_department == "Acting"` 归一为「演员」；其余部门（Directing /
 *   Writing 等）保留接口原值（◐ 推断：中文名需人工维护映射表，本轮不引入，未覆盖 Directing=导演）。
 * - 没有 `profile_path` 的演员**仍然返回**（profileUrl=null），由 UI 决定占位；
 *   不按「有图」过滤，避免热门榜单被图像可用性扭曲。
 *
 * 线程：同步阻塞，须在调用方 IO 线程执行。
 */
object TmdbPeople {

    private const val ACTING = "Acting"
    private const val ACTING_ZH = "演员"

    fun popular(client: TmdbClient, limit: Int = 12): List<TmdbPerson> {
        if (limit <= 0) return emptyList()
        val results = TmdbHome(client).get("/person/popular?language=${TmdbLocale.tag()}&page=1").first
            ?.optJSONArray("results") ?: return emptyList()
        val out = ArrayList<TmdbPerson>(minOf(limit, results.length()))
        for (i in 0 until results.length()) {
            if (out.size >= limit) break
            val o = results.optJSONObject(i) ?: continue
            val id = o.optInt("id")
            val name = o.optString("name")
            if (id <= 0 || name.isBlank()) continue
            val department = o.optString("known_for_department")
            out.add(
                TmdbPerson(
                    id = id,
                    name = name,
                    profileUrl = TmdbHome.imgPath(o, "profile_path")
                        ?.let { "${TmdbHome.IMG}/w185$it" },
                    knownFor = if (department == ACTING) ACTING_ZH else department,
                )
            )
        }
        return out
    }
}