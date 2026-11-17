package com.kivo.cinema.ui.profile.calendar

/**
 * 日历数据层冻结契约（C1）。
 *
 * 只描述「某天有没有看、看了多久、看了什么」，不携带任何尺寸/颜色/字号 ——
 * 渲染由 C2（`CalendarMonthGrid` / `CalendarYearGrid` / `CalendarPage`）负责。
 *
 * ## 周起点口径（重要）
 *
 * 一周**以周日为第一天**，标签顺序 `日 一 二 三 四 五 六`。
 * 依据是既有实现，不是新选择：
 * - [com.kivo.cinema.ui.profile.calendar.WeekStrip] 的 `PreviewDays`（`WeekStrip.kt:166-174`）
 *   把 `2026-11-15`（周日）标为「日」、`2026-11-21`（周六）标为「六」；
 * - `ProfileStats.WEEKDAY_LABELS`（`ProfileStats.kt:54`）= `日 一 二 … 六`，
 *   且 `weekStart()`（`ProfileStats.kt:258-267`）按 `DAY_OF_WEEK - SUNDAY` 回退；
 * - `HistoryCell` 的 KDoc（`ProfileModels.kt:16`）同样写明「day 0 为周日」。
 *
 * ⇒ [CalendarMonthGrid.leadingBlanks] 的量纲是 **周日 = 0 … 周六 = 6**。
 *
 * ⚠️ 显式偏差：C1 任务卡的括号注释写的是「周一 = 0 … 周日 = 6」，同一张卡又要求
 * 「以 WeekStrip 为准」。两者冲突时按后者执行：若用周一起点，月网格相对周历/热力图
 * 会整体错一列（每一格都落在错误星期上），这是用户可见的错误，比契约注释的措辞更硬。
 *
 * ## 尺寸/字号对照
 *
 * 本文件是**纯数据层**（无 Composable、无 `@Preview`），`profile-1.png` 的 px → dp/sp
 * 对照表不适用；网格的格高/字号/圆角口径由 C2 各网格文件在自己的 KDoc 里携带。
 */
data class CalendarDayMark(
    /** `yyyy-MM-dd`，与 [com.kivo.cinema.stats.WatchStore] 的 `daily` 键同口径。 */
    val date: String,
    /**
     * 当日观看分钟，整数截断（与 `ProfileStats.minutesOf` 同口径：`ms / 60_000`）。
     * `0` = **没有记录**，不是「记录了 0 秒」——两者在数据源里无法区分，UI 按无记录渲染。
     */
    val minutes: Int,
    /**
     * 当日看过的片名，最近一次观看优先、已去重，最多 [MAX_TITLES_PER_DAY] 条。
     *
     * 只在 `minutes > 0` 的日子非空（无记录日恒空，不造假）。获取方式与误差边界见
     * [CalendarSource] 的类 KDoc。
     */
    val titles: List<String>,
)

/**
 * 一个自然月的网格。
 *
 * 不变量（C2 可直接依赖，不必再校验）：
 * - [days] 长度恒等于当月天数（28/29/30/31），按 1 日→月末连续排列；
 *   **无记录的日子不省略**（`minutes = 0, titles = emptyList()`），保证网格等长；
 * - [leadingBlanks] ∈ `0..6`，量纲见文件 KDoc（周日 = 0 … 周六 = 6）；
 * - [month] ∈ `1..12`（年视图的月列下标 = `month - 1`）。
 */
data class CalendarMonthGrid(
    val year: Int,
    /** 1..12（**不是** `Calendar.MONTH` 的 0..11）。 */
    val month: Int,
    /** 当月 1 日之前要留白的格子数。 */
    val leadingBlanks: Int,
    val days: List<CalendarDayMark>,
)

/** 单日最多携带几条片名：超出部分在数据层截断，UI 不做二次裁剪。 */
internal const val MAX_TITLES_PER_DAY = 5