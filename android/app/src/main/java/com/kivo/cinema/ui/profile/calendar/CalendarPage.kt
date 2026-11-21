package com.kivo.cinema.ui.profile.calendar

import android.content.Context
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.lerp
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.time.LocalDate

/**
 * 追剧日历页（独立页，入口在「我的」周历，见 `CalendarActivity`）。
 *
 * ## 冻结契约
 *
 * `CalendarPage(context: Context, initialDate: String?, onBack: () -> Unit)`
 * - [initialDate] = `yyyy-MM-dd`，缺省/非法一律回落「今天」；解析口径只在这里存在一次。
 * - 数据全部来自 C1 的 [CalendarSource]（**非 suspend**、内部不切线程），本页在
 *   `Dispatchers.IO` 里调用；`year`/`month` 变化即重取月网格与年热度，不跨月缓存
 *   （播放会改动数据源）。`CalendarSource.month()` 要求 `month ∈ 1..12`，此处恒在范围内。
 * - 表头固定「日 一 二 三 四 五 六」：C1 的 `leadingBlanks` 量纲是周日 = 0（与 `WeekStrip` 同口径）。
 *
 * ## 结构
 *
 * 顶栏 → 月/年胶囊 → 期间行 → 月网格（或年网格）→ 当日摘要卡；`hasRecords == false` 时
 * 只留顶栏 + 居中空态（不画网格，不给假数据）。
 *
 * 换月（箭头 / 年视图点月 / 「今天」）一律经 `focusMonth` 同步选中日：目标月是本月选今天，
 * 否则选 1 日 —— 否则摘要卡会停在旧月的某天，而网格已经换页。
 *
 * ## 尺寸对照（`profile-1.png` → 本页）
 *
 * | 维度 | 基准 | 实现 | 说明 |
 * | --- | --- | --- | --- |
 * | 页面左右边距 | 13dp | 13dp | `DesignTokens.screenPad`（子组件自带） |
 * | 模块间距 | 10dp | 10dp | 切换行/期间行/网格之间；网格到摘要 16dp |
 * | 摘要卡内边距 | 14dp | 14dp | 颜值门「卡片内边距 14dp」 |
 * | 摘要卡圆角 | 16dp | 16dp | 颜值门「卡片圆角 16dp」 |
 * | 日期标题 | — | 16sp Bold | 见 [DaySummaryCard] 的对照表 |
 *
 * ## 偏差
 *
 * - ⚠️ 期间行箭头：年视图切**年**、月视图切**月**（12 月「›」进位到次年 1 月），
 *   而不是两条箭头各管一件事——期间行只有一个文字位，混用量纲会让「›」跳到次年却显示旧年。
 * - ⚠️ 年份范围 `2000..今年+1`（任务卡要求），越界用 `coerceIn` 收口而不是抛异常；
 *   `CalendarSource` 自身允许 1970..9999，但 `WatchStore` 的 10 年回溯之外恒为 0，
 *   UI 收窄到 2000 起是刻意的可达范围，不是能力缺失。
 */
@Composable
fun CalendarPage(context: Context, initialDate: String?, onBack: () -> Unit) {
    val today = remember { LocalDate.now() }
    val todayKey = today.toString()
    val maxYear = today.year + YearLookAhead
    val seed = remember(initialDate) {
        initialDate?.let { runCatching { LocalDate.parse(it) }.getOrNull() } ?: today
    }

    var year by rememberSaveable { mutableIntStateOf(seed.year.coerceIn(MinYear, maxYear)) }
    var month by rememberSaveable { mutableIntStateOf(seed.monthValue) }
    var selectedDate by rememberSaveable { mutableStateOf(seed.toString()) }
    var yearView by rememberSaveable { mutableStateOf(false) }

    val hasRecords by produceState<Boolean?>(initialValue = null) {
        value = withContext(Dispatchers.IO) { CalendarSource.hasRecords(context) }
    }
    val monthGrid by produceState<CalendarMonthGrid?>(initialValue = null, year, month) {
        value = withContext(Dispatchers.IO) { CalendarSource.month(context, year, month) }
    }
    val yearMinutes by produceState<List<Int>?>(initialValue = null, year) {
        value = withContext(Dispatchers.IO) { CalendarSource.yearMinutes(context, year) }
    }

    // 换月必须把选中日带进新月份，否则摘要卡会停在旧月的某天而网格已换页。
    // 规则：目标月就是本月 → 选今天；否则选该月 1 日（今天不在的月份，「今天」没有意义）。
    val focusMonth: (Int, Int) -> Unit = { targetYear, targetMonth ->
        year = targetYear
        month = targetMonth
        selectedDate = if (targetYear == today.year && targetMonth == today.monthValue) {
            todayKey
        } else {
            LocalDate.of(targetYear, targetMonth, 1).toString()
        }
    }

    // 期间行箭头：年视图切年；月视图切月（12 月「›」进次年 1 月，1 月「‹」退上年 12 月）。
    val shiftPeriod: (Int) -> Unit = { delta ->
        if (yearView) {
            year = (year + delta).coerceIn(MinYear, maxYear)
        } else {
            val moved = LocalDate.of(year, month, 1).plusMonths(delta.toLong())
            val clamped = moved.year.coerceIn(MinYear, maxYear)
            val targetMonth = when {
                clamped == moved.year -> moved.monthValue
                clamped < moved.year -> 12
                else -> 1
            }
            focusMonth(clamped, targetMonth)
        }
    }
    val toToday: () -> Unit = {
        year = today.year
        month = today.monthValue
        selectedDate = todayKey
        yearView = false
    }

    Box(modifier = Modifier.fillMaxSize().background(CalendarPageBackdrop)) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .statusBarsPadding()
                .navigationBarsPadding(),
        ) {
            CalendarTopBar(onBack = onBack, onToday = toToday)
            if (hasRecords == false) {
                CalendarEmptyState(modifier = Modifier.fillMaxWidth().weight(1f))
                return@Column
            }
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .verticalScroll(rememberScrollState())
                    .padding(bottom = DesignTokens.gap.dp * 2),
            ) {
                CalendarViewSwitch(
                    yearView = yearView,
                    onSelectMonth = { yearView = false },
                    onSelectYear = { yearView = true },
                )
                Spacer(Modifier.height(DesignTokens.gap.dp))
                CalendarPeriodRow(
                    label = if (yearView) "${year}年" else "${year}年${month}月",
                    onPrev = { shiftPeriod(-1) },
                    onNext = { shiftPeriod(1) },
                )
                Spacer(Modifier.height(DesignTokens.gap.dp))
                if (yearView) {
                    yearMinutes?.let { minutes ->
                        CalendarYearGrid(
                            year = year,
                            minutesByMonth = minutes,
                            onSelectMonth = { picked ->
                                focusMonth(year, picked)
                                yearView = false
                            },
                        )
                        if (minutes.sum() == 0) {
                            PeriodEmptyHint("$year 年没有观看记录")
                        }
                    } ?: LoadingHint(modifier = Modifier.fillMaxWidth())
                } else {
                    monthGrid?.let { grid ->
                        CalendarMonthGrid(
                            grid = grid,
                            selectedDate = selectedDate,
                            todayKey = todayKey,
                            onSelect = { selectedDate = it },
                        )
                        if (grid.days.sumOf { it.minutes } == 0) {
                            PeriodEmptyHint("$month 月没有观看记录")
                        }
                    } ?: LoadingHint(modifier = Modifier.fillMaxWidth())
                    Spacer(Modifier.height(DaySummaryGap))
                    DaySummaryCard(
                        date = selectedDate,
                        mark = monthGrid?.days?.firstOrNull { it.date == selectedDate },
                    )
                }
            }
        }
    }
}

