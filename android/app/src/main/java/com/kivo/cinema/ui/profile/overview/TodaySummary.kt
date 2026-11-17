package com.kivo.cinema.ui.profile.overview

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
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
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.profile.model.TodaySummary
import java.time.LocalDate
import java.time.format.DateTimeFormatter
import java.util.Locale
import kotlin.math.max

/**
 * 今日观看摘要卡（B2 版 `profile-1.png` ✅ 实拍复刻）。
 *
 * 版式与尺寸（✅ 基准截图量得）：白卡 `canvasFill` + `hairline` 描边 + 圆角 16，
 * 左右 13dp（实测 13.1）、内 14dp，**卡高 188.4dp**；首行「今天」16sp Bold + 右侧 `yyyy年M月d日` 13sp；
 * 第二行左侧 32sp Bold 小时数 + 13sp「小时」，右侧两行「N 部电影 / N 集剧」15sp 右对齐；
 * 底部 24 槽柱状（可用高 60dp、圆角 1dp、间隔均分）+ 11sp 横轴 00:00/12:00/24:00。
 *
 * 柱宽取**槽宽的 45%**（✅ 实测：槽 14.1dp、柱 6.2dp、间隙 7.8dp），而不是更细的固定宽度 ——
 * 基准截图里 24 条底线是全宽可见的，柱宽 2dp 会只剩一排细针，与基准看上去不是同一张图。
 *
 * 空数据口径：`hourBuckets` 全 0（或某槽为 0）时**不画柱**，只画 2dp 高灰底线 `groupedFill`
 * （✅ 实测底线 1.8dp）；小时与计数照实显示 0.0 / 0 部 / 0 集，不造数据。
 *
 * ◐ 推断：柱状图上限不出现刻度与数值标签，与基准截图一致（截图全 0 只可见灰底线）；
 * 单槽最大值按当日最大槽归一，因此柱高只表达当天内的相对分布，不跨日可比。
 *
 * 尺寸收口（2026-11-17 R6）：调整前卡高 192dp（基准 188.4dp，多 3.6dp），
 * 只改两个间距常量把它压回去 —— `HeadlineGap` 10 → 8（−2.0）、`BarsGap` 12 → 10.4（−1.6），
 * 合计 −3.6dp ⇒ **调整后 188.4dp**。字号、两行结构、柱状高 60dp、横轴 11sp 均未动；
 * 减的是纯空白，所以视觉密度与基准一致而不是"挤"。
 */
@Composable
fun TodaySummaryCard(summary: TodaySummary) {
    val dateText = remember { LocalDate.now().format(TodayDateFormatter) }
    val shape = RoundedCornerShape(CardRadius)
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp)
            .clip(shape)
            .background(DesignTokens.canvasFill)
            .border(CardStrokeWidth, DesignTokens.hairline, shape)
            .padding(CardPad),
    ) {
        Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            Text(
                text = "今天",
                modifier = Modifier.weight(1f),
                fontSize = HeaderTextSize,
                fontWeight = FontWeight.Bold,
                color = DesignTokens.textMain,
            )
            Text(text = dateText, fontSize = CaptionTextSize, color = DesignTokens.textSub)
        }
        Spacer(Modifier.height(HeadlineGap))
        Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            Row(modifier = Modifier.weight(1f), verticalAlignment = Alignment.Bottom) {
                Text(
                    text = formatHours(summary.hours),
                    fontSize = HoursTextSize,
                    fontWeight = FontWeight.Bold,
                    color = DesignTokens.textMain,
                )
                Text(
                    text = "小时",
                    modifier = Modifier.padding(start = UnitGap, bottom = UnitBaselinePad),
                    fontSize = CaptionTextSize,
                    color = DesignTokens.textSub,
                )
            }
            Column(horizontalAlignment = Alignment.End) {
                Text(
                    text = "${summary.movies} 部电影",
                    fontSize = CountTextSize,
                    color = DesignTokens.textMain,
                )
                Text(
                    text = "${summary.episodes} 集剧",
                    fontSize = CountTextSize,
                    color = DesignTokens.textMain,
                )
            }
        }
        Spacer(Modifier.height(BarsGap))
        HourBars(summary.hourBuckets)
        Spacer(Modifier.height(AxisGap))
        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
            HourAxisLabels.forEach { label ->
                Text(text = label, fontSize = AxisTextSize, color = DesignTokens.textSub)
            }
        }
    }
}

