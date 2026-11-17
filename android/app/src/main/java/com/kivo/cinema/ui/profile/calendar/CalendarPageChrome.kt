package com.kivo.cinema.ui.profile.calendar

import androidx.compose.foundation.Canvas
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
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.components.KivoBackPill
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 日历页的「chrome」：顶栏（返回 / 标题 / 今天）、月·年切换、期间行、空态。
 *
 * 从 [CalendarPage] 拆出来的唯一理由是职责：本文件只画**不依赖数据**的框架件，
 * 页面文件只做数据编排与摘要卡。拆后两者都在 300 行内（`tools/ui_gate.py` 行数门）。
 *
 * ## 尺寸对照（任务卡 → 实现）
 *
 * | 元素 | 基准 | 实现 | 说明 |
 * | --- | --- | --- | --- |
 * | 返回按钮命中 | 44dp | 44dp | 白 60% 圆底（`canvasFill` @60%） |
 * | 页面标题 | 28sp Bold | 28sp Bold | 颜值门「页面标题 26–28sp Bold」 |
 * | 「今天」文字 | 13sp `navActive` | 13sp `navActive` | 命中区 44dp 高（上下各补 8dp） |
 * | 月/年胶囊 | 30dp 高 / 圆角 15 | 30dp / 15dp / 宽 56dp | 选中白底 + `textMain`，未选中 `textSub` |
 * | 期间文字 | 16sp Bold | 16sp Bold | 月视图显示「2026年11月」，年视图显示「2026年」 |
 * | 左右箭头命中 | 44dp | 44dp | 箭头本体 16dp，Canvas 画 `‹ ›` |
 * | 空态图标 | 40dp Canvas | 40dp | 日历线框，`textSub` 描边 + `accentGreen` 一天点 |
 * | 空态文案 | 13sp `textSub` | 13sp `textSub` | 行高 20sp，多行居中 |
 *
 * ## 偏差
 *
 * - 选中胶囊加 1dp `hairline` 描边（白底落在浅紫渐变上，不描边时右缘会「融进」背景）；
 *   空态图标不画具体日期数字（Canvas 里塞文字会被字体缩放拉糊），只画线框 + 一个绿点。
 */
@Composable
internal fun CalendarTopBar(onBack: () -> Unit, onToday: () -> Unit) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp, vertical = DesignTokens.gap.dp),
    ) {
        Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            KivoBackPill(
                onClick = onBack,
                background = DesignTokens.canvasFill.copy(alpha = BackCapsuleAlpha),
                modifier = Modifier.size(BackHit),
            ) {
                Chevron(pointsLeft = true, size = BackGlyph, color = DesignTokens.textMain)
            }
            Spacer(Modifier.weight(1f))
            Text(
                text = "今天",
                modifier = Modifier
                    .clip(CircleShape)
                    .clickable(role = Role.Button, onClick = onToday)
                    .padding(horizontal = TodaySidePad, vertical = TodayVerticalPad),
                fontSize = TodayTextSize,
                fontWeight = FontWeight.Medium,
                color = DesignTokens.navActive,
            )
        }
        Spacer(Modifier.height(TitleTopGap))
        Text(
            text = "日历",
            fontSize = PageTitleSize,
            fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
        )
    }
}

/** 月 / 年分段切换：两枚独立胶囊，不做轨道（轨道会让 30dp 的胶囊显小）。 */
@Composable
internal fun CalendarViewSwitch(yearView: Boolean, onSelectMonth: () -> Unit, onSelectYear: () -> Unit) {
    Row(
        modifier = Modifier.padding(horizontal = DesignTokens.screenPad.dp),
        horizontalArrangement = Arrangement.spacedBy(SwitchGap),
    ) {
        SwitchCapsule(label = "月", selected = !yearView, onClick = onSelectMonth)
        SwitchCapsule(label = "年", selected = yearView, onClick = onSelectYear)
    }
}

@Composable
private fun SwitchCapsule(label: String, selected: Boolean, onClick: () -> Unit) {
    val shape = RoundedCornerShape(SwitchCorner)
    Box(
        modifier = Modifier
            .width(SwitchWidth)
            .height(SwitchHeight)
            .clip(shape)
            .then(
                if (selected) {
                    Modifier.background(DesignTokens.canvasFill).border(1.dp, DesignTokens.hairline, shape)
                } else {
                    Modifier
                }
            )
            .clickable(role = Role.Button, onClick = onClick),
        contentAlignment = Alignment.Center,
    ) {
        Text(
            text = label,
            fontSize = SwitchTextSize,
            fontWeight = if (selected) FontWeight.Medium else FontWeight.Normal,
            color = if (selected) DesignTokens.textMain else DesignTokens.textSub,
        )
    }
}

/** 期间行：左右各 44dp 命中，中间文字居中；月视图切月、年视图切年由调用方决定。 */
@Composable
internal fun CalendarPeriodRow(label: String, onPrev: () -> Unit, onNext: () -> Unit) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        PeriodArrow(pointsLeft = true, onClick = onPrev)
        Text(
            text = label,
            modifier = Modifier.weight(1f),
            fontSize = PeriodTextSize,
            fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
            textAlign = TextAlign.Center,
        )
        PeriodArrow(pointsLeft = false, onClick = onNext)
    }
}

