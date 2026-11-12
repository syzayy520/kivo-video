package com.kivo.cinema.ui.detail

import com.kivo.cinema.emby.EmbyShows
import java.time.LocalDate
import java.time.YearMonth

/**
 * 255 #31 追剧日历纯数据（无 UI、无网络）：从既有集 airDate 推播日表/进度/月模型。
 * 不造假日程——无 airDate 的集忽略；全缺播日时进度退化为「共 N 集」，由 UI 诚实呈现。
 */
internal object DetailCalendarData {

    /** 播日表：airDate → 该日播出的集序号（升序）。 */
    fun airDays(episodes: List<EmbyShows.Episode>): Map<LocalDate, List<Int>> =
        episodes.mapNotNull { ep ->
            parseDate(ep.airDate)?.let { it to ep.index }
        }.groupBy({ it.first }, { it.second }).mapValues { it.value.sorted() }

    data class Progress(val aired: Int, val total: Int, val nextDate: LocalDate?)

    fun progress(episodes: List<EmbyShows.Episode>, today: LocalDate): Progress {
        val aired = episodes.count { ep -> parseDate(ep.airDate)?.let { !it.isAfter(today) } == true }
        val next = airDays(episodes).keys.filter { it.isAfter(today) }.minOrNull()
        return Progress(aired, episodes.size, next)
    }

    /** 播出日历条右侧标签：有播日 → 「MM-DD 集aired/total」；无播日 → 「共total集」；空集 → ""。 */
    fun barLabel(episodes: List<EmbyShows.Episode>, today: LocalDate): String {
        if (episodes.isEmpty()) return ""
        val days = airDays(episodes)
        if (days.isEmpty()) return "共${episodes.size}集"
        val p = progress(episodes, today)
        val date = p.nextDate ?: days.keys.maxOrNull() ?: today
        return "%02d-%02d 集%d/%d".format(date.monthValue, date.dayOfMonth, p.aired, p.total)
    }

    data class Cell(val day: Int?, val date: LocalDate?, val eps: List<Int>, val isToday: Boolean)
    data class Month(val title: String, val weeks: List<List<Cell>>)

    /** 月视图（周日起始）：补前后空位凑整周，每格挂播日集序号与今日标记。 */
    fun month(ym: YearMonth, days: Map<LocalDate, List<Int>>, today: LocalDate): Month {
        val offset = ym.atDay(1).dayOfWeek.value % 7
        val cells = ArrayList<Cell>()
        repeat(offset) { cells.add(Cell(null, null, emptyList(), false)) }
        for (d in 1..ym.lengthOfMonth()) {
            val date = ym.atDay(d)
            cells.add(Cell(d, date, days[date] ?: emptyList(), date == today))
        }
        while (cells.size % 7 != 0) cells.add(Cell(null, null, emptyList(), false))
        return Month("${ym.year}年${ym.monthValue}月", cells.chunked(7))
    }

    /** 一周内连续播日段（供刷痕连播带）：返回列下标区间。 */
    fun runs(week: List<Cell>): List<IntRange> {
        val out = mutableListOf<IntRange>()
        var start = -1
        for (i in week.indices) {
            val has = week[i].eps.isNotEmpty()
            if (has && start < 0) start = i
            if (!has && start >= 0) { out.add(start..(i - 1)); start = -1 }
        }
        if (start >= 0) out.add(start..(week.size - 1))
        return out
    }

    /** 一段连播的集数范围标签：单集「5」，多集「1-4」。 */
    fun rangeLabel(week: List<Cell>, run: IntRange): String {
        val eps = run.flatMap { week[it].eps }.sorted()
        if (eps.isEmpty()) return ""
        return if (eps.size == 1) "${eps.first()}" else "${eps.first()}-${eps.last()}"
    }

    private fun parseDate(s: String): LocalDate? =
        if (s.isBlank()) null else runCatching { LocalDate.parse(s) }.getOrNull()
}