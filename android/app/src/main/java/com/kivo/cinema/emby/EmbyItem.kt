package com.kivo.cinema.emby

/** Emby 条目数据类（224 设计稿家族树）：海报墙一个卡片的最小信息集。 */
data class EmbyItem(
    val id: String,
    val name: String,
    val year: Int?,        // ProductionYear，旧条目可能没有
    val posterUrl: String, // /Items/{id}/Images/Primary?maxWidth=320（tag 已拼）
    val series: Boolean = false, // 255：Type==Series，钻取网格/详情据此分电影/剧
    // 256 聚合搜索可选字段：仅 rich 查询填充，默认值不破坏单源浏览/详情旧调用。
    val rating: Double? = null,          // CommunityRating，结果卡右下评分
    val genres: List<String> = emptyList(),
    val countries: List<String> = emptyList(), // ProductionLocations，英文国名，地区客户端过滤
    val premiereDate: String = "",       // yyyy-MM-dd，日期徽标/年份回退
    val childCount: Int? = null,         // Series 季数 → "共N季"
    val dateCreated: String = "",        // 入库时间，聚合"最新更新"全局合并排序键
)
