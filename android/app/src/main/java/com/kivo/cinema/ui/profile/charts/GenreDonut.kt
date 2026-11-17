package com.kivo.cinema.ui.profile.charts

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.profile.model.GenreSlice

/**
 * 类型分布环（B2 冻结契约 `GenreDonut(slices, totalHours, movieCount, episodeCount)`）。
 *
 * 基准实测（`profile-4.png`，密度 2.75，空态）：
 * - 空态卡 1008×348 px = 366.5×126.5 dp；环外径 264 px = 96 dp、环带 38 px = 13.8 dp；
 * - 环与卡同中心（卡 y 1596–1943，环心 y 1769）⇒ 环上下各留 15 dp，**基准卡内没有标题行**；
 * - 环心「1.7h」字形 35 px ⇒ 18 sp；右侧「暂无数据」字形 32 px ⇒ 13 sp，距环右沿 17.5 dp。
 *
 * 尺寸收口（2026-11-17 R6，与基准 1:1）：
 * - **删除内建「类型分布」标题行**：基准里标题在卡片外，现由 `ProfilePage` 的
 *   `SectionTitle("类型分布")` 提供（key `genre-title`），本卡不再自绘标题；
 * - 环 84 → 96 dp、描边 12 → 13.8 dp（此前为容纳标题按比例缩小，标题外移后恢复基准值）；
 * - 垂直内距 15.25 dp × 2 + 环 96 dp = **126.5 dp**，与基准一致；左右内距 17 → 14 dp；
 * - ⚠️ 任务口径写「内边距 14 dp + 环 96 dp」，但 14×2 + 96 = 124 dp（差 2.5 dp）——
 *   本卡以**卡高**为准（基准与尺寸门禁都看卡高），故上下取 15.25 dp 而非 14 dp；
 * - 底部「N 部电影 · M 部剧」仅在**有数据**时出现：出现时卡高 126.5 + 6(间距) + 15(行高)
 *   = 147.5 dp；基准空态不渲染此行，因此不计入尺寸门禁。
 *
 * 数据取舍：
 * - 只取 `percent > 0` 的前 6 段（契约 4–6 段）；少于 4 段照画，不补假段；
 * - 4 档色板全部由 `accentGreen` 透明度派生（1 / .75 / .5 / .3），索引循环；
 * - 空态画一整圈 `groupedFill` + 右侧「暂无数据」，环心仍显示真实总时长（0 -> 「0h」）；
 * - `movieCount` / `episodeCount` 同为 null **或**同为 0 时整行不渲染：
 *   0 与「没有分类计数」在 UI 上不可区分，基准空态（1.7h、无分类计数）也不渲染该行；
 *   只有一个为 null 时按 0 处理，不隐藏另一侧的真实计数。
 */
@Composable
fun GenreDonut(
    slices: List<GenreSlice>,
    totalHours: Float,
    movieCount: Int?,
    episodeCount: Int?,
) {
    val visible = slices.filter { it.percent > 0 }.take(MaxSlices)
    val showCounts = !(movieCount == null && episodeCount == null) && !(movieCount == 0 && episodeCount == 0)

    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp)
            .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
            .background(DesignTokens.canvasFill)
            .padding(horizontal = CardSidePad, vertical = CardVerticalPad),
    ) {

        Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            DonutRing(visible, totalHours, Modifier.size(RingSize))
            Spacer(Modifier.width(RingGap))
            if (visible.isEmpty()) {
                Text(
                    text = "暂无数据",
                    fontSize = MetaSize,
                    lineHeight = MetaLine,
                    color = DesignTokens.textSub,
                )
            } else {
                Legend(visible, Modifier.weight(1f))
            }
        }
        if (showCounts) {
            Spacer(Modifier.height(CountsGap))
            Text(
                text = "${movieCount ?: 0} 部电影 · ${episodeCount ?: 0} 部剧",
                fontSize = CountsSize,
                lineHeight = CountsLine,
                color = DesignTokens.textSub,
            )
        }
    }
}

