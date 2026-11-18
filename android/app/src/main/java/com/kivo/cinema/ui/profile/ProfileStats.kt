package com.kivo.cinema.ui.profile

import android.content.Context
import com.kivo.cinema.library.HomeEntry
import com.kivo.cinema.sources.storage.SourceStore
import com.kivo.cinema.stats.WatchStore
import com.kivo.cinema.ui.profile.model.CalendarDay
import com.kivo.cinema.ui.profile.model.GenreSlice
import com.kivo.cinema.ui.profile.model.HistoryCell
import com.kivo.cinema.ui.profile.model.MonthlyPoint
import com.kivo.cinema.ui.profile.model.ProfileDashboard
import com.kivo.cinema.ui.profile.model.ProfileIdentity
import com.kivo.cinema.ui.profile.model.ProfileModule
import com.kivo.cinema.ui.profile.model.TodaySummary
import com.kivo.cinema.ui.profile.model.WeeklyInsight

import java.text.SimpleDateFormat
import java.util.Calendar
import java.util.Date
import java.util.Locale

/** 热力图周数：20 周 × 7 天 = 140 天，覆盖竞品看板的历史回看范围。 */
private const val HISTORY_WEEKS = 20
private const val DAYS_PER_WEEK = 7
private const val HISTORY_DAYS = HISTORY_WEEKS * DAYS_PER_WEEK
private const val CALENDAR_DAYS = 7
private const val HOURS_PER_DAY = 24

private const val MILLIS_PER_MINUTE = 60_000L
private const val MILLIS_PER_HOUR = 3_600_000f
private const val MINUTES_PER_HOUR = 60

/** 热力图分档下限（分钟）：0 / 1 / 15 / 30 / 60 ⇒ level 0..4。 */
private const val HEAT_LEVEL_1_MINUTES = 1
private const val HEAT_LEVEL_2_MINUTES = 15
private const val HEAT_LEVEL_3_MINUTES = 30
private const val HEAT_LEVEL_4_MINUTES = 60

/** 无 Environment 模型前，唯一真实环境 = 本机（所有观看数据都在本机文件里）。 */
private const val ENVIRONMENT_LOCAL = "本地"
private const val MONTH_LABEL_PATTERN = "M月"

/** 影院头图的身份字面量：没有账号体系，所以不是「读出来的用户数据」，而是固定默认值。 */
private const val IDENTITY_NAME = "我的影院"
private const val IDENTITY_AVATAR_TEXT = "K"

/** 连续天数只往前看 30 天：再早的记录对「连续」没有意义，也避免历史数据无限回溯。 */
private const val STREAK_MAX_DAYS = 30

/** 周报「偏爱某类型」的判定下限（占比 %）：过半才算偏爱，两强并列时不宣称偏好。 */
private const val GENRE_DOMINANT_PERCENT = 50


private val WEEKDAY_LABELS = listOf("日", "一", "二", "三", "四", "五", "六")

/**
 * 「我的」看板的纯聚合层：现有数据 → [ProfileDashboard]。
 *
 * 数据源只有四处，全部是本机既有存储：
 * - [WatchStore]：`daily`（yyyy-MM-dd → 当日观看毫秒）与 `progress`（路径 → 续播位）；
 * - [com.kivo.cinema.library.ContinueWatching]：把续播位与本地片源对齐（与发现页/在看覆盖层同源）；
 * - 看板条目片名与缩略图：[ContinueSource]（对齐不上的条目直接丢弃，不把 URL 当片名）；
 * - 影院身份（`ProfileIdentity`）：[SourceStore] 的源记录条数 + [WatchStore] 的日累计；
 *   **不发任何网络请求** —— `online` 只是「已配置可见源」，不是服务器可达性。
 *
 * 明确没有数据源的项一律留空，不补假值：
 * - 今日 `movies` / `episodes`、24 小时桶：只有「按天累计」，没有按片与小时级采样；
 * - 待看 / 收藏：[com.kivo.cinema.media.marks.FilmMarks] 只存布尔标记，没有标题与海报，
 *   无法构造 [com.kivo.cinema.ui.profile.model.WatchEntry]（标题必填）⇒ 返回空集合，由 UI 渲染空态；
 * - 刷剧 TOP3 / 类型分布 / 分类走势：没有按片观看时长与类型数据 ⇒ 空集合；
 * - 片单 `collectionCards`：没有片单存储 ⇒ 空集合（P2 接片单，不拿收藏/在看凑数）；
 * - 周报 `weekly.titles`：同上，没有按片统计 ⇒ 0（不拿续播位条数冒充「部数」）；
 * - 日历 `hasAiring`：没有播出日程 ⇒ false；`environmentName`：没有环境模型 ⇒ 「本地」。
 *
 * 磁盘与 MediaStore 读取是阻塞调用：请在上 `Dispatchers.IO` 调用本方法。
 */
