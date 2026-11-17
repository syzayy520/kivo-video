package com.kivo.cinema.ui.profile.charts

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
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
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.profile.model.MonthlyPoint
import kotlin.math.abs
import kotlin.math.round

/**
 * 本月观看折线卡（B2 冻结契约 `MonthlyChart(label, points, hours, delta)`）。
 *
 * 尺寸全部来自基准截图 `docs/audit/golden/kivo/profile-3.png` 的像素实测
 * （K40 1080×2400、密度 2.75 ⇒ dp = px / 2.75）：
 *
 * | 项 | 基准实测 | 本实现 |
 * | --- | --- | --- |
 * | 卡片 | 1008×593 px = 366.5×215.6 dp，左右外边距 13 dp | 宽随父、高 216 dp（19+20+6+32+8+92+4+16+19） |
 * | 标题 | 「本月观看」字形高 42 px ⇒ 16 sp Bold；左边距 17 dp、上边距 19 dp | 16 sp / 20 sp 行高 |
 * | 右上「{label}」 | 字形 29 px ⇒ 13 sp，右内边距 18 dp | 13 sp textSub |
 * | 大数字 | 字形高 58 px（cap 0.72）⇒ 29 sp | 28 sp Bold（与「小时」底对齐） |
 * | 「小时」「↑{delta}小时」 | 字形 31 px ⇒ 13 sp | 13 sp；增量绿 / 下降红 / 零灰 |
 * | 折线 | 基线 y=1335、峰值 y=1150 ⇒ 纵向 67 dp；线宽 2 dp；每个采样点 4 dp 实心圆 | 92 dp 画布 + 2 dp 线 + 2 dp 半径圆点 |
 * | 横轴 | 3 个刻度（1 / 15 / 30），字形 21 px ⇒ 12 sp | 12 sp，首/中/末日 |
 *
 * 取舍（◐ 推断，已显式记录）：
 * - 基准**没有**网格线，任务口径要求「横轴 3 刻度和网格线」⇒ 在 3 个刻度处画 0.75 dp
 *   `hairline`（黑 5%），肉眼几乎不可见，只在放大对比时存在，不改变基准观感；
 * - 基准每个采样点都有 4 dp 圆点（不只是末端），任务只写了「末端圆点」；
 *   两处并存时按基准全点绘制 —— 只画末端会让日级折线丢失采样密度；
 * - `points` 的横轴按 `day` 线性映射（首末 day 为端点），不是按下标等距：
 *   这样「15」刻度与峰值位置天然对齐（基准峰值正好落在 15 的刻度上）；
 * - 空数据：不画折线，只留 `groupedFill` 底线 + 1/15/30 刻度，不伪造曲线。
 */
@Composable
fun MonthlyChart(label: String, points: List<MonthlyPoint>, hours: Float, delta: Float) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp)
            .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
            .background(DesignTokens.canvasFill)
            .padding(horizontal = CardSidePad, vertical = CardVerticalPad),
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.Top,
        ) {
            Text(
                text = "本月观看",
                fontSize = TitleSize,
                lineHeight = TitleLine,
                fontWeight = FontWeight.Bold,
                color = DesignTokens.textMain,
            )
            Text(text = label, fontSize = MetaSize, lineHeight = MetaLine, color = DesignTokens.textSub)
        }
        Spacer(Modifier.height(TitleGap))
        Row(verticalAlignment = Alignment.Bottom) {
            Text(
                text = chartHoursText(hours),
                fontSize = BigSize,
                lineHeight = BigLine,
                fontWeight = FontWeight.Bold,
                color = DesignTokens.textMain,
            )
            Spacer(Modifier.width(UnitGap))
            Text(
                text = "小时",
                fontSize = MetaSize,
                lineHeight = MetaLine,
                color = DesignTokens.textSub,
                modifier = Modifier.padding(bottom = MetaBaselinePad),
            )
            Spacer(Modifier.width(DeltaGap))
            Text(
                text = deltaText(delta),
                fontSize = MetaSize,
                lineHeight = MetaLine,
                color = deltaColor(delta),
                modifier = Modifier.padding(bottom = MetaBaselinePad),
            )
        }
        Spacer(Modifier.height(ChartTopGap))
        MonthLine(points = points, modifier = Modifier.fillMaxWidth().height(ChartHeight))
        Spacer(Modifier.height(AxisGap))
        AxisLabels(points)
    }
}

