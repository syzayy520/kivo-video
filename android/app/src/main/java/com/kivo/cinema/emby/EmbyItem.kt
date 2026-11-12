package com.kivo.cinema.emby

/** Emby 条目数据类（224 设计稿家族树）：海报墙一个卡片的最小信息集。 */
data class EmbyItem(
    val id: String,
    val name: String,
    val year: Int?,        // ProductionYear，旧条目可能没有
    val posterUrl: String, // /Items/{id}/Images/Primary?maxWidth=320（tag 已拼）
    val series: Boolean = false, // 255：Type==Series，钻取网格/详情据此分电影/剧
)