object ProfileStats {

    /**
     * @param continueItems Shell 侧已对齐的续播列表；看板与发现页/覆盖层共用同一份（null 时现算）。
     */
    fun build(
        context: Context,
        continueItems: List<HomeEntry>? = null,
    ): ProfileDashboard {
        val app = context.applicationContext
        val watch = WatchStore(app)
        val daily = watch.dailyMs(HISTORY_DAYS).toMap()
        val todayKey = dayKey(Calendar.getInstance())
        val calendar = calendarDays(todayKey)
        val month = monthSummary(daily)
        val sourceCount = visibleSourceCount(app)

        val genres = emptyList<GenreSlice>()
        val streak = streakDays(daily, todayKey)
        return ProfileDashboard(
            environmentName = ENVIRONMENT_LOCAL,
            calendar = calendar,
            selectedDay = calendar.firstOrNull { it.isToday },
            today = todaySummary(daily[todayKey] ?: 0L),
            history = historyCells(daily),
            watchlist = emptyList(),
            collections = emptyList(),
            watching = ContinueSource.continueEntries(app, continueItems),
            monthLabel = SimpleDateFormat(MONTH_LABEL_PATTERN, Locale.CHINA).format(Date()),
            monthPoints = month.points,
            monthHours = month.hours,
            monthDelta = month.deltaHours,
            topShows = emptyList(),
            genres = genres,
            trends = emptyList(),
            modules = ProfileModule.values().toList(),
            identity = ProfileIdentity(
                name = IDENTITY_NAME,
                avatarText = IDENTITY_AVATAR_TEXT,
                sourceCount = sourceCount,
                online = sourceCount > 0,
                weeklyHours = watch.dailyMs(DAYS_PER_WEEK).sumOf { it.second } / MILLIS_PER_HOUR,
                streakDays = streak,
            ),
            // 没有片单存储：空就是空，不拿「在看/收藏」凑数（见 ProfileModels.CollectionCard）。
            collectionCards = emptyList(),
            weekly = weeklyInsight(daily, streak, genres),
        )
    }
}

/** 本月逐日点数 + 本月总时长 + 与上月总时长的差值（小时）。 */
private class MonthSummary(
    val points: List<MonthlyPoint>,
    val hours: Float,
    val deltaHours: Float,
)

private fun todaySummary(todayMs: Long): TodaySummary = TodaySummary(
    hours = todayMs / MILLIS_PER_HOUR,
    movies = 0,
    episodes = 0,
    hourBuckets = List(HOURS_PER_DAY) { 0 },
)

/** 本周 7 天（周日起），`hasAiring` 无数据源恒 false。 */
private fun calendarDays(todayKey: String): List<CalendarDay> {
    val start = weekStart(Calendar.getInstance())
    return (0 until CALENDAR_DAYS).map { offset ->
        val cell = (start.clone() as Calendar).also { it.add(Calendar.DAY_OF_MONTH, offset) }
        val key = dayKey(cell)
        CalendarDay(
            label = WEEKDAY_LABELS[cell.get(Calendar.DAY_OF_WEEK) - Calendar.SUNDAY],
            date = key,
            hasAiring = false,
            isToday = key == todayKey,
        )
    }
}

