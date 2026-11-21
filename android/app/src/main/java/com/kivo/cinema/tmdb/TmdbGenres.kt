package com.kivo.cinema.tmdb

/**
 * 电影类型（B3 冻结模型）。[movie] 恒给 `backdropUrl = null`；
 * 只有调用方显式使用 `TmdbGenres.backdropFor` / `TmdbGenres.fillBackdrops` 时才会有底图。
 */
data class TmdbGenre(val id: Int, val name: String, val backdropUrl: String?)

/**
 * 电影类型（首页「分类」卡行的数据源）。
 *
 * 端点：`GET /genre/movie/list?language=zh-CN`（✅ 实测 19 条：动作/冒险/动画/喜剧/犯罪/
 * 纪录/剧情/家庭/奇幻/历史/恐怖/音乐/悬疑/爱情/科幻/电视电影/惊悚/战争/西部）。
 *
 * `backdropUrl` 在 [movie] 里**固定为 null**（B3 分类卡用文字 + 底色，不拉图）：这是刻意取舍，
 * 不是缺失 —— 19 个类型各拉一次 discover 会让首屏多 19 次请求且大多被裁掉。
 * 需要底图时走 [fillBackdrops]（只补前 N 个）或 [backdropFor]（按单个 id 取）。
 *
 * 三条取图办法共用一个 discover URL（`language=zh-CN&with_genres={id}&sort_by=popularity.desc`），
 * 因此同一次会话里 [backdropFor]、[posters]、[fillBackdrops] 命中 `TmdbHome.get` 的 12h 列表缓存，
 * 现实中请求数比调用次数更少（✅ 缓存键 = 完整 URL 字符串）。
 *
 * [backdropFor]：取**首条**有 `backdrop_path` 的结果（w780）；全部无图 → null（不造假）。
 * [posters]：取前 `limit` 条 `poster_path` → w342（分类海报墙，卡宽约 68–80dp，w342 足够）。
 * [fillBackdrops]：只对前 `max` 个 genre 串行取底图并 `copy`，其余原样返回；
 * 总请求数 ≤ `max`（默认 6），须在 IO 线程调用。
 *
 * 失败语义：无 key / 无网 / 非 2xx → 空列表（[movie] / [posters]）或 null（[backdropFor]）；
 * [fillBackdrops] 单项失败只留 null，不影响其它项。
 * 线程：同步阻塞，须在调用方 IO 线程执行。
 */
object TmdbGenres {

    /** 三条取图办法的同一份 discover URL —— 只有 URL 完全一致才能共享 12h 缓存。 */
    private fun discoverUrl(genreId: Int): String =
        "/discover/movie?language=zh-CN&with_genres=$genreId&sort_by=popularity.desc"

    fun movie(client: TmdbClient): List<TmdbGenre> {
        val arr = TmdbHome(client).get("/genre/movie/list?language=zh-CN").first
            ?.optJSONArray("genres") ?: return emptyList()
        val out = ArrayList<TmdbGenre>(arr.length())
        for (i in 0 until arr.length()) {
            val o = arr.optJSONObject(i) ?: continue
            val id = o.optInt("id")
            val name = o.optString("name")
            if (id <= 0 || name.isBlank()) continue
            out.add(TmdbGenre(id = id, name = name, backdropUrl = null))
        }
        return out
    }

    fun backdropFor(client: TmdbClient, genreId: Int): String? {
        if (genreId <= 0) return null
        val arr = TmdbHome(client).get(discoverUrl(genreId)).first
            ?.optJSONArray("results") ?: return null
        for (i in 0 until arr.length()) {
            val path = TmdbHome.imgPath(arr.optJSONObject(i), "backdrop_path") ?: continue
            return "${TmdbHome.IMG}/w780$path"
        }
        return null
    }

    /** 分类海报墙：该类型人气前 `limit` 条海报（w342）；失败或全无图 → 空列表。 */
    fun posters(client: TmdbClient, genreId: Int, limit: Int = 10): List<String> {
        if (genreId <= 0 || limit <= 0) return emptyList()
        val arr = TmdbHome(client).get(discoverUrl(genreId)).first
            ?.optJSONArray("results") ?: return emptyList()
        val out = ArrayList<String>(limit)
        for (i in 0 until arr.length()) {
            val path = TmdbHome.imgPath(arr.optJSONObject(i), "poster_path") ?: continue
            out.add("${TmdbHome.IMG}/w342$path")
            if (out.size >= limit) break
        }
        return out
    }

    /**
     * 只给前 `max` 个类型补底图（串行，总请求 ≤ max）。已带底图的项不重复请求。
     * 返回新列表：前 `max` 项为 copy 后的值，其余保持原对象。
     */
    fun fillBackdrops(
        client: TmdbClient,
        genres: List<TmdbGenre>,
        max: Int = 6,
    ): List<TmdbGenre> {
        if (max <= 0) return genres
        val cut = minOf(max, genres.size)
        return genres.mapIndexed { index, genre ->
            if (index >= cut) genre
            else genre.copy(backdropUrl = genre.backdropUrl ?: backdropFor(client, genre.id))
        }
    }
}