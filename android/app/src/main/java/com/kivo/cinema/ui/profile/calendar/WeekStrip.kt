package com.kivo.cinema.ui.profile.calendar

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.RowScope
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.profile.model.CalendarDay

/**
 * 周日历条（B2 版 `profile-1.png` ✅ 实拍复刻）：7 列等宽，「日期数字 + 星期标签 + 播出点」。
 *
 * 尺寸口径（✅ 基准截图量得，390dp 逻辑宽下逐项对齐）：
 * - 整条水平内边距 13dp、行高 56dp；✅ 实测：列宽 52.4dp、胶囊 48×44dp（列内左右各缩 2dp）、
 *   点直径 4dp 且距胶囊底 2dp；
 * - 日期 15sp、星期 11sp；选中 = 白胶囊（`canvasFill`，44dp 高 / 圆角 12 / 2dp 轻阴影），
 *   日期与星期同为 `dangerRed`，胶囊下方 4dp 红点；
 * - 未选中 = `textSub`；今日且未选中 = 日期加粗 `textMain`（◐ 推断：基准截图里"今天"恰好
 *   就是选中日，没有"今日未选中"的实拍样本；不加粗又会让今天在未选中态彻底消失）；
 * - `hasAiring` 的未选中日在同一位置画 4dp `accentGreen` 点，无播出则留空占位，
 *   保证七列基线一致（点行高度固定，不因有无播出跳动）。
 *
 * 空数据（`days` 为空）渲染一行居中的「暂无日历」，不伪造七天。
 */
@Composable
fun WeekStrip(days: List<CalendarDay>, selected: CalendarDay?, onSelect: (CalendarDay) -> Unit) {
    if (days.isEmpty()) {
        Text(
            text = "暂无日历",
            modifier = Modifier
                .fillMaxWidth()
                .padding(vertical = DesignTokens.gap.dp),
            fontSize = LabelTextSize,
            color = DesignTokens.textSub,
            textAlign = TextAlign.Center,
        )
        return
    }
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(RowHeight)
            .padding(horizontal = DesignTokens.screenPad.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        days.forEach { day ->
            DayColumn(
                day = day,
                selected = day == selected,
                onClick = { onSelect(day) },
            )
        }
    }
}

/** 单列：白胶囊（仅选中态）+ 下方 4dp 指示点。整列可点，命中区随列宽伸展。 */
@Composable
private fun RowScope.DayColumn(day: CalendarDay, selected: Boolean, onClick: () -> Unit) {
    val capsuleShape = RoundedCornerShape(PillCorner)
    Column(
        modifier = Modifier
            .weight(1f)
            .clip(capsuleShape)
            .clickable(role = Role.Button, onClick = onClick),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
    ) {
        Column(
            modifier = Modifier
                .padding(horizontal = PillSideInset)
                .fillMaxWidth()
                .then(if (selected) Modifier.shadow(PillElevation, capsuleShape) else Modifier)
                .clip(capsuleShape)
                .then(if (selected) Modifier.background(DesignTokens.canvasFill) else Modifier)
                .height(PillHeight),
            verticalArrangement = Arrangement.Center,
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            Text(
                text = dayOfMonth(day.date),
                fontSize = DateTextSize,
                fontWeight = if (selected || day.isToday) FontWeight.Bold else FontWeight.Normal,
                color = when {
                    selected -> DesignTokens.dangerRed
                    day.isToday -> DesignTokens.textMain
                    else -> DesignTokens.textSub
                },
            )
            Spacer(Modifier.height(LabelsGap))
            Text(
                text = day.label,
                fontSize = LabelTextSize,
                fontWeight = if (selected) FontWeight.Medium else FontWeight.Normal,
                color = if (selected) DesignTokens.dangerRed else DesignTokens.textSub,
            )
        }
        Spacer(Modifier.height(DotGap))
        Box(Modifier.height(DotSize), contentAlignment = Alignment.Center) {
            if (selected || day.hasAiring) {
                Box(
                    Modifier
                        .size(DotSize)
                        .clip(CircleShape)
                        .background(
                            if (selected) DesignTokens.dangerRed else DesignTokens.accentGreen
                        )
                )
            }
        }
    }
}

/** `date` 契约是 `yyyy-MM-dd`；只取日，不解析日期对象（B2-T1 明确禁止 LocalDate）。 */
private fun dayOfMonth(date: String): String = date.substringAfterLast('-')

// 令牌里没有日历专用尺寸（分隔卡片的 13/16 圆角与字号层级都不是它），故落文件私有常量。
private val RowHeight = 56.dp
private val PillHeight = 44.dp
private val PillCorner = 12.dp
private val PillElevation = 2.dp
private val PillSideInset = 2.dp
private val LabelsGap = 2.dp
private val DotGap = 2.dp
private val DotSize = 4.dp
private val DateTextSize = 15.sp
private val LabelTextSize = 11.sp

@Preview(showBackground = true, widthDp = 390)
@Composable
private fun WeekStripPreview() {
    Column(
        modifier = Modifier
            .background(DesignTokens.pageBackground)
            .padding(vertical = DesignTokens.gap.dp),
        verticalArrangement = Arrangement.spacedBy(DesignTokens.gap.dp),
    ) {
        // 0 数据态
        WeekStrip(days = emptyList(), selected = null, onSelect = {})
        // 有数据态：选中「17 二」、今日「18 三」（未选中）、「20 五」有播出
        WeekStrip(days = PreviewDays, selected = PreviewDays[2], onSelect = {})
    }
}

private val PreviewDays = listOf(
    CalendarDay("日", "2026-11-15", hasAiring = false, isToday = false),
    CalendarDay("一", "2026-11-16", hasAiring = false, isToday = false),
    CalendarDay("二", "2026-11-17", hasAiring = false, isToday = false),
    CalendarDay("三", "2026-11-18", hasAiring = false, isToday = true),
    CalendarDay("四", "2026-11-19", hasAiring = false, isToday = false),
    CalendarDay("五", "2026-11-20", hasAiring = true, isToday = false),
    CalendarDay("六", "2026-11-21", hasAiring = false, isToday = false),
)