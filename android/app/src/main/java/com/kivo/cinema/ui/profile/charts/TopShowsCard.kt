package com.kivo.cinema.ui.profile.charts

import androidx.compose.foundation.background
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
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.profile.model.TopShow

/**
 * 刷剧 TOP3 卡（B2 冻结契约 `TopShowsCard(shows)`）。
 *
 * 基准实测（`profile-3.png`，密度 2.75）：
 * - 空态卡 1008×215 px = 366.5×78 dp，标题「刷剧 TOP3」字形 42 px ⇒ 16 sp Bold；
 *   标题上边距 19 dp、左边距 17 dp；「暂无数据」字形 31 px ⇒ 13 sp，行框间隙约 9 dp。
 * - 有数据态基准未出图（TOP3 为空），三行的间距按同页其它列表的 14 sp / 18 sp 行高组合推断（◐）。
 *
 * 布局取舍：
 * - 条形按本卡内最大值归一，宽度用 `fillMaxWidth(fraction)` 而不是固定 dp：
 *   卡片宽度由外层决定（手机 366.5 dp / 平板更宽），固定条宽会在宽屏上失真；
 * - `hours <= 0` 的条宽度为 0（不画 1 dp 假条）：0 小时与「排第一但为 0」在数据上都表示无观看，
 *   画可见细条会伪造信息；
 * - 行标题 `maxLines = 1` + 省略号，避免长剧名把右侧小时数字挤出卡外。
 */
@Composable
fun TopShowsCard(shows: List<TopShow>) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = DesignTokens.screenPad.dp)
            .clip(RoundedCornerShape(DesignTokens.radiusCard.dp))
            .background(DesignTokens.canvasFill)
            .padding(horizontal = CardSidePad, vertical = CardVerticalPad),
    ) {
        Text(
            text = "刷剧 TOP3",
            fontSize = TitleSize,
            lineHeight = TitleLine,
            fontWeight = FontWeight.Bold,
            color = DesignTokens.textMain,
        )
        val ranked = shows.filter { it.hours > 0f }.take(TopCount)
        if (ranked.isEmpty()) {
            Spacer(Modifier.height(EmptyGap))
            Text(
                text = "暂无数据",
                fontSize = MetaSize,
                lineHeight = MetaLine,
                color = DesignTokens.textSub,
            )
        } else {
            Spacer(Modifier.height(RowsGap))
            ranked.forEachIndexed { index, show ->
                if (index > 0) Spacer(Modifier.height(RowGap))
                TopShowRow(show = show, peak = ranked.first().hours)
            }
        }
    }
}

/** 一行：剧名 + 小时数 + 下方 6 dp 圆角条形（按本卡最大值归一）。 */
@Composable
private fun TopShowRow(show: TopShow, peak: Float) {
    Column(Modifier.fillMaxWidth()) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(
                text = show.title,
                modifier = Modifier.weight(1f),
                fontSize = RowTitleSize,
                lineHeight = RowTitleLine,
                color = DesignTokens.textMain,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
            Spacer(Modifier.width(TitleGap))
            Text(
                text = "${chartHoursText(show.hours)}小时",
                fontSize = MetaSize,
                lineHeight = MetaLine,
                color = DesignTokens.textSub,
            )
        }
        Spacer(Modifier.height(BarTopGap))
        Box(
            modifier = Modifier
                .fillMaxWidth(fraction = (show.hours / peak).coerceIn(0f, 1f))
                .height(BarHeight)
                .clip(RoundedCornerShape(BarRadius))
                .background(DesignTokens.accentGreen),
        )
    }
}

private val CardSidePad = 17.dp
private val CardVerticalPad = 19.dp
private val EmptyGap = 9.dp
private val RowsGap = 8.dp
private val RowGap = 8.dp
private val TitleGap = 8.dp
private val BarTopGap = 4.dp
private val BarHeight = 6.dp
private val BarRadius = 3.dp
private val TitleSize = 16.sp
private val TitleLine = 20.sp
private val MetaSize = 13.sp
private val MetaLine = 16.sp
private val RowTitleSize = 14.sp
private val RowTitleLine = 18.sp
private const val TopCount = 3

@Preview(showBackground = true, widthDp = 393)
@Composable
private fun TopShowsCardPreview() {
    Column(
        modifier = Modifier.background(DesignTokens.pageBackground).padding(vertical = DesignTokens.gap.dp),
        verticalArrangement = Arrangement.spacedBy(DesignTokens.gap.dp),
    ) {
        TopShowsCard(shows = emptyList())
        TopShowsCard(
            shows = listOf(
                TopShow(title = "漫长的季节", hours = 6.4f),
                TopShow(title = "三体", hours = 3.1f),
                TopShow(title = "狂飙", hours = 1.2f),
            ),
        )
    }
}