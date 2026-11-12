package com.kivo.cinema.emby.aggregate

import androidx.compose.ui.graphics.Color
import com.kivo.cinema.emby.EmbyItem

/**
 * 256 类型墙数据：24 类型 + 渐变调色板（一个组件二十四份数据，不写 24 个文件），
 * 以及"按 genre 取真实海报"的出网口（IO，调用方自备线程）。海报必须来自服务器真实条目。
 */
object GenreWallData {
    data class Genre(val name: String, val start: Color, val end: Color)

    val wall: List<Genre> = listOf(
        Genre("剧情", Color(0xFF4A6CF7), Color(0xFF6E8BFF)),
        Genre("喜剧", Color(0xFFFF9A3D), Color(0xFFFFC46B)),
        Genre("动作", Color(0xFFF0503C), Color(0xFFFF7A59)),
        Genre("动画", Color(0xFFFF5FA2), Color(0xFFFF8FC0)),
        Genre("犯罪", Color(0xFF3E4A5E), Color(0xFF5C6B84)),
        Genre("惊悚", Color(0xFF5B2A86), Color(0xFF8A4FC8)),
        Genre("悬疑", Color(0xFF1F6F6B), Color(0xFF3AA08F)),
        Genre("冒险", Color(0xFF2FA36B), Color(0xFF5FC98E)),
        Genre("科幻", Color(0xFF2BB3E8), Color(0xFF5F7CFF)),
        Genre("爱情", Color(0xFFFF6B81), Color(0xFFFFA0B4)),
        Genre("动作冒险", Color(0xFFE8542F), Color(0xFFFF8A4C)),
        Genre("奇幻", Color(0xFF7A4FE8), Color(0xFFA98BFF)),
        Genre("家庭", Color(0xFF8FBF3F), Color(0xFFB9DC6B)),
        Genre("恐怖", Color(0xFF4A1420), Color(0xFF7A2438)),
        Genre("历史", Color(0xFF8A6A3B), Color(0xFFB08D57)),
        Genre("战争", Color(0xFF5C6B3C), Color(0xFF84945C)),
        Genre("纪录", Color(0xFF3D7EA6), Color(0xFF6BA3C8)),
        Genre("音乐", Color(0xFFB04FE8), Color(0xFFE08BFF)),
        Genre("西部", Color(0xFFB07A3C), Color(0xFFD8A860)),
        Genre("儿童", Color(0xFF3FB8E8), Color(0xFF7FD8F7)),
        Genre("真人秀", Color(0xFFFF7A59), Color(0xFFFFB08A)),
        Genre("电视电影", Color(0xFF4A5AC8), Color(0xFF7A88E8)),
        Genre("肥皂剧", Color(0xFFE84F9A), Color(0xFFFF8BC0)),
        Genre("脱口秀", Color(0xFFE8A23C), Color(0xFFFFC873)),
    )

    /** 取该 genre 的真实海报（扇形堆用）。失败返回空——卡片仍显示渐变与名字，不放假图。 */
    fun posters(sources: List<AggregateSource>, genre: String, count: Int = 3): List<EmbyItem> {
        val src = sources.firstOrNull() ?: return emptyList()
        return runCatching {
            src.client.items(src.auth, limit = count, types = "Movie,Series", genres = genre).items
                .filter { it.posterUrl.isNotBlank() }
        }.getOrDefault(emptyList())
    }
}