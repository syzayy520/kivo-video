package com.kivo.cinema.ui.profile.charts

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.profile.model.TrendStat
import kotlin.math.abs

/**
 * 分类走势卡（B2 冻结契约 `TrendCard(stat)`，`ProfilePage` 逐条渲染）。
 *
 * ⚠️ **基准缺口（显式记录）**：`profile-*` 系列基准帧里趋势区未渲染
 * （`ProfilePage` 的 `trends` 恒空，空数据不画卡），因此本卡没有可比对的像素基准。
 * 以下数值全部来自任务口径，不是截图实测：
 *
 * - 浅紫底 `washFill`（`0x80EDEDFB`，50% 淡紫）、圆角 16 dp、内边距 12 dp、高 72 dp；
 * - 名称 14 sp、小时 22 sp Bold、`↑/↓{delta}小时` 13 sp（绿 `accentGreen` / 红 `dangerRed`）。
 *
 * 取舍（◐ 推断）：
 * - 高度用 `heightIn(min = 72.dp)` 而不是 `height(72.dp)`：字体放大到 1.3× 时固定高度会裁切
 *   22 sp 的小时数；1.0× 下内容高 69 dp < 72 dp，实际渲染仍是 72 dp；
 * - `delta == 0` 用灰色「0小时」而不是绿色「↑0小时」：没有变化不是正向增长，
 *   借绿箭头会被读成「涨了」；
 * - `washFill` 自带 50% alpha，直接作 `background` 会与页面渐变合成淡紫，
 *   不再额外 `copy(alpha=)` —— 二次降透明会把它压成接近白色。
 */
@Composable
fun TrendCard(stat: TrendStat) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp)
            .heightIn(min = CardHeight)
            .clip(RoundedCornerShape(CardRadius))
            .background(DesignTokens.washFill)
            .padding(CardPad),
    ) {
        Text(
            text = stat.name,
            fontSize = NameSize,
            lineHeight = NameLine,
            color = DesignTokens.textSub,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis,
        )
        Spacer(Modifier.height(NameGap))
        Row(verticalAlignment = Alignment.Bottom) {
            Text(
                text = chartHoursText(stat.hours),
                fontSize = HoursSize,
                lineHeight = HoursLine,
                fontWeight = FontWeight.Bold,
                color = DesignTokens.textMain,
            )
            Spacer(Modifier.width(DeltaGap))
            Text(
                text = deltaText(stat.delta),
                fontSize = DeltaSize,
                lineHeight = DeltaLine,
                color = deltaColor(stat.delta),
                modifier = Modifier.padding(bottom = DeltaBaselinePad),
            )
        }
    }
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

private val CardRadius = 16.dp
private val CardHeight = 72.dp
private val CardPad = 12.dp
private val NameGap = 2.dp
private val DeltaGap = 6.dp
private val DeltaBaselinePad = 2.dp
private val NameSize = 14.sp
private val NameLine = 17.sp
private val HoursSize = 22.sp
private val HoursLine = 26.sp
private val DeltaSize = 13.sp
private val DeltaLine = 16.sp

@Preview(showBackground = true, widthDp = 393)
@Composable
private fun TrendCardPreview() {
    Column(
        modifier = Modifier.background(DesignTokens.pageBackground).padding(vertical = DesignTokens.gap.dp),
        verticalArrangement = Arrangement.spacedBy(DesignTokens.gap.dp),
    ) {
        TrendCard(TrendStat(name = "剧情", hours = 8.4f, delta = 1.7f))
        TrendCard(TrendStat(name = "喜剧", hours = 3.2f, delta = -0.4f))
        TrendCard(TrendStat(name = "纪录片", hours = 0f, delta = 0f))
    }
}