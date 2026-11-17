package com.kivo.cinema.ui.profile.calendar

import android.content.Context
import com.kivo.cinema.library.ContinueWatching
import com.kivo.cinema.stats.WatchStore
import com.kivo.cinema.ui.profile.ProfileNames
import java.text.SimpleDateFormat
import java.util.Calendar
import java.util.Date
import java.util.Locale

/**
 * 日历数据层（C1 冻结契约）：把 [WatchStore] 的「按天毫秒」投影成月矩阵 / 年热度 / 空态判定。
 *
 * 数据源**只有** [WatchStore.dailyMs] 与 [ContinueWatching.aligned]，不发网络请求、不读片源元数据。
 * 三个入口都按「目标年份」现算窗口，**不写死 7 / 140 / 3650 天**。
 *
 * ## 当年窗口怎么算（替 [WatchStore.dailyMs] 补上「任意年段」）
 *
 * `dailyMs(n)` 的窗口是「今天往前 n 天」，终点固定为今天、起点不可指定。要拿到 `year` 年
 * 1 月 1 日起的数据，窗口起点必须 ≤ 当年 1 月 1 日：
 *
 * ```text
 * n = 从 year-01-01 到今天的自然日数 + 1   （今天在末位，键 = yyyy-MM-dd）
 * ```
 *
 * 再按 `"$year-"` 前缀过滤出当年（未来日期天然不在窗口内）。所以：
 * - 当年（2026）n ≈ 321，不是 140；
 * - 2025 年 n ≈ 686；
 * - 过去年份只按「到今年今天」的长度算，**不叠加 10 年**；目标年在未来（晚于今年）⇒ 不读盘，全 0。
 * - 跨年求天数用 [Calendar] 逐日递增（夏令时/闰日安全），不用 `ms / 86_400_000`。
 *
 * ## 性能与线程
 *
 * 所有函数都会读盘（[WatchStore.load] 会解析一次 `watch.json`）并可能查 MediaStore
 * （[ContinueWatching.aligned] 现查 [com.kivo.cinema.library.LocalVideoSource]）。
 * **必须在 IO 线程调用**；页面切换月份时建议自行缓存结果，本层不做缓存（无状态，便于单测）。
 *
 * ## titles 的来源与已知误差（不造假，逐条写明）
 *
 * 1. 来源：`ContinueWatching.aligned(app, watch, [TITLE_SOURCE_LIMIT])` 的 `MediaItem`，
 *    片名经 [ProfileNames.display] 解析（与「我的/在看」同一条规则，URL 永不上屏）。
 * 2. 归日：`WatchStore` **没有**「按片 × 按日」记录，唯一可用的时间锚点是
 *    `Progress.updatedAtMs`（播放器最后一次上报时刻）⇒ 用它所在的本地自然日归日。
 *    同一部片当天多次播放只记一次（按标题去重），取**最后一次**上报所在日。
 * 3. 误差边界（真实存在，不补数据）：
 *    - 跨零点观看（23:50 播到 00:10）会归到 00:10 那天，前一天的 `titles` 可能少一部；
 *    - 播到 ≥95% 的记录会被 [ContinueWatching.aligned] 的正常过滤丢掉（该片已无续播位）
 *      ⇒ 看完一整部片的那天可能查不到它的片名，`minutes` 仍然正确（来自 `daily`）；
 *    - 只对齐得上本地片源；网络源 key 永远对不上，因此不出现在 `titles`；
 *    - 最多回溯 [TITLE_SOURCE_LIMIT] 条最近续播记录；更早的片不再提供片名（当日 `minutes` 不受影响）。
 * 4. 只在 `minutes > 0` 的日子填写：无记录日 `titles = emptyList()`（[CalendarDayMark] 契约）。
 */
object CalendarSource {

    /** `watch.json` 与 [com.kivo.cinema.ui.profile.ProfileStats] 的日期键口径一致。 */
    private const val DAY_PATTERN = "yyyy-MM-dd"

