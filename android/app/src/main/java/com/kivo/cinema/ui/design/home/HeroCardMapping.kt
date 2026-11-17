package com.kivo.cinema.ui.design.home

import com.kivo.cinema.tmdb.TmdbHome

/**
 * D13 · TMDB 列表项 → 首页 hero 卡（纯数据映射，无 IO、无状态）。
 *
 * 单独成文件的原因：[com.kivo.cinema.ui.design.DiscoverPage] 触到 300 行门限，
 * 而映射本身是"一进一出"的单一职责，放在页面文件里会挤掉布局逻辑。
 * 调用方负责在 IO 线程取好 [heroUrl]（首帧用列表自带海报，后台再换竖版顶部海报）。
 */
internal fun heroCardOf(item: TmdbHome.Item, heroUrl: String): HeroCard = HeroCard(
    id = item.id.toString(),
    title = item.title,
    heroUrl = heroUrl,
    metaLine = item.metaLine,
    overview = item.overview,
    rating = item.rating.toDouble(),
    ratingSource = "TMDB",
    trailerKey = item.id,
    isSeries = item.isTv,
)
