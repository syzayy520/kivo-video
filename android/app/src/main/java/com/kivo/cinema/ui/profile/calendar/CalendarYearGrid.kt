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
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 年视图：3 列 × 4 行，每月一格。
 *
 * ## 冻结契约
 *
 * `CalendarYearGrid(year: Int, minutesByMonth: List<Int>, onSelectMonth: (Int) -> Unit)` ——
 * [minutesByMonth] 来自 C1 的 `CalendarSource.yearMinutes()`，索引 = `month - 1`（12 项，
 * 不足按 0 处理）。点击某月只回调 `1..12`，不负责切页（页面自己把月视图置为选中月）。
 *
 * ## 尺寸对照
 *
 * | 维度 | 基准 | 实现 | 说明 |
 * | --- | --- | --- | --- |
 * | 格高 | — | 72dp | 任务卡指定；与月视图单格 44dp 形成层级 |
 * | 列数 / 行数 | — | 3 × 4 | 12 个月恰好两屏内一屏 |
 * | 圆角 | 13dp（`radiusCard`） | 12dp | 与月视图格、周历胶囊统一 12dp |
 * | 月份名 | — | 13sp | 与 `DesignTokens.fontPosterDate` 同级（刻度 13） |
 * | 格间距 | 10dp（`DesignTokens.gap`） | 10dp | 与页面模块间距一致 |
 * | 左右边距 | 13dp | 13dp | `DesignTokens.screenPad` |
 *
 * ## 偏差与实测理由
 *
 * - ⚠️ 「12 格微热力条」按**该月总分钟**统一给强度，不是逐日热力：`yearMinutes` 只有月度
 *   合计，画逐日热力就必须让 C1 再暴露日矩阵（当前契约没有），此处不编造日粒度。
 *   12 根条是**刻度尺**：让「有没有看、看了多少」在同一格里可比较，且空月仍占位不塌陷。
 * - 无记录月画 `hairline` 色的 12 根底条（与热力图的空格同色），不从版面上消失。
 * - 卡片用 1dp `hairline` 描边而不是阴影：12 张卡片同时投影会让整屏发灰，
 *   `profile-1.png` 的热力图卡也是描边优先。
 */
@Composable
fun CalendarYearGrid(
    year: Int,
    minutesByMonth: List<Int>,
    onSelectMonth: (Int) -> Unit,
) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp),
        verticalArrangement = Arrangement.spacedBy(DesignTokens.gap.dp),
    ) {
        for (row in 0 until MonthRows) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(DesignTokens.gap.dp),
            ) {
                for (col in 0 until MonthColumns) {
                    val month = row * MonthColumns + col + 1
                    MonthCard(
                        month = month,
                        minutes = minutesByMonth.getOrElse(month - 1) { 0 },
                        modifier = Modifier.weight(1f),
                        onSelect = onSelectMonth,
                    )
                }
            }
        }
    }
}

/** 单月卡：月份名 + 12 根微热力条。整格可点（命中 ≥ 72dp 高 × 1/3 屏宽）。 */
@Composable
private fun MonthCard(
    month: Int,
    minutes: Int,
    modifier: Modifier = Modifier,
    onSelect: (Int) -> Unit,
) {
    val shape = RoundedCornerShape(CardCorner)
    Column(
        modifier = modifier
            .height(CardHeight)
            .clip(shape)
            .background(DesignTokens.canvasFill)
            .border(Hairline, DesignTokens.hairline, shape)
            .clickable(role = Role.Button) { onSelect(month) }
            .padding(horizontal = CardSidePad, vertical = CardVerticalPad),
        verticalArrangement = Arrangement.SpaceBetween,
    ) {
        Text(
            text = monthLabel(month),
            fontSize = MonthTextSize,
            fontWeight = if (minutes > 0) FontWeight.Bold else FontWeight.Medium,
            color = if (minutes > 0) DesignTokens.textMain else DesignTokens.textSub,
        )
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(TickGap),
            verticalAlignment = Alignment.Bottom,
        ) {
            repeat(TickCount) {
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .height(TickHeight)
                        .clip(RoundedCornerShape(TickCorner))
                        .background(tickColor(minutes)),
                )
            }
        }
    }
}

/** 月份名用「1月」而不是英文缩写：应用其余页面没有本地化层，中文标签最短且无需翻译。 */
private fun monthLabel(month: Int): String = "${month}月"

/** 三档强度（与月视图绿点同一阈值）：满绿 / 66% / 38%；无记录用 hairline 占位。 */
private fun tickColor(minutes: Int) = when {
    minutes >= HeavyMonthMinutes -> DesignTokens.accentGreen
    minutes > 0 -> DesignTokens.accentGreen.copy(alpha = if (minutes >= MediumMonthMinutes) 0.66f else 0.38f)
    else -> DesignTokens.hairline
}

private const val MonthColumns = 3
private const val MonthRows = 4
private const val TickCount = 12
private const val HeavyMonthMinutes = 12 * 60
private const val MediumMonthMinutes = 3 * 60

private val CardHeight = 72.dp
private val CardCorner = 12.dp
private val CardSidePad = 12.dp
private val CardVerticalPad = 11.dp
private val Hairline = 1.dp
private val MonthTextSize = 13.sp
private val TickHeight = 8.dp
private val TickGap = 2.dp
private val TickCorner = 2.dp

@Preview(showBackground = true, widthDp = 390)
@Composable
private fun CalendarYearGridPreview() {
    Column(
        modifier = Modifier.background(DesignTokens.pageBackground).padding(vertical = DesignTokens.gap.dp),
        verticalArrangement = Arrangement.spacedBy(DesignTokens.gap.dp),
    ) {
        // 有数据态：春季与暑期追得多，其余为空月
        CalendarYearGrid(
            year = 2026,
            minutesByMonth = listOf(360, 0, 820, 430, 60, 0, 1500, 1180, 240, 0, 0, 96),
            onSelectMonth = {},
        )
        // 全空年：12 张卡照画，微热力条只剩 hairline 底
        CalendarYearGrid(year = 2026, minutesByMonth = List(12) { 0 }, onSelectMonth = {})
    }
}