    private const val MILLIS_PER_MINUTE = 60_000L
    private const val MONTHS_PER_YEAR = 12
    private const val MIN_YEAR = 1970
    private const val MAX_YEAR = 9999

    /**
     * `titles` 的片名池上限：按 `updatedAtMs` 倒序取最近这么多个续播位。
     * 60 条约等于「最近几周看过的全部片子」，再大只是白查 MediaStore。
     */
    private const val TITLE_SOURCE_LIMIT = 60

    /**
     * [hasRecords] 的回溯上限（10 年，含 2 个闰日余量）。
     *
     * `WatchStore` 没有「是否为空」的廉价入口，也没有暴露全量 `daily` 键集合；
     * 一次扫到应用生命周期起点意味着逐日格式化上万次。10 年覆盖了任何真实使用史，
     * 超出部分不影响页面（网格仍按月返回 0），因此这里明示上限而不是假装「任意」。
     */
    private const val HAS_RECORDS_SCAN_DAYS = 3660

    /**
     * 某年某月（1..12）的完整网格。
     *
     * `year` 允许任意合理年份（[MIN_YEAR]..[MAX_YEAR]，含未来年份 —— 返回全 0 网格），
     * `month` 必须是 1..12（`Calendar.MONTH` 的 0..11 会让 `leadingBlanks` 静默错月）。
     *
     * @throws IllegalArgumentException `year`/`month` 越界（契约违例，不静默纠正）。
     */
    fun month(context: Context, year: Int, month: Int): CalendarMonthGrid {
        requireYear(year)
        require(month in 1..MONTHS_PER_YEAR) { "month 必须落在 1..$MONTHS_PER_YEAR，收到 $month" }
        val app = context.applicationContext
        val watch = WatchStore(app)
        val daily = yearDaily(watch, year)
        // 空态（整年无计时）时不查 MediaStore：没有观看就没有片名可匹配。
        val titlesByDate = if (daily.values.any { it > 0L }) titlesByDate(app, watch) else emptyMap()

        val dayCount = daysInMonth(year, month)
        val days = ArrayList<CalendarDayMark>(dayCount)
        for (day in 1..dayCount) {
            val key = dayKey(year, month, day)
            val minutes = minutesOf(daily[key] ?: 0L)
            days += CalendarDayMark(
                date = key,
                minutes = minutes,
                titles = if (minutes > 0) titlesByDate[key].orEmpty() else emptyList(),
            )
        }
        return CalendarMonthGrid(
            year = year,
            month = month,
            leadingBlanks = leadingBlanks(year, month),
            days = days,
        )
    }

    /**
     * 该年 12 个月的观看总分钟（下标 0 = 1 月），用于年视图热度。
     *
     * 未来月份/未来年份为 0；整数截断按「月内各日分钟求和」（不是月总毫秒整体截断，
     * 逐日截断最多每月损失 <1 分钟，可接受且与 [CalendarDayMark.minutes] 可加一致）。
     */
    fun yearMinutes(context: Context, year: Int): List<Int> {
        requireYear(year)
        val daily = yearDaily(WatchStore(context.applicationContext), year)
        return (1..MONTHS_PER_YEAR).map { month ->
            (1..daysInMonth(year, month)).sumOf { day ->
                minutesOf(daily[dayKey(year, month, day)] ?: 0L)
            }
        }
    }

    /**
     * 是否有任何观看记录（决定日历页空态）。
     *
     * 口径：最近 [HAS_RECORDS_SCAN_DAYS] 天内任意一天 `> 0` ms 即 true。
     * 与「目标年份」无关 —— 空态说的是「这台设备从没用它看过片」，不是「当年没看过」。
     */
    fun hasRecords(context: Context): Boolean {
        val watch = WatchStore(context.applicationContext)
        return watch.dailyMs(HAS_RECORDS_SCAN_DAYS).any { it.second > 0L }
    }

    // ── 年份窗口 ────────────────────────────────────────────────────────────

