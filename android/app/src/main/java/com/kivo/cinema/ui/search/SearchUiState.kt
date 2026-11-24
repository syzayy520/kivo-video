package com.kivo.cinema.ui.search

import com.kivo.cinema.search.BrowseFilter

/**
 * ④ 吸顶摘要文案（family-tree D4）：类型/地区/年份非默认全部入列，排序始终展示，
 * 「只看可播」开启也必须可见 —— 摘要要能表达全部生效条件。
 * 纯函数，不碰 Compose 状态，可单测。
 */
internal fun filterSummary(filter: BrowseFilter): String {
    val parts = mutableListOf<String>()
    if (filter.genreId != 0) parts += filter.genreLabel
    if (filter.region.isNotEmpty()) parts += filter.regionLabel
    if (filter.year != "全部") parts += filter.year
    parts += filter.sortLabel
    if (filter.onlyPlayable) parts += "只看可播"
    return parts.joinToString(" · ")
}