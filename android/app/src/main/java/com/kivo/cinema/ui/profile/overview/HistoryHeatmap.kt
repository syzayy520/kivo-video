package com.kivo.cinema.ui.profile.overview

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.profile.model.HistoryCell

/**
 * 「历史观看」热力图**白卡**（R2 复刻，基准 `docs/audit/golden/kivo/profile-1.png` 历史区）。
 *
 * 只画卡，不画节标题：「历史观看」16sp Bold 由编排层（`ProfilePage` 的 `history-title` item）
 * 提供，间距由 LazyColumn 的 10dp 承担 —— 与基准「标题字形底 → 卡顶 12.7dp」吻合。
 * 这样标题在同屏只出现一次（本组件若自带标题会和编排层重复）。
 *
 * 版式（✅ 真机 1080×2400 ÷ 2.75 逐像素实测）：
 * - 白卡左右各 13dp（36px）、卡内 14dp（39px）、圆角 16dp（44px）、1dp hairline 描边；
 * - 20 列 × 7 行，格 14dp（39px）、间距 3dp（8px）、格圆角 3dp；
 * - 网格总宽 337dp、总高 116dp，卡高 144dp（398px）。
 *
 * 偏差（有意，以基准为准）：任务书写「格 13dp / 卡内 12dp」，按 13dp 复算网格总宽只有 317dp，
 * 比基准窄 20dp（超质量门 ±8dp），故取实测 14dp；改成 14dp 后列数、格距、卡高三项同时吻合。
 *
 * 数据口径：缺失的 (week, day) 按 level 0 画 —— 基准整块网格没有空洞，浅灰格本身即
 * 「这天没有观看」；只有 level ≥ 1 才是绿格。颜色全部来自令牌：
 * level 0 = [DesignTokens.groupedFill]（实测 #F2F2F7），1–4 = [DesignTokens.accentGreen] 25/45/70/100%。
 *
 * 未覆盖：横轴日期标签、图例、点击下钻（基准三者都没有，冻结签名也不带回调）。
 */
@Composable
fun HistoryHeatmap(cells: List<HistoryCell>, weeks: Int = 20) {
    BoxWithConstraints(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp)
            .clip(CardShape)
            .background(DesignTokens.canvasFill)
            .border(HairlineWidth, DesignTokens.hairline, CardShape)
            .padding(CardPad),
    ) {
        val columns = weeks.coerceAtLeast(1)
        // 卡内可用宽按列均分：基准 338dp 宽扣 19 个 3dp 间距后正好 14dp，故 14dp 为上限。
        val cell = ((maxWidth - CellGap * (columns - 1)) / columns).coerceIn(MinCell, MaxCell)
        val gridHeight = cell * RowsPerWeek + CellGap * (RowsPerWeek - 1)
        val levels = remember(cells) { cells.associate { (it.week to it.day) to it.level } }

        Canvas(Modifier.fillMaxWidth().height(gridHeight)) {
            val cellPx = cell.toPx()
            val gapPx = CellGap.toPx()
            val radius = CornerRadius(CellCorner.toPx())
            val step = cellPx + gapPx
            for (column in 0 until columns) {
                for (row in 0 until RowsPerWeek) {
                    drawRoundRect(
                        color = levelFill(levels[column to row] ?: NoLevel),
                        topLeft = Offset(column * step, row * step),
                        size = Size(cellPx, cellPx),
                        cornerRadius = radius,
                    )
                }
            }
        }
    }
}

/** level → 填充色；0 为浅灰底，1–4 逐级加深，越界值夹到 0..4（不引入第五档）。 */
private fun levelFill(level: Int): Color = when (level.coerceIn(NoLevel, TopLevel)) {
    NoLevel -> DesignTokens.groupedFill
    1 -> DesignTokens.accentGreen.copy(alpha = Alpha1)
    2 -> DesignTokens.accentGreen.copy(alpha = Alpha2)
    3 -> DesignTokens.accentGreen.copy(alpha = Alpha3)
    else -> DesignTokens.accentGreen
}

private const val NoLevel = 0
private const val TopLevel = 4
private const val RowsPerWeek = 7
private const val Alpha1 = 0.25f
private const val Alpha2 = 0.45f
private const val Alpha3 = 0.7f

private val CardPad = 14.dp
private val CellGap = 3.dp
private val MaxCell = 14.dp
private val MinCell = 6.dp
private val CellCorner = 3.dp
private val HairlineWidth = 1.dp
private val CardShape = RoundedCornerShape(16.dp)

@Preview(name = "历史观看热力图（20 周 × 7 天）", widthDp = 390, showBackground = true)
@Composable
private fun PreviewHistoryHeatmap() {
    Box(Modifier.background(DesignTokens.pageBackground).padding(vertical = DesignTokens.gap.dp)) {
        HistoryHeatmap(cells = previewCells())
    }
}

/** 预览样例：只铺少量非零格；真实数据由 `ProfileStats` 聚合，这里不参与生产。 */
private fun previewCells(): List<HistoryCell> = (0 until PreviewWeeks).flatMap { week ->
    (0 until RowsPerWeek).map { day ->
        HistoryCell(week = week, day = day, level = if ((week * RowsPerWeek + day) % 9 == 0) week % 4 + 1 else 0)
    }
}

private const val PreviewWeeks = 20