    /** `year` 年 1 月 1 日 → 今天的 `dailyMs` 窗口长度；目标年未开始（未来年）返回 0。 */
    private fun windowDays(year: Int): Int {
        // 一次取「今天」：分开取三次会在跨零点的瞬间把年/月/日拼成不同日期。
        val now = Calendar.getInstance()
        val today = startOfDay(
            now.get(Calendar.YEAR),
            now.get(Calendar.MONTH),
            now.get(Calendar.DAY_OF_MONTH),
        )
        val start = startOfDay(year, Calendar.JANUARY, 1)
        if (start > today) return 0
        var days = 1
        val cursor = Calendar.getInstance().also { it.timeInMillis = start }
        while (cursor.timeInMillis < today) {
            cursor.add(Calendar.DAY_OF_MONTH, 1)
            days++
        }
        return days
    }

    /** 某年 `yyyy-MM-dd → ms` 的日表（只含当年、且不晚于今天；未来年为空）。 */
    private fun yearDaily(watch: WatchStore, year: Int): Map<String, Long> {
        val days = windowDays(year)
        if (days <= 0) return emptyMap()
        val prefix = String.format(Locale.US, "%04d-", year)
        val out = LinkedHashMap<String, Long>(days)
        watch.dailyMs(days).forEach { (key, ms) -> if (key.startsWith(prefix)) out[key] = ms }
        return out
    }

    // ── titles ──────────────────────────────────────────────────────────────

    /** `yyyy-MM-dd → 片名列表`（最近优先、去重、最多 [MAX_TITLES_PER_DAY]）。 */
    private fun titlesByDate(context: Context, watch: WatchStore): Map<String, List<String>> {
        val fmt = SimpleDateFormat(DAY_PATTERN, Locale.US)
        val byDate = LinkedHashMap<String, LinkedHashSet<String>>()
        ContinueWatching.aligned(context, watch, TITLE_SOURCE_LIMIT).forEach { (item, progress) ->
            val title = ProfileNames.display(item.path, item.title)
            byDate.getOrPut(fmt.format(Date(progress.updatedAtMs))) { LinkedHashSet() }.add(title)
        }
        return byDate.mapValues { (_, titles) -> titles.take(MAX_TITLES_PER_DAY) }
    }

    // ── 日期算术（全部走 Calendar，夏令时/闰年安全） ─────────────────────────

    /**
     * 当月 1 日前的空格数，**周日 = 0 … 周六 = 6**（口径见 `CalendarModels.kt` 文件 KDoc）。
     * `Calendar.DAY_OF_WEEK` 是 `SUNDAY=1 … SATURDAY=7`，减 SUNDAY 即得该量纲。
     */
    private fun leadingBlanks(year: Int, month: Int): Int {
        val first = Calendar.getInstance().also {
            it.clear()
            it.set(year, month - 1, 1)
        }
        return first.get(Calendar.DAY_OF_WEEK) - Calendar.SUNDAY
    }

    private fun daysInMonth(year: Int, month: Int): Int {
        val first = Calendar.getInstance().also {
            it.clear()
            it.set(year, month - 1, 1)
        }
        return first.getActualMaximum(Calendar.DAY_OF_MONTH)
    }

    /** 当日 0 点的毫秒时刻（本地时区），用于逐日比较。 */
    private fun startOfDay(year: Int, month0: Int, day: Int): Long {
        val cal = Calendar.getInstance()
        cal.clear()
        cal.set(year, month0, day)
        return cal.timeInMillis
    }

    private fun dayKey(year: Int, month: Int, day: Int): String =
        String.format(Locale.US, "%04d-%02d-%02d", year, month, day)

    private fun minutesOf(ms: Long): Int = (ms / MILLIS_PER_MINUTE).toInt()

    private fun requireYear(year: Int) {
        require(year in MIN_YEAR..MAX_YEAR) { "year 必须在 $MIN_YEAR..$MAX_YEAR，收到 $year" }
    }
}