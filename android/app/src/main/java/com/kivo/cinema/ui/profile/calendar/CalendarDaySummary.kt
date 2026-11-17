package com.kivo.cinema.ui.profile.calendar

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import java.time.LocalDate

/**
 * 选中日的摘要卡：日期 + 总分钟 + 最多 5 条片名。
 *
 * ## 冻结契约
 *
 * `DaySummaryCard(date: String, mark: CalendarDayMark?)` —— [date] 为 `yyyy-MM-dd`，
 * [mark] 从当月网格里按日期取；`null` 或 `minutes = 0` 都渲染「这天没有观看记录」，
 * 因为数据层无法区分「没看」与「看了 0 秒」（`CalendarDayMark.minutes` 的契约）。
 *
 * ## 尺寸对照（`profile-1.png` 「今天」卡 → 本卡）
 *
 * | 维度 | 基准 | 实现 | 说明 |
 * | --- | --- | --- | --- |
 * | 卡片圆角 | 13dp | 16dp | 颜值门要求卡片 16dp（基准图量得 13dp，按门禁口径放大） |
 * | 卡片内边距 | 14dp | 14dp | 颜值门「卡片内边距 14dp」 |
 * | 阴影 | 无（描边） | 2dp | 颜值门「卡片浮起来」；本页只有这一张卡，不形成灰面 |
 * | 日期标题 | 17sp | 16sp Bold | 颜值门「小节标题 16–20sp Bold」 |
 * | 分钟数 | — | 13sp `textSub` | 与正文同层，不抢标题 |
 * | 片名行 | 13sp | 13sp | 单行省略，行距 6dp |
 *
 * ## 偏差
 *
 * - ⚠️ 不显示「共 N 部」：`CalendarDayMark.titles` 已按最近观看去重且上限 5 条，
 *   由它推「部数」在超过 5 条时会少报，宁可不说。
 * - 分钟数写成「1 小时 24 分钟」而不是 `1.4 小时`：日历是逐日对账，整数分钟比小数小时可核对。
 */
@Composable
internal fun DaySummaryCard(date: String, mark: CalendarDayMark?) {
    val shape = RoundedCornerShape(SummaryCorner)
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp)
            .shadow(SummaryElevation, shape)
            .clip(shape)
            .background(DesignTokens.canvasFill)
            .padding(SummaryPad),
    ) {
        Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            Text(
                text = summaryTitle(date),
                fontSize = SummaryTitleSize,
                fontWeight = FontWeight.Bold,
                color = DesignTokens.textMain,
            )
            Spacer(Modifier.weight(1f))
            Text(
                text = minutesText(mark?.minutes ?: 0),
                fontSize = SummaryMetaSize,
                color = DesignTokens.textSub,
            )
        }
        Spacer(Modifier.height(SummaryTitleGap))
        val titles = mark?.titles.orEmpty().take(SummaryTitleLimit)
        if (titles.isEmpty()) {
            Text(text = NoRecordText, fontSize = SummaryBodySize, color = DesignTokens.textSub)
        } else {
            Column(verticalArrangement = Arrangement.spacedBy(SummaryLineGap)) {
                titles.forEach { title ->
                    Text(
                        text = title,
                        fontSize = SummaryBodySize,
                        color = DesignTokens.textMain,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis,
                    )
                }
            }
        }
    }
}

/** `2026-11-17` → `11月17日 周二`；非法输入原样回显，不抛异常。 */
private fun summaryTitle(date: String): String = runCatching {
    val day = LocalDate.parse(date)
    "${day.monthValue}月${day.dayOfMonth}日 ${WeekdayNames[day.dayOfWeek.value - 1]}"
}.getOrDefault(date)

/** `84` → `1 小时 24 分钟`；`45` → `45 分钟`；`0` → `0 分钟`（「没看」由标题下的文案说）。 */
private fun minutesText(minutes: Int): String {
    if (minutes <= 0) return "0 分钟"
    val hours = minutes / MinutesPerHour
    val rest = minutes % MinutesPerHour
    return when {
        hours > 0 && rest > 0 -> "$hours 小时 $rest 分钟"
        hours > 0 -> "$hours 小时"
        else -> "$rest 分钟"
    }
}

private const val NoRecordText = "这天没有观看记录"
private const val MinutesPerHour = 60
private const val SummaryTitleLimit = 5
private val WeekdayNames = listOf("周一", "周二", "周三", "周四", "周五", "周六", "周日")

private val SummaryCorner = 16.dp
private val SummaryElevation = 2.dp
private val SummaryPad = 14.dp
private val SummaryTitleGap = 8.dp
private val SummaryLineGap = 6.dp
private val SummaryTitleSize = 16.sp
private val SummaryMetaSize = 13.sp
private val SummaryBodySize = 13.sp

@Preview(showBackground = true, widthDp = 390)
@Composable
private fun DaySummaryCardPreview() {
    Column(
        modifier = Modifier.background(DesignTokens.pageBackground).padding(vertical = DesignTokens.gap.dp),
        verticalArrangement = Arrangement.spacedBy(DesignTokens.gap.dp),
    ) {
        // 有数据：两条片名 + 2 小时 28 分钟
        DaySummaryCard(
            date = "2026-11-17",
            mark = CalendarDayMark(
                date = "2026-11-17",
                minutes = 148,
                titles = listOf("漫长的季节", "繁花"),
            ),
        )
        // 无记录：片名行退化为一句提示
        DaySummaryCard(date = "2026-11-18", mark = null)
    }
}