/** 折线画布：3 条淡网格线 + `accentGreen` 2 dp 折线与 4 dp 采样点。 */
@Composable
private fun MonthLine(points: List<MonthlyPoint>, modifier: Modifier) {
    Canvas(modifier) {
        val width = size.width
        val height = size.height
        val dot = DotRadius.toPx()
        val stroke = LineStroke.toPx()
        val top = dot
        val bottom = height - dot

        GridFractions.forEach { fraction ->
            val x = width * fraction
            drawLine(DesignTokens.hairline, Offset(x, 0f), Offset(x, height), GridStroke.toPx())
        }

        val samples = points.filter { it.day > 0 }.sortedBy { it.day }
        if (samples.isEmpty()) {
            drawLine(DesignTokens.groupedFill, Offset(0f, bottom), Offset(width, bottom), stroke)
            return@Canvas
        }

        val peak = samples.maxOf { it.minutes }.coerceAtLeast(0)
        fun yOf(minutes: Int): Float =
            if (peak <= 0) bottom else bottom - (minutes.coerceAtLeast(0).toFloat() / peak) * (bottom - top)

        val firstDay = samples.first().day
        val span = (samples.last().day - firstDay).coerceAtLeast(1)
        fun xOf(day: Int): Float =
            if (samples.size == 1) width / 2f else ((day - firstDay).toFloat() / span) * (width - 2 * dot) + dot

        val path = Path()
        samples.forEachIndexed { index, sample ->
            val x = xOf(sample.day)
            val y = yOf(sample.minutes)
            if (index == 0) path.moveTo(x, y) else path.lineTo(x, y)
        }
        drawPath(
            path = path,
            color = DesignTokens.accentGreen,
            style = Stroke(width = stroke, cap = StrokeCap.Round, join = StrokeJoin.Round),
        )
        samples.forEach { sample ->
            drawCircle(DesignTokens.accentGreen, dot, Offset(xOf(sample.day), yOf(sample.minutes)))
        }
    }
}

/** 横轴三刻度：首日 / 中日 / 末日；空数据回落到 1 / 15 / 30（基准的固定刻度）。 */
@Composable
private fun AxisLabels(points: List<MonthlyPoint>) {
    val sorted = points.filter { it.day > 0 }.sortedBy { it.day }
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
    ) {
        AxisTick(sorted.firstOrNull()?.day ?: FirstDay)
        AxisTick(sorted.getOrNull(sorted.size / 2)?.day ?: MidDay)
        AxisTick(sorted.lastOrNull()?.day ?: LastDay)
    }
}

@Composable
private fun AxisTick(day: Int) {
    Text(text = day.toString(), fontSize = AxisSize, lineHeight = AxisLine, color = DesignTokens.textSub)
}

private fun deltaText(delta: Float): String = when {
    delta > 0f -> "↑${chartHoursText(delta)}小时"
    delta < 0f -> "↓${chartHoursText(abs(delta))}小时"
    else -> "0小时"
}

private fun deltaColor(delta: Float): Color = when {
    delta > 0f -> DesignTokens.accentGreen
    delta < 0f -> DesignTokens.dangerRed
    else -> DesignTokens.textSub
}

/**
 * 图表区统一的小时格式化（一位小数、去尾随 .0），与基准的 `1.7 小时` / `1.7h` 同口径。
 *
 * 放在本文件因为 `MonthlyChart` 是第一个消费方；`TopShowsCard` / `GenreDonut` / `TrendCard`
 * 共用同一份实现，避免四份相同的小数格式化在图表区漂移（治理审计关注「重复实现」）。
 */
internal fun chartHoursText(value: Float): String {
    val tenths = round(value * 10f).toInt()
    val magnitude = abs(tenths)
    val whole = magnitude / 10
    val fraction = magnitude % 10
    val sign = if (tenths < 0) "-" else ""
    return if (fraction == 0) "$sign$whole" else "$sign$whole.$fraction"
}

private val CardSidePad = 17.dp
private val CardVerticalPad = 19.dp
private val TitleGap = 6.dp
private val UnitGap = 4.dp
private val DeltaGap = 10.dp
private val MetaBaselinePad = 3.dp
private val ChartTopGap = 8.dp
private val ChartHeight = 92.dp
private val AxisGap = 4.dp
private val DotRadius = 2.dp
private val LineStroke = 2.dp
private val GridStroke = 0.75.dp
private val TitleSize = 16.sp
private val TitleLine = 20.sp
private val MetaSize = 13.sp
private val MetaLine = 16.sp
private val BigSize = 28.sp
private val BigLine = 32.sp
private val AxisSize = 12.sp
private val AxisLine = 16.sp
private val GridFractions = listOf(0f, 0.5f, 1f)
private const val FirstDay = 1
private const val MidDay = 15
private const val LastDay = 30

@Preview(showBackground = true, widthDp = 393)
@Composable
private fun MonthlyChartPreview() {
    Column(
        modifier = Modifier.background(DesignTokens.pageBackground).padding(vertical = DesignTokens.gap.dp),
        verticalArrangement = Arrangement.spacedBy(DesignTokens.gap.dp),
    ) {
        MonthlyChart(
            label = "11月",
            points = List(30) { index -> MonthlyPoint(day = index + 1, minutes = if (index == 14) 102 else 0) },
            hours = 1.7f,
            delta = 1.7f,
        )
        MonthlyChart(label = "11月", points = emptyList(), hours = 0f, delta = -0.3f)
    }
}