package com.kivo.cinema.tmdb

/**
 * 流媒体平台（B3 冻结模型）。`logoUrl` 可为 null = 该平台在 TMDB 无 logo 库存图。
 *
 * [titles] 是平台卡右侧拼贴用的海报地址（w185）。**默认空列表**：拉取成本高，
 * 只有调用方显式用 [TmdbProviders.titles] 填充过的平台才有值（首页只填前 4 个）。
 * 空 = 「未填充或该平台无可取影片」，UI 必须按空处理、不得用别的平台海报顶替（不造假）。
 */
data class TmdbProvider(
    val id: Int,
    val name: String,
    val logoUrl: String?,
    val titles: List<String> = emptyList(),
)

/**
 * 流媒体平台拉取器（首页「观看平台」横排）。
 *
 * 端点：`GET /watch/providers/movie?watch_region={region}&language=zh-CN`。
 * 取图：`logo_path` → `w154`（平台 logo 在卡内约 40dp 高，w154 已足够且体积小）。
 * 排序：按该 region 的 `display_priority` 升序（✅ 实测 HK 段为 Netflix 1 / Amazon 2 / Apple 2）。
 *
 * ⚠️ 已实测的事实（务必让消费方知道）：**`watch_region=CN` 返回 0 条** ——
 * TMDB 在中国大陆没有 JustWatch 平台数据（✅ 实测 CN=0 / HK=42 / TW=44 / JP=70 / US=296）。
 * 所以默认参数下 [movie] 恒为空列表。这是接口真实结果，不造假、不拿别的地区数据冒充中国，
 * UI 必须按「空 → 整段不渲染」处理；要展示平台行只能由调用方显式传 `region = "HK"` 等。
 *
 * [titles]：`GET /discover/movie?with_watch_providers={id}&watch_region={region}`
 * `&sort_by=popularity.desc&language=zh-CN`，按人气取前 `limit` 条 `poster_path` → w185。
 * **刻意不放进 [movie]**：8 个平台各拉一次 discover 会让首页首屏多 8 次串行请求
 * （实测单次 100–400ms 量级，◐ 推断），由调用方只对展示前几名调
 * [titles] 后 `copy(titles = …)` 填充，请求数可控。
 *
 * 失败语义：无 key / 无网 / 非 2xx → 空列表（[movie] 与 [titles] 同一口径）。
 * 线程：同步阻塞，须在调用方 IO 线程执行。
 */
object TmdbProviders {

    private data class Ranked(val priority: Int, val provider: TmdbProvider)

    fun movie(client: TmdbClient, region: String = "CN", limit: Int = 8): List<TmdbProvider> {
        if (limit <= 0) return emptyList()
        val results = TmdbHome(client)
            .get("/watch/providers/movie?watch_region=$region&language=zh-CN").first
            ?.optJSONArray("results") ?: return emptyList()
        val ranked = ArrayList<Ranked>(results.length())
        for (i in 0 until results.length()) {
            val o = results.optJSONObject(i) ?: continue
            val id = o.optInt("provider_id")
            val name = o.optString("provider_name")
            if (id <= 0 || name.isBlank()) continue
            ranked.add(
                Ranked(
                    priority = o.optInt("display_priority", Int.MAX_VALUE),
                    provider = TmdbProvider(
                        id = id,
                        name = name,
                        logoUrl = TmdbHome.imgPath(o, "logo_path")
                            ?.let { "${TmdbHome.IMG}/w154$it" },
                    ),
                )
            )
        }
        return ranked.sortedBy { it.priority }.take(limit).map { it.provider }
    }

    /** 某平台在该 region 的热门影片海报（w185），最多 `limit` 张；取不到就返回空。 */
    fun titles(
        client: TmdbClient,
        providerId: Int,
        region: String = "US",
        limit: Int = 4,
    ): List<String> {
        if (providerId <= 0 || limit <= 0) return emptyList()
        val arr = TmdbHome(client).get(
            "/discover/movie?with_watch_providers=$providerId&watch_region=$region" +
                "&sort_by=popularity.desc&language=zh-CN"
        ).first?.optJSONArray("results") ?: return emptyList()
        val out = ArrayList<String>(limit)
        for (i in 0 until arr.length()) {
            val path = TmdbHome.imgPath(arr.optJSONObject(i), "poster_path") ?: continue
            out.add("${TmdbHome.IMG}/w185$path")
            if (out.size >= limit) break
        }
        return out
    }
}