/** 20 周 × 7 天结构恒定产出 140 格；无记录的日期 level=0。 */
private fun historyCells(daily: Map<String, Long>): List<HistoryCell> {
    val start = weekStart(Calendar.getInstance())
    start.add(Calendar.DAY_OF_MONTH, -(HISTORY_WEEKS - 1) * DAYS_PER_WEEK)
    val out = ArrayList<HistoryCell>(HISTORY_DAYS)
    for (week in 0 until HISTORY_WEEKS) {
        for (day in 0 until DAYS_PER_WEEK) {
            val cell = (start.clone() as Calendar).also {
                it.add(Calendar.DAY_OF_MONTH, week * DAYS_PER_WEEK + day)
            }
            out += HistoryCell(week, day, heatLevel(minutesOf(daily[dayKey(cell)])))
        }
    }
    return out
}

private fun heatLevel(minutes: Int): Int = when {
    minutes < HEAT_LEVEL_1_MINUTES -> 0
    minutes < HEAT_LEVEL_2_MINUTES -> 1
    minutes < HEAT_LEVEL_3_MINUTES -> 2
    minutes < HEAT_LEVEL_4_MINUTES -> 3
    else -> 4
}

/** 本月 1 日到月末逐日（未来日期为 0 分钟，即没有数据）。 */
private fun monthSummary(daily: Map<String, Long>): MonthSummary {
    val thisMonth = (Calendar.getInstance().clone() as Calendar).also {
        it.set(Calendar.DAY_OF_MONTH, 1)
    }
    val points = (1..thisMonth.getActualMaximum(Calendar.DAY_OF_MONTH)).map { day ->
        MonthlyPoint(day, minutesOf(daily[monthDayKey(thisMonth, day)]))
    }
    val prevMonth = (thisMonth.clone() as Calendar).also { it.add(Calendar.MONTH, -1) }
    var prevMinutes = 0L
    for (day in 1..prevMonth.getActualMaximum(Calendar.DAY_OF_MONTH)) {
        prevMinutes += minutesOf(daily[monthDayKey(prevMonth, day)])
    }
    val hours = points.sumOf { it.minutes }.toFloat() / MINUTES_PER_HOUR
    val prevHours = prevMinutes.toFloat() / MINUTES_PER_HOUR
    return MonthSummary(points, hours, hours - prevHours)
}

/** 续播记录 → 列表条目由 [ContinueSource] 负责（同源对齐 + 片名 + 缩略图）。 */

private fun minutesOf(ms: Long?): Int = ((ms ?: 0L) / MILLIS_PER_MINUTE).toInt()

private fun monthDayKey(firstOfMonth: Calendar, day: Int): String =
    dayKey((firstOfMonth.clone() as Calendar).also { it.set(Calendar.DAY_OF_MONTH, day) })

/** 本周 7 个日期键，周一 → 周日。[weekStart] 以周日为起点（沿用日历口径），故周一 = 周日 + 1 天。 */
private fun weekDayKeys(): List<String> {
    val monday = weekStart(Calendar.getInstance()).also { it.add(Calendar.DAY_OF_MONTH, 1) }
    return (0 until DAYS_PER_WEEK).map { offset ->
        dayKey((monday.clone() as Calendar).also { it.add(Calendar.DAY_OF_MONTH, offset) })
    }
}

/**
 * 周报聚合（P2），字段口径见 [WeeklyInsight]。
 *
 * `daily` 是 `dailyMs(HISTORY_DAYS)`（140 天 ⊇ 本周），未到/无记录的槽为 0。`hours` 用**毫秒**
 * 求和再换算，避免逐日分钟截断把小时算少（实测 6,219,190 ms ⇒ 1.7276 h；截断得 1.7167 h）。
 *
 * @param streakDays 已算好的连续天数（与头图同源，避免重复回溯）。
 * @param genres 已算好的类型分布（当前恒空，不另造数据）。
 */
private fun weeklyInsight(
    daily: Map<String, Long>,
    streakDays: Int,
    genres: List<GenreSlice>,
): WeeklyInsight {
    val weekMs = weekDayKeys().map { daily[it] ?: 0L }
    val hours = weekMs.sumOf { it } / MILLIS_PER_HOUR
    return WeeklyInsight(
        headline = weeklyHeadline(hours, streakDays, genres),
        hours = hours,
        titles = 0,
        streakDays = streakDays,
        dailyMinutes = weekMs.map { (it / MILLIS_PER_MINUTE).toInt() },
        genres = genres,
    )
}