/** 24 槽柱状：固定 2dp 宽柱居中于等分槽内；0 值退化为 2dp 灰底线，其余按当日最大槽归一。 */
@Composable
private fun HourBars(buckets: List<Int>) {
    Canvas(
        Modifier
            .fillMaxWidth()
            .height(BarsHeight)
    ) {
        val slotWidth = size.width / HourSlots
        val barWidth = slotWidth * BarWidthRatio
        val maxValue = buckets.maxOrNull() ?: 0
        val baselineHeight = BaselineBarHeight.toPx()
        val baselineCorner = CornerRadius(VerticalCorner.toPx())
        repeat(HourSlots) { index ->
            val value = buckets.getOrElse(index) { 0 }
            val x = slotWidth * index + (slotWidth - barWidth) / 2f
            if (maxValue <= 0 || value <= 0) {
                drawRoundRect(
                    color = DesignTokens.groupedFill,
                    topLeft = Offset(x, size.height - baselineHeight),
                    size = Size(barWidth, baselineHeight),
                    cornerRadius = baselineCorner,
                )
            } else {
                val height = max(
                    baselineHeight,
                    (value / maxValue.toFloat()) * (size.height - ShapeInset.toPx()),
                )
                drawRoundRect(
                    color = DesignTokens.accentGreen,
                    topLeft = Offset(x, size.height - height),
                    size = Size(barWidth, height),
                    cornerRadius = CornerRadius(VerticalCorner.toPx()),
                )
            }
        }
    }
}

private val TodayDateFormatter = DateTimeFormatter.ofPattern("yyyy年M月d日", Locale.CHINA)

private fun formatHours(hours: Float): String = String.format(Locale.US, "%.1f", hours)

private const val HourSlots = 24
private val HourAxisLabels = listOf("00:00", "12:00", "24:00")

// 令牌无 16dp 卡圆角与 14dp 内距（radiusCard=13、screenPad=13 都不是这张卡的规格），故私有。
private val CardRadius = 16.dp
private val CardStrokeWidth = 0.5.dp
private val CardPad = 14.dp

// R6 尺寸收口：192 → 188.4 dp，两处间距合计 −3.6dp（见 KDoc）。
private val HeadlineGap = 8.dp
private val BarsGap = 10.4.dp
private val AxisGap = 4.dp
private val BarsHeight = 60.dp
private const val BarWidthRatio = 0.45f
private val BaselineBarHeight = 2.dp
private val VerticalCorner = 1.dp
private val ShapeInset = 2.dp
private val UnitGap = 3.dp
private val UnitBaselinePad = 4.dp
private val HoursTextSize = 32.sp
private val HeaderTextSize = 16.sp
private val CountTextSize = 15.sp
private val CaptionTextSize = 13.sp
private val AxisTextSize = 11.sp

@Preview(showBackground = true, widthDp = 390)
@Composable
private fun TodaySummaryCardPreview() {
    Column(
        modifier = Modifier
            .background(DesignTokens.pageBackground)
            .padding(vertical = DesignTokens.gap.dp),
        verticalArrangement = Arrangement.spacedBy(DesignTokens.gap.dp),
    ) {
        // 0 数据态：只画 24 条 2dp 灰底线
        TodaySummaryCard(TodaySummary(0f, 0, 0, List(HourSlots) { 0 }))
        // 有数据态：8–10 时各 6 个单位，其余为 0（0 槽仍画灰底线）
        TodaySummaryCard(
            TodaySummary(
                hours = 1.6f,
                movies = 1,
                episodes = 2,
                hourBuckets = List(HourSlots) { if (it in 8..10) 6 else 0 },
            )
        )
    }
}