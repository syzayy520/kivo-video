package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.core.Animatable
import androidx.compose.animation.core.spring
import androidx.compose.material3.Text
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.unit.sp
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.compose.player.row.PlayerTogglePill

/**
 * 玻璃面板的行内构件（P10 版）：**分组分隔线 + 行尾控件**。
 *
 * 与 P10 前的差别：
 * · 分隔线只出现在**组间**，白 8% → **白 6%**，缩进交给列的 20dp 内边距；
 * · 行首零件（[MenuLeading]：icon / 左勾）**删除** —— 列表行不再放装饰图标，
 *   勾与 chevron 一律到行尾（§5），文字从 20dp 起；
 * · 滚动条迁到 [PlayerPanelScroll.PanelScrollbar]（它需要 ScrollState 与自动隐藏计时，
 *   与"一行之内/之间"的零件不是同一件事）。
 *
 * 本文件只画"一行之内/之间"的零件，不决定面板布局与滚动行为。
 */

/** 组间分隔线：白 6%，上下各 4dp，横向由列的 20dp 内边距决定。 */
@Composable
internal fun MenuSeparator() {
    Box(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = PlayerPanelStyle.dividerPad)
            .height(1.dp)
            .background(PlayerPanelStyle.dividerColor),
    )
}

/**
 * 行尾：chevron（钻入 16dp 白 45%）/ 开关 / 勾（16dp 白 90%，出现时 140ms spring 放大）。
 *
 * 【勾为什么有动画】§8：`spring(0.7, 500)` + scale 0.8→1。切换轨道时如果勾"啪"地出现，
 * 用户很难确认刚才点的是哪一行；140ms 的放大就是那一下"对，是它"。
 */
@Composable
internal fun MenuTrailing(item: PlayerOptionItem) {
    when {
        item.control == PlayerRowControl.CHEVRON -> PlayerGlyph(
            kind = "chevron-right",
            color = PlayerPanelStyle.chevronColor,
            modifier = Modifier
                .padding(start = 8.dp)
                .size(PlayerPanelStyle.rowChevronSize),
        )
        item.control == PlayerRowControl.TOGGLE -> PlayerTogglePill(
            on = item.on,
            modifier = Modifier.padding(start = 8.dp),
        )
        item.control == PlayerRowControl.CHECK && item.selected -> SelectCheck()
    }
}

/** 行尾勾：选中态出现时 140ms spring(0.7, 500) 放大 0.8→1，同时淡入。 */
@Composable
private fun SelectCheck() {
    // 用 Animatable 而不是 animateFloatAsState：后者的首帧初值等于目标值，不会动。
    val progress = remember { Animatable(0f) }
    LaunchedEffect(Unit) {
        progress.animateTo(1f, spring(dampingRatio = 0.7f, stiffness = 500f))
    }
    val p = progress.value.coerceIn(0f, 1f)
    PlayerGlyph(
        kind = "check",
        color = PlayerPanelStyle.checkColor.copy(
            alpha = PlayerPanelStyle.checkColor.alpha * p,
        ),
        modifier = Modifier
            .padding(start = 8.dp)
            .size(PlayerPanelStyle.rowCheckSize)
            .graphicsLayer {
                val s = 0.8f + 0.2f * p
                scaleX = s
                scaleY = s
            },
        strokeWidth = 2.2f,
    )
}

/** 面板头（56 高：18/24 题 + 12/17 副，§4/§5）。 */
@Composable
internal fun MenuHeader(title: String, subtitle: String) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(PlayerPanelStyle.headerHeight)
            .padding(end = PlayerPanelStyle.rowTrailingPad),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = title,
                color = PlayerPanelStyle.titleColor,
                fontSize = PlayerPanelStyle.titleSize.sp,
                lineHeight = PlayerPanelStyle.titleLineHeight.sp,
                letterSpacing = PlayerPanelStyle.titleTracking.sp,
                fontWeight = FontWeight.Medium,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
            if (subtitle.isNotBlank()) {
                Text(
                    text = subtitle,
                    color = PlayerPanelStyle.subtitleColor,
                    fontSize = PlayerPanelStyle.subtitleSize.sp,
                    lineHeight = PlayerPanelStyle.subtitleLineHeight.sp,
                    letterSpacing = PlayerPanelStyle.subtitleTracking.sp,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )
            }
        }
        if (subtitle.isNotBlank()) {
            PlayerGlyph(
                kind = "chevron-down",
                color = PlayerPanelStyle.chevronColor,
                modifier = Modifier
                    .padding(start = 8.dp)
                    .size(14.dp),
            )
        }
    }
}