@Composable
private fun LoadingHint(modifier: Modifier = Modifier) {
    Text(
        text = "正在读取观看记录…",
        modifier = modifier.padding(vertical = DesignTokens.gap.dp * 2),
        fontSize = LoadingTextSize,
        color = DesignTokens.textSub,
        textAlign = TextAlign.Center,
    )
}

/** 某年/某月整段无记录时的提示：保留网格可浏览，只补一行说明（不伪装成空态整页）。 */
@Composable
private fun PeriodEmptyHint(text: String) {
    Text(
        text = text,
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp, vertical = DesignTokens.gap.dp),
        fontSize = LoadingTextSize,
        color = DesignTokens.textSub,
        textAlign = TextAlign.Center,
    )
}

/** 与「我的」页同款渐变：上缘紫灰、下缘回落 `pageBackground`（两色都由 token 派生）。 */
private val CalendarPageBackdrop: Brush = Brush.verticalGradient(
    listOf(
        lerp(DesignTokens.pageBackground, DesignTokens.bgLavender, BackdropLavenderMix),
        DesignTokens.pageBackground,
    ),
)

private const val MinYear = 2000
private const val YearLookAhead = 1
private const val BackdropLavenderMix = 0.62f
private val DaySummaryGap = 16.dp
private val LoadingTextSize = 13.sp

@Preview(showBackground = true, widthDp = 390, heightDp = 720)
@Composable
private fun CalendarPagePreview() {
    val days = (1..30).map { day ->
        val minutes = when (day) {
            4 -> 96
            11 -> 22
            17 -> 148
            else -> 0
        }
        CalendarDayMark(
            date = "2026-11-${day.toString().padStart(2, '0')}",
            minutes = minutes,
            titles = if (minutes > 0) listOf("漫长的季节", "繁花") else emptyList(),
        )
    }
    // 有数据态：真实 `CalendarPage` 需要 Context + IO，Preview 只拼可见部分（同款组件同款参数）
    Column(modifier = Modifier.fillMaxSize().background(CalendarPageBackdrop).statusBarsPadding()) {
        CalendarTopBar(onBack = {}, onToday = {})
        Column(Modifier.verticalScroll(rememberScrollState())) {
            CalendarViewSwitch(yearView = false, onSelectMonth = {}, onSelectYear = {})
            Spacer(Modifier.height(DesignTokens.gap.dp))
            CalendarPeriodRow(label = "2026年11月", onPrev = {}, onNext = {})
            Spacer(Modifier.height(DesignTokens.gap.dp))
            CalendarMonthGrid(
                grid = CalendarMonthGrid(year = 2026, month = 11, leadingBlanks = 0, days = days),
                selectedDate = "2026-11-17",
                todayKey = "2026-11-17",
                onSelect = {},
            )
            Spacer(Modifier.height(DaySummaryGap))
            DaySummaryCard(date = "2026-11-17", mark = days[16])
        }
    }
}
/** 空态：`CalendarSource.hasRecords == false` 时的整页样子（无网格、无摘要，只有顶栏 + 引导）。 */
@Preview(showBackground = true, widthDp = 390, heightDp = 720)
@Composable
private fun CalendarPageEmptyPreview() {
    Column(modifier = Modifier.fillMaxSize().background(CalendarPageBackdrop).statusBarsPadding()) {
        CalendarTopBar(onBack = {}, onToday = {})
        CalendarEmptyState(modifier = Modifier.fillMaxWidth().weight(1f))
    }
}