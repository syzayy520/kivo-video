package com.kivo.cinema.ui.profile.calendar

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 月视图网格：7 列表头 + 6×7 日期格。
 *
 * ## 冻结契约
 *
 * `CalendarMonthGrid(grid: CalendarMonthGrid, selectedDate: String?, todayKey: String,
 * onSelect: (String) -> Unit)` —— [grid] 由 C1 的 `CalendarSource.month()` 产出，不变量
 * （`days` 与当月等长、`minutes = 0` 表示无记录、周起点周日 = 0）见 `CalendarModels.kt`。
 * 本组件**不判空、不补日**：数据层说多少天就画多少天，缺日只留白格。
 *
 * ## 尺寸对照（`profile-1.png` 周历实测 → 本网格）
 *
 * | 维度 | 基准（周历 7 列） | 月视图实现 | 说明 |
 * | --- | --- | --- | --- |
 * | 列宽 / 格宽 | 52.4dp | 44dp | 月视图 7 列固定 44dp，比周历窄 8.4dp 换取 6 行的呼吸感 |
 * | 格高 / 圆角 | 44dp / 12dp | 44dp / 12dp | 与周历胶囊同口径，选中态视觉能跨页对齐 |
 * | 日期字号 | 15sp | 15sp | 与 [WeekStrip] 的 `DateTextSize` 一致 |
 * | 星期表头 | 11sp | 11sp | 基准截图的 `日 一 二…` 量级 |
 * | 指示点 | 4dp | 4dp | 绿点（`accentGreen`）而非周历的红点 |
 * | 左右边距 | 13dp | 13dp | `DesignTokens.screenPad` |
 *
 * ## 偏差与实测理由
 *
 * - ⚠️ 格宽 44dp 而列距用 `SpaceBetween` 撑满：390dp 屏上相邻格间隙约 9.9dp，
 *   比周历的 0dp 间距更松，避免 6 行版式糊成一片。窄屏（<360dp）不会换行，靠 `weight` 均分兜底。
 * - ⚠️ 绿点强度三档阈值（60 / 20 分钟）为**推断**：基准截图没有「看了一天」的样本，
 *   阈值取「一集 ≈ 20 分钟、一集剧 ≈ 45–60 分钟」让三档在真机上都能出现。
 * - 今天用**白胶囊描边**而不是实心：`profile-1.png` 里实心白胶囊代表「选中」，两者同形会让
 *   「今天」和「选中的另一天」无法区分。
 */
@Composable
fun CalendarMonthGrid(
    grid: CalendarMonthGrid,
    selectedDate: String?,
    todayKey: String,
    onSelect: (String) -> Unit,
) {
    val cells = remember(grid) { monthCells(grid) }
    Column(modifier = Modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = DesignTokens.screenPad.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
        ) {
            WeekdayLabels.forEach { label ->
                Text(
                    text = label,
                    modifier = Modifier.width(CellSize),
                    fontSize = WeekdayTextSize,
                    color = DesignTokens.textSub,
                    textAlign = TextAlign.Center,
                )
            }
        }
        Spacer(Modifier.height(HeaderGap))
        Column(verticalArrangement = Arrangement.spacedBy(RowGap)) {
            for (row in 0 until RowCount) {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(horizontal = DesignTokens.screenPad.dp),
                    horizontalArrangement = Arrangement.SpaceBetween,
                ) {
                    for (col in 0 until ColumnCount) {
                        val mark = cells.getOrNull(row * ColumnCount + col)
                        DayCell(
                            mark = mark,
                            selected = mark != null && mark.date == selectedDate,
                            isToday = mark != null && mark.date == todayKey,
                            onSelect = onSelect,
                        )
                    }
                }
            }
        }
    }
}