@Composable
private fun PeriodArrow(pointsLeft: Boolean, onClick: () -> Unit) {
    Box(
        modifier = Modifier
            .size(ArrowHit)
            .clip(CircleShape)
            .clickable(role = Role.Button, onClick = onClick),
        contentAlignment = Alignment.Center,
    ) {
        Chevron(pointsLeft = pointsLeft, size = ArrowGlyph, color = DesignTokens.textSub)
    }
}

/** 空态：40dp 日历线框 + 一句引导。无记录时不画网格，也不给「今天」按钮之外的假入口。 */
@Composable
internal fun CalendarEmptyState(modifier: Modifier = Modifier) {
    Column(
        modifier = modifier.padding(horizontal = EmptySidePad),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
    ) {
        CalendarGlyph()
        Spacer(Modifier.height(EmptyGlyphGap))
        Text(
            text = "追剧日历会在你看过或加入待看后出现",
            fontSize = EmptyHintSize,
            lineHeight = EmptyHintLineHeight,
            color = DesignTokens.textSub,
            textAlign = TextAlign.Center,
        )
    }
}

@Composable
private fun CalendarGlyph() {
    Canvas(modifier = Modifier.size(GlyphSize)) {
        val stroke = GlyphStroke.toPx()
        val w = size.width
        val h = size.height
        val bodyTop = h * 0.22f
        val corner = CornerRadius(GlyphCorner.toPx(), GlyphCorner.toPx())
        drawRoundRect(
            color = DesignTokens.textSub,
            topLeft = Offset(stroke / 2, bodyTop),
            size = Size(w - stroke, h - bodyTop - stroke / 2),
            cornerRadius = corner,
            style = Stroke(stroke),
        )
        // 表头分隔线
        drawLine(
            color = DesignTokens.textSub,
            start = Offset(stroke / 2, h * 0.40f),
            end = Offset(w - stroke / 2, h * 0.40f),
            strokeWidth = stroke,
        )
        // 顶部两个挂环
        listOf(0.32f, 0.68f).forEach { x ->
            drawLine(
                color = DesignTokens.textSub,
                start = Offset(w * x, h * 0.06f),
                end = Offset(w * x, bodyTop),
                strokeWidth = stroke,
                cap = StrokeCap.Round,
            )
        }
        // 一格「看过的一天」
        drawCircle(
            color = DesignTokens.accentGreen,
            radius = w * 0.07f,
            center = Offset(w * 0.5f, h * 0.60f),
        )
    }
}

/** `‹` / `›` 用两笔折线画，避免字体差异导致箭头粗细不一致。 */
@Composable
private fun Chevron(pointsLeft: Boolean, size: Dp, color: Color) {
    Canvas(modifier = Modifier.size(size)) {
        val stroke = ChevronStroke.toPx()
        val tipX = if (pointsLeft) this.size.width * 0.34f else this.size.width * 0.66f
        val backX = if (pointsLeft) this.size.width * 0.64f else this.size.width * 0.36f
        drawLine(
            color = color,
            start = Offset(backX, this.size.height * 0.18f),
            end = Offset(tipX, this.size.height * 0.5f),
            strokeWidth = stroke,
            cap = StrokeCap.Round,
        )
        drawLine(
            color = color,
            start = Offset(tipX, this.size.height * 0.5f),
            end = Offset(backX, this.size.height * 0.82f),
            strokeWidth = stroke,
            cap = StrokeCap.Round,
        )
    }
}

private val BackHit = 44.dp
private val BackGlyph = 18.dp
private val TitleTopGap = 6.dp
private val PageTitleSize = 28.sp
private val TodayTextSize = 13.sp
private val TodaySidePad = 8.dp
private val TodayVerticalPad = 8.dp
private val SwitchHeight = 30.dp
private val SwitchWidth = 56.dp
private val SwitchCorner = 15.dp
private val SwitchGap = 6.dp
private val SwitchTextSize = 13.sp
private val ArrowHit = 44.dp
private val ArrowGlyph = 16.dp
private val PeriodTextSize = 16.sp
private val GlyphSize = 40.dp
private val GlyphStroke = 1.5.dp
private val GlyphCorner = 7.dp
private val ChevronStroke = 1.8.dp
private val EmptyGlyphGap = 16.dp
private val EmptySidePad = 40.dp
private val EmptyHintSize = 13.sp
private val EmptyHintLineHeight = 20.sp
private const val BackCapsuleAlpha = 0.6f

@Preview(showBackground = true, widthDp = 390)
@Composable
private fun CalendarChromePreview() {
    Column(modifier = Modifier.background(DesignTokens.pageBackground)) {
        CalendarTopBar(onBack = {}, onToday = {})
        CalendarViewSwitch(yearView = false, onSelectMonth = {}, onSelectYear = {})
        Spacer(Modifier.height(DesignTokens.gap.dp))
        CalendarPeriodRow(label = "2026年11月", onPrev = {}, onNext = {})
        Spacer(Modifier.height(24.dp))
        // 空态：居中图标 + 引导句（40dp 图标，13sp textSub）
        CalendarEmptyState(modifier = Modifier.fillMaxWidth().height(260.dp))
    }
}