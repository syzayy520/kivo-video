package com.kivo.cinema.ui.design.home

/**
 * 317 · **Hero 页的中立数据模型**（TMDB 与 Emby 共用一套渲染）。
 *
 * ## 为什么需要它
 * 改造前 [HeroPage] 直接吃 `TmdbHome.Item` ⇒ 服务器首页（Emby 数据）没法复用同一套
 * 大海报渲染，只能另写一份 —— 那就是平行实现（铁律禁止）。
 * 抽成中立模型后：**一套渲染、两套适配**，视觉必然一致（这正是"和我们首页那样"的要求）。
 *
 * ## 字段为什么是这 9 个
 * 全部来自 [HeroPage] 实际读取的位置，一个不多：
 * `title`（大标题）、`heroUrl`（铺屏图）、`metaLine`（元信息行）、
 * `overview`（简介）、`rating` + `ratingSource`（评分徽章）、
 * `logoUrl`（片名艺术字，可空 ⇒ 回落文字标题）、
 * `trailerKey`（TMDB 预告片查询键；Emby 没有 ⇒ null ⇒ 不显示预告）。
 *
 * `id` + `isSeries` 只用于**点击后跳详情**时还原来源。
 */
data class HeroCard(
    /** 来源侧的唯一 id（TMDB 是数字串，Emby 是 GUID）。 */
    val id: String,
    val title: String,
    /** 铺屏大图；空串 = 无图，渲染层走渐变兜底。 */
    val heroUrl: String,
    /** 已拼好的元信息行（「韩国 · 剧情 · 2025-09-24 · 12集」）。 */
    val metaLine: String,
    val overview: String,
    val rating: Double?,
    val ratingSource: String,
    /** 片名艺术字；null = 用文字标题。 */
    val logoUrl: String? = null,
    /** TMDB 预告片查询键；null = 无预告能力（Emby）。 */
    val trailerKey: Int? = null,
    val isSeries: Boolean = false,
)