/** 环形本体：有数据按占比分段，空数据整圈灰；环心恒显示总时长。 */
@Composable
private fun DonutRing(slices: List<GenreSlice>, totalHours: Float, modifier: Modifier) {
    Box(modifier = modifier, contentAlignment = Alignment.Center) {
        Canvas(Modifier.fillMaxSize()) {
            val stroke = RingStroke.toPx()
            val topLeft = Offset(stroke / 2f, stroke / 2f)
            val arcSize = Size(size.width - stroke, size.height - stroke)
            val total = slices.sumOf { it.percent }
            if (total <= 0) {
                drawArc(
                    color = DesignTokens.groupedFill,
                    startAngle = StartAngle,
                    sweepAngle = FullCircle,
                    useCenter = false,
                    topLeft = topLeft,
                    size = arcSize,
                    style = Stroke(width = stroke, cap = StrokeCap.Butt),
                )
            } else {
                var start = StartAngle
                slices.forEachIndexed { index, slice ->
                    val sweep = FullCircle * slice.percent / total
                    drawArc(
                        color = ringColor(index),
                        startAngle = start,
                        sweepAngle = sweep,
                        useCenter = false,
                        topLeft = topLeft,
                        size = arcSize,
                        style = Stroke(width = stroke, cap = StrokeCap.Butt),
                    )
                    start += sweep
                }
            }
        }
        Text(
            text = "${chartHoursText(totalHours)}h",
            fontSize = CenterSize,
            lineHeight = CenterLine,
            fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
        )
    }
}

/** 右侧图例：色点 + 名称 + 百分比。 */
@Composable
private fun Legend(slices: List<GenreSlice>, modifier: Modifier) {
    Column(modifier = modifier, verticalArrangement = Arrangement.spacedBy(LegendGap)) {
        slices.forEachIndexed { index, slice ->
            Row(verticalAlignment = Alignment.CenterVertically) {
                Box(
                    modifier = Modifier
                        .size(LegendDot)
                        .clip(CircleShape)
                        .background(ringColor(index)),
                )
                Spacer(Modifier.width(LegendDotGap))
                Text(
                    text = slice.name,
                    modifier = Modifier.weight(1f),
                    fontSize = MetaSize,
                    lineHeight = MetaLine,
                    color = DesignTokens.textMain,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )
                Text(
                    text = "${slice.percent}%",
                    fontSize = MetaSize,
                    lineHeight = MetaLine,
                    color = DesignTokens.textSub,
                )
            }
        }
    }
}

/** 4 档同色相色板，索引循环（契约允许 4–6 段）。 */
private fun ringColor(index: Int): Color = when (index % PaletteSize) {
    0 -> DesignTokens.accentGreen
    1 -> DesignTokens.accentGreen.copy(alpha = 0.75f)
    2 -> DesignTokens.accentGreen.copy(alpha = 0.5f)
    else -> DesignTokens.accentGreen.copy(alpha = 0.3f)
}

private val CardSidePad = 14.dp

// 15.25 × 2 + 环 96 = 126.5 dp（基准卡高）；标题已外移到 ProfilePage，见 KDoc。
private val CardVerticalPad = 15.25.dp
private val RingSize = 96.dp
private val RingStroke = 13.8.dp
private val RingGap = 16.dp
private val CountsGap = 6.dp
private val LegendGap = 6.dp
private val LegendDot = 8.dp
private val LegendDotGap = 6.dp
private val MetaSize = 13.sp
private val MetaLine = 16.sp
private val CenterSize = 18.sp
private val CenterLine = 22.sp
private val CountsSize = 12.sp
private val CountsLine = 15.sp
private const val PaletteSize = 4
private const val MaxSlices = 6
private const val FullCircle = 360f
private const val StartAngle = -90f

@Preview(showBackground = true, widthDp = 393)
@Composable
private fun GenreDonutPreview() {
    Column(
        modifier = Modifier.background(DesignTokens.pageBackground).padding(vertical = DesignTokens.gap.dp),
        verticalArrangement = Arrangement.spacedBy(DesignTokens.gap.dp),
    ) {
        GenreDonut(slices = emptyList(), totalHours = 1.7f, movieCount = null, episodeCount = null)
        GenreDonut(
            slices = listOf(
                GenreSlice(name = "剧情", percent = 42),
                GenreSlice(name = "喜剧", percent = 26),
                GenreSlice(name = "科幻", percent = 18),
                GenreSlice(name = "纪录片", percent = 14),
            ),
            totalHours = 12.4f,
            movieCount = 3,
            episodeCount = 5,
        )
    }
}