/**
 * 周报结论句（纯函数：不碰 Context / 磁盘 / 时钟，可直接在 JVM 单测里调用）。
 *
 * 三条分支，自上而下命中即返回：
 * 1. `hours <= 0f` ⇒「本周还没有观看记录」—— 0 时长不编结论；
 * 2. `genres` 非空且最大 `percent` ≥ [GENRE_DOMINANT_PERCENT] ⇒「你本周偏爱{name}」；
 * 3. 其余 ⇒「本周看了 X 小时 · 连续 N 天」，X 保留一位小数（与指标卡同口径）。
 *
 * ✅ 实测（b6e950ee，2026-11-17 05:53）：6,219,190 ms、连续 2 天、无类型 ⇒ 分支 3
 * 「本周看了 1.7 小时 · 连续 2 天」，与头图动态句一致。◐ 推断：阈值 50% 取「过半即偏爱」；分支 2 仅单测覆盖。
 */
internal fun weeklyHeadline(hours: Float, streakDays: Int, genres: List<GenreSlice>): String {
    if (hours <= 0f) return "本周还没有观看记录"
    val top = genres.maxByOrNull { it.percent }
    if (top != null && top.percent >= GENRE_DOMINANT_PERCENT) return "你本周偏爱${top.name}"
    return "本周看了 ${String.format(Locale.CHINA, "%.1f", hours)} 小时 · 连续 $streakDays 天"
}

/** 周日为起点、清零时分秒的对齐本周。 */
private fun weekStart(now: Calendar): Calendar {
    val c = now.clone() as Calendar
    c.set(Calendar.HOUR_OF_DAY, 0)
    c.set(Calendar.MINUTE, 0)
    c.set(Calendar.SECOND, 0)
    c.set(Calendar.MILLISECOND, 0)
    c.add(Calendar.DAY_OF_MONTH, -(c.get(Calendar.DAY_OF_WEEK) - Calendar.SUNDAY))
    return c
}

/** 与 [WatchStore] 的 `yyyy-MM-dd` 键同口径。 */
private fun dayKey(cal: Calendar): String = "%04d-%02d-%02d".format(
    cal.get(Calendar.YEAR), cal.get(Calendar.MONTH) + 1, cal.get(Calendar.DAY_OF_MONTH),
)

/**
 * 已配置的可见源数量：读 [SourceStore] 的持久化记录，隐藏源不计（隐藏源在源管理页也不展示，
 * 计数与页面保持一致）。源存储读不到（版本不支持 / 解密失败 / 文件损坏）一律返回 0，
 * 由 UI 渲染「0 个源」，不猜。
 */
private fun visibleSourceCount(app: Context): Int =
    SourceStore(app).read().getOrNull()?.count { !it.hidden } ?: 0

/**
 * 从今天往前连续「有观看记录」的天数，上限 [STREAK_MAX_DAYS]。
 *
 * 口径是 `dailyMs(1) > 0`：今天为 0 即 0 天（不把「昨天看过」当成连续到今天），
 * 因为头图那句文案是「连续 N 天」而不是「上次连续」。
 *
 * @param daily 已按 `yyyy-MM-dd` 取好的日累计（[HISTORY_DAYS] 天 ⊇ [STREAK_MAX_DAYS]）——
 *   复用同一份 map 避免为头图再读一次盘。
 */
private fun streakDays(daily: Map<String, Long>, todayKey: String): Int {
    val cursor = Calendar.getInstance()
    var day = todayKey   // 第一天直接用调用方的 todayKey，跨零点时不会与本页其它指标错位
    var streak = 0
    while (streak < STREAK_MAX_DAYS && (daily[day] ?: 0L) > 0L) {
        streak++
        cursor.add(Calendar.DAY_OF_MONTH, -1)
        day = dayKey(cursor)
    }
    return streak
}