package com.kivo.cinema.library

/**
 * NAS 片库的分类（用户要求：电影/剧集/综艺/动漫/演唱会…必须分开，且可在设置里开关）。
 *
 * 分类**由 NAS 的根目录名决定**（用户自己的整理习惯就是分类），不做内容识别——
 * 猜错分类比按目录归类更糟。认不出的目录整体跳过，不塞进"其他"污染分类。
 */
enum class NasCategory(val label: String) {
    Movie("电影"),
    Tv("剧集"),
    Variety("综艺"),
    Anime("动漫"),
    Concert("演唱会"),
    Documentary("纪录片"),
    ;

    /** 设置里的存储键（避免用中文做 key）。 */
    val key: String get() = name.lowercase()

    companion object {
        /** 真机 12T 实测存在的目录名 → 分类映射。 */
        fun fromDirName(name: String): NasCategory? = when (name.trim()) {
            "电影", "REMUX", "原盘" -> Movie
            "剧集", "电视剧", "美剧", "英剧", "日剧", "韩剧", "国剧" -> Tv
            "综艺" -> Variety
            "动漫", "动画片", "番剧", "动画" -> Anime
            "演唱会", "音乐", "MV" -> Concert
            "纪录片", "记录片", "记录" -> Documentary
            else -> null
        }

        /** 首页与设置的展示顺序。 */
        val ordered: List<NasCategory> = listOf(Movie, Tv, Variety, Anime, Concert, Documentary)

        /** 默认显示的分类（最常用）；其余默认关闭，用户可在设置里开。 */
        val defaultVisible: Set<NasCategory> = setOf(Movie, Tv)
    }
}
