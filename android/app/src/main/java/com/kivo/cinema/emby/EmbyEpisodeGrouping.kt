package com.kivo.cinema.emby

/**
 * 一次全量拉集 → 按季分组（D366）。
 *
 * ## 为什么要独立成纯函数
 * 「一次请求拿整剧集」有个**静默失败面**：服务器若分页截断，拿到的集会少，
 * 而少一集用户看不出来、选集面板却会缺。所以这里把"能否用这份全量数据"的判定做成
 * 可单测的纯逻辑：**每一季都至少有一集**才算覆盖完整，否则返回 null 让调用方退回逐季请求。
 *
 * 单一职责：只做分组与覆盖判定，不发请求、不碰缓存、不持有状态。
 */
internal object EmbyEpisodeGrouping {

    /**
     * @param seasons 季表（来自 `/Shows/{id}/Seasons`，已按 index 升序）
     * @param all 全量集（来自 `/Shows/{id}/Episodes`，不带 SeasonId）
     * @return 与 [seasons] 同序的分组；**任一季为空则返回 null**（视为不完整，调用方回退）
     */
    fun group(
        seasons: List<EmbyShows.Season>,
        all: List<EmbyShows.Episode>,
    ): List<Pair<EmbyShows.Season, List<EmbyShows.Episode>>>? {
        if (seasons.isEmpty()) return emptyList()
        val bySeason = all.groupBy { it.seasonIndex }
        val covered = seasons.all { bySeason[it.index]?.isNotEmpty() == true }
        if (!covered) return null
        return seasons.map { season -> season to bySeason.getValue(season.index) }
    }
}