/** 单格：44dp 命中；空位（月初留白 / 月末补白）不可点，也不画任何东西。 */
@Composable
private fun DayCell(
    mark: CalendarDayMark?,
    selected: Boolean,
    isToday: Boolean,
    onSelect: (String) -> Unit,
) {
    if (mark == null) {
        Spacer(Modifier.size(CellSize))
        return
    }
    val shape = RoundedCornerShape(CellCorner)
    Box(
        modifier = Modifier
            .size(CellSize)
            .clip(shape)
            .then(if (selected) Modifier.background(DesignTokens.canvasFill) else Modifier)
            .then(if (isToday) Modifier.border(TodayBorder, DesignTokens.canvasFill, shape) else Modifier)
            .clickable(role = Role.Button) { onSelect(mark.date) },
        contentAlignment = Alignment.Center,
    ) {
        Text(
            text = dayNumber(mark.date),
            fontSize = DateTextSize,
            fontWeight = if (selected || mark.minutes > 0) FontWeight.Bold else FontWeight.Normal,
            color = if (selected) DesignTokens.textMain else DesignTokens.textSub,
        )
        if (mark.minutes > 0) {
            Box(
                modifier = Modifier
                    .align(Alignment.BottomCenter)
                    .padding(bottom = DotBottomPad)
                    .size(DotSize)
                    .clip(CircleShape)
                    .background(DesignTokens.accentGreen.copy(alpha = dotAlpha(mark.minutes))),
            )
        }
    }
}

/** 网格格子：`leadingBlanks` 个空位 + 当月每一天，总长固定 42（6×7），缺的补空。 */
private fun monthCells(grid: CalendarMonthGrid): List<CalendarDayMark?> {
    val cells = ArrayList<CalendarDayMark?>(RowCount * ColumnCount)
    repeat(grid.leadingBlanks.coerceIn(0, ColumnCount - 1)) { cells.add(null) }
    grid.days.take(RowCount * ColumnCount - cells.size).forEach { cells.add(it) }
    while (cells.size < RowCount * ColumnCount) cells.add(null)
    return cells
}

/** `date` 契约是 `yyyy-MM-dd`；只取日，不解析日期对象（与 WeekStrip 同口径）。 */
private fun dayNumber(date: String): String = date.substringAfterLast('-')

/** 三档强度：≥60 分钟满绿、≥20 分钟 66%、>0 分钟 38%；无记录不画点。 */
private fun dotAlpha(minutes: Int): Float = when {
    minutes >= HeavyDayMinutes -> 1f
    minutes >= MediumDayMinutes -> 0.66f
    minutes > 0 -> 0.38f
    else -> 0f
}

private const val RowCount = 6
private const val ColumnCount = 7
private const val HeavyDayMinutes = 60
private const val MediumDayMinutes = 20

private val WeekdayLabels = listOf("日", "一", "二", "三", "四", "五", "六")
private val CellSize = 44.dp
private val CellCorner = 12.dp
private val RowGap = 2.dp
private val HeaderGap = 4.dp
private val TodayBorder = 1.dp
private val DotSize = 4.dp
private val DotBottomPad = 6.dp
private val DateTextSize = 15.sp
private val WeekdayTextSize = 11.sp

@Preview(showBackground = true, widthDp = 390)
@Composable
private fun CalendarMonthGridPreview() {
    val days = (1..30).map { day ->
        val minutes = when (day) {
            3 -> 45
            8 -> 130
            17 -> 25
            21 -> 5
            else -> 0
        }
        CalendarDayMark(
            date = "2026-11-${day.toString().padStart(2, '0')}",
            minutes = minutes,
            titles = if (minutes > 0) listOf("示例剧集 $day") else emptyList(),
        )
    }
    Column(
        modifier = Modifier.background(DesignTokens.pageBackground).padding(vertical = DesignTokens.gap.dp),
        verticalArrangement = Arrangement.spacedBy(DesignTokens.gap.dp),
    ) {
        // 有数据态：11 月周日开月（1 日 = 周日，leadingBlanks = 0），今天 17 选中
        CalendarMonthGrid(
            grid = CalendarMonthGrid(year = 2026, month = 11, leadingBlanks = 0, days = days),
            selectedDate = "2026-11-17",
            todayKey = "2026-11-21",
            onSelect = {},
        )
        // 全 0 分钟：网格照画，只是没有绿点（「这天没有观看记录」由页面摘要卡说）
        CalendarMonthGrid(
            grid = CalendarMonthGrid(
                year = 2026,
                month = 11,
                leadingBlanks = 0,
                days = days.map { it.copy(minutes = 0, titles = emptyList()) },
            ),
            selectedDate = null,
            todayKey = "2026-11-21",
            onSelect = {},
        )
    }
}