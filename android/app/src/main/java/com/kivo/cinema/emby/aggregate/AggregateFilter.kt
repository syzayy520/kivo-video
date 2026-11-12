package com.kivo.cinema.emby.aggregate

/**
 * 256 六排筛选的**数据定义 + chip→查询翻译 + 地区映射**（纯逻辑，无 UI）。
 * 一排一组件六份数据：内容类别/排序/地区/年份/类型各在这里定义，UI 只渲染。
 * 地区是客户端过滤（Emby /Items 无制作国家参数），英文国名→中文 chip 见 [regionCountries]。
 */
object AggregateFilter {

    /** 内容类别排：types + 可选 genre（动漫/综艺/演唱会/纪录片靠 genre 区分）。 */
    data class Category(val label: String, val types: String, val genre: String?)
    val categories = listOf(
        Category("全部", "Movie,Series", null),
        Category("电视剧", "Series", null),
        Category("电影", "Movie", null),
        Category("动漫", "Movie,Series", "动画"),
        Category("综艺", "Series", "真人秀"),
        Category("演唱会", "Movie", "音乐"),
        Category("纪录片", "Movie,Series", "纪录"),
    )

    /** 排序排：sort 索引对应 EmbyClient.items 的 SortBy（0 DateCreated / 4 PremiereDate / 3 CommunityRating）。 */
    data class Sorting(val label: String, val sort: Int)
    val sortings = listOf(Sorting("最新更新", 0), Sorting("最新上映", 4), Sorting("影片评分", 3))

    /** 类型墙与类型排共用的 24 类型（与截图一致，服务器 Genres 实测含这些）。 */
    val genres = listOf(
        "剧情", "喜剧", "动作", "动画", "犯罪", "惊悚", "悬疑", "冒险", "科幻", "爱情",
        "动作冒险", "奇幻", "家庭", "恐怖", "历史", "战争", "纪录", "音乐", "西部", "儿童",
        "真人秀", "电视电影", "肥皂剧", "脱口秀",
    )

    /** 地区排（客户端过滤）。 */
    val regions = listOf(
        "中国大陆", "中国香港", "中国台湾", "新加坡", "欧美", "美国", "韩国", "日本",
        "泰国", "英国", "法国", "德国", "意大利", "西班牙", "印度", "俄罗斯",
    )

    private val western = setOf(
        "United States of America", "USA", "United States", "Canada", "United Kingdom", "UK",
        "England", "France", "Germany", "Italy", "Spain", "Australia", "New Zealand", "Ireland",
        "Belgium", "Netherlands", "Sweden", "Norway", "Denmark", "Finland", "Poland", "Austria",
        "Switzerland", "Portugal", "Greece", "Czech Republic", "Hungary", "Mexico", "Brazil", "Argentina",
    )
    private val regionCountries: Map<String, Set<String>> = mapOf(
        "中国大陆" to setOf("China"),
        "中国香港" to setOf("Hong Kong", "Hong Kong SAR China"),
        "中国台湾" to setOf("Taiwan", "Taiwan Province of China"),
        "新加坡" to setOf("Singapore"),
        "美国" to setOf("United States of America", "USA", "United States"),
        "韩国" to setOf("South Korea", "Korea", "Republic of Korea"),
        "日本" to setOf("Japan"),
        "泰国" to setOf("Thailand"),
        "英国" to setOf("United Kingdom", "UK", "England"),
        "法国" to setOf("France"),
        "德国" to setOf("Germany"),
        "意大利" to setOf("Italy"),
        "西班牙" to setOf("Spain"),
        "印度" to setOf("India"),
        "俄罗斯" to setOf("Russia", "Russian Federation"),
        "欧美" to western,
    )

    /** 地区命中：条目任一制作国落在该 chip 的国名集合里。未识别国名按无匹配处理。 */
    fun matchesRegion(countries: List<String>, region: String?): Boolean {
        if (region.isNullOrBlank()) return true
        val set = regionCountries[region] ?: return false
        return countries.any { it in set }
    }

    /** 年份排：decade + 单年 + 更早。years 为逗号分隔逐年；更早走 MaxPremiereDate。 */
    data class YearOption(val label: String, val years: String?, val maxPremiereDate: String?)
    val yearOptions: List<YearOption> = buildList {
        add(decade("2020年代", 2020))
        for (y in 2026 downTo 2020) add(YearOption("$y", "$y", null))
        add(decade("2010年代", 2010))
        add(decade("2000年代", 2000))
        add(decade("90年代", 1990))
        add(decade("80年代", 1980))
        add(decade("70年代", 1970))
        add(decade("60年代", 1960))
        add(YearOption("更早", null, "1960-01-01"))
    }
    private fun decade(label: String, start: Int) =
        YearOption(label, (start until start + 10).joinToString(","), null)

    /** 一次查询的翻译结果：服务端参数 + 客户端地区。 */
    data class Query(
        val search: String, val types: String, val genres: String?, val sort: Int,
        val years: String?, val maxPremiereDate: String?, val region: String?,
    )

    /** 把六排选择翻译成查询。类别 genre 与类型排 genre 合并（逗号=Emby 任一匹配）。 */
    fun translate(
        search: String, category: Category, sorting: Sorting,
        genre: String?, region: String?, year: YearOption?,
    ): Query {
        val genreList = listOfNotNull(category.genre, genre?.takeIf { it.isNotBlank() }).distinct()
        return Query(
            search = search.trim(), types = category.types,
            genres = genreList.joinToString(",").ifBlank { null },
            sort = sorting.sort, years = year?.years, maxPremiereDate = year?.maxPremiereDate,
            region = region,
        )
    }
}