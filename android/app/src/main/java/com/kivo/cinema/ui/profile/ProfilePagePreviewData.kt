package com.kivo.cinema.ui.profile

import com.kivo.cinema.ui.profile.model.CalendarDay
import com.kivo.cinema.ui.profile.model.GenreSlice
import com.kivo.cinema.ui.profile.model.HistoryCell
import com.kivo.cinema.ui.profile.model.MonthlyPoint
import com.kivo.cinema.ui.profile.model.ProfileDashboard
import com.kivo.cinema.ui.profile.model.ProfileIdentity
import com.kivo.cinema.ui.profile.model.ProfileModule
import com.kivo.cinema.ui.profile.model.TodaySummary
import com.kivo.cinema.ui.profile.model.TopShow
import com.kivo.cinema.ui.profile.model.TrendStat
import com.kivo.cinema.ui.profile.model.WatchEntry
import com.kivo.cinema.ui.profile.model.WeeklyInsight

/**
 * `ProfilePage` 的 @Preview 样例数据（◐ 推断形状，不是生产数据来源）。
 *
 * 拆出本文件的理由与 [ProfilePageGradient] 相同：页面文件要守住 300 行上限，
 * 而样例数据每个模块都要有非空形状，否则 Preview 看不到密度是否达标。
 * 数值与基准 `profile-1..5.png` 同量级（1.7 小时 / 1 部电影 / 2 集剧 / 单日尖峰）。
 */
internal fun previewDashboard(): ProfileDashboard {
    val today = CalendarDay("二", "2026-11-17", hasAiring = false, isToday = true)
    return ProfileDashboard(
        environmentName = "本地",
        calendar = PreviewWeek.map { (label, date) ->
            CalendarDay(label, date, hasAiring = false, isToday = date == today.date)
        },
        selectedDay = today,
        today = TodaySummary(
            hours = 0.4f,
            movies = 1,
            episodes = 2,
            hourBuckets = List(24) { if (it in 19..21) 8 else 0 },
        ),
        history = (0 until PreviewWeeks).flatMap { week ->
            (0 until 7).map { day -> HistoryCell(week, day, if ((week + day) % 5 == 0) 3 else 0) }
        },
        watchlist = listOf(WatchEntry("天国王朝", "2005 · 电影", posterPath = null, progressPercent = null)),
        collections = listOf(WatchEntry("收藏", "12 部", posterPath = null, progressPercent = null)),
        watching = listOf(
            WatchEntry("kivo-b2-演示剧集", "第 3 集 · 75%", posterPath = null, progressPercent = 75),
        ),
        monthLabel = "11月",
        monthPoints = (1..30).map { MonthlyPoint(it, if (it == 15) 102 else 0) },
        monthHours = 1.7f,
        monthDelta = 1.7f,
        topShows = listOf(TopShow("示范剧集", 1.2f), TopShow("示范电影", 0.5f)),
        genres = listOf(GenreSlice("剧集", 62), GenreSlice("电影", 38)),
        trends = listOf(TrendStat("剧集", 1.2f, 0.6f), TrendStat("电影", 0.5f, -0.2f)),
        modules = ProfileModule.values().toList(),
        identity = ProfileIdentity(
            name = "我的影院",
            avatarText = "K",
            sourceCount = 2,
            online = true,
            weeklyHours = 1.7f,
            streakDays = 2,
        ),
        weekly = WeeklyInsight(
            headline = "本周看了 1.7 小时 · 连续 2 天",
            hours = 1.7f,
            titles = 0,
            streakDays = 2,
            dailyMinutes = listOf(0, 15, 30, 15, 0, 0, 0),
            genres = listOf(GenreSlice("剧集", 62), GenreSlice("电影", 38)),
        ),
    )
}

/** 一周 7 天（周日→周六），与 `ProfileStats.calendarDays` 同口径。 */
private val PreviewWeek = listOf(
    "日" to "2026-11-15", "一" to "2026-11-16", "二" to "2026-11-17", "三" to "2026-11-18",
    "四" to "2026-11-19", "五" to "2026-11-20", "六" to "2026-11-21",
)

/** 热力图周数：与 `ProfileStats.HISTORY_WEEKS` 同口径（20 周 × 7 天 = 140 格）。 */
private const val PreviewWeeks = 20