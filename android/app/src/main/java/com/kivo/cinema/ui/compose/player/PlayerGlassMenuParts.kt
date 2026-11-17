package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.unit.dp

/**
 * 玻璃面板的行内构件：分隔线、行首、行尾、滚动条。
 *
 * 从 [PlayerGlassMenu] 拆出（原文件加 D14 语义后 304 行，超 300 上限）。
 * 拆分口径：本文件只画"一行之内/之间的零件"，不决定面板布局与滚动行为——
 * 那些留在主文件，两者因此可独立改动而不互相牵连。
 */

/** 分隔线：白 8%，**从文本左缘起**（不是通栏），与实测一致。 */
@Composable
internal fun MenuSeparator() {
    Box(
        modifier = Modifier
            .fillMaxWidth()
            .padding(start = PlayerTokens.rowTitleInset.dp, top = 4.dp, bottom = 4.dp)
            .height(1.dp)
            .clip(RoundedCornerShape(0.5.dp))
            .background(PlayerTokens.panelDivider),
    )
}

/**
 * 行首：有 icon 时 icon 心在 +30、题在 +65；
 * 无 icon 时选中勾心在 +25、题在 +43（倍速列表的实测排布）。
 */
@Composable
internal fun MenuLeading(item: PlayerOptionItem) {
    val hasIcon = item.icon != null
    val inset = if (hasIcon) PlayerTokens.rowIconTitleInset else PlayerTokens.rowTitleInset
    Box(
        modifier = Modifier.width(inset.dp),
        contentAlignment = Alignment.CenterStart,
    ) {
        if (hasIcon) {
            PlayerGlyph(
                kind = item.icon!!,
                color = PlayerTokens.textPrimary,
                modifier = Modifier
                    .padding(start = 21.dp)
                    .size(PlayerTokens.rowIconSize.dp),
            )
        } else if (item.selected && item.control == PlayerRowControl.CHECK) {
            PlayerGlyph(
                kind = "check",
                color = PlayerTokens.textPrimary,
                modifier = Modifier
                    .padding(start = 18.dp)
                    .size(PlayerTokens.rowCheckSize.dp),
                strokeWidth = 2.2f,
            )
        }
    }
}

/** 行尾：chevron（钻入）/ 开关 pill / 白勾（带 icon 的列表）/ 无。 */
@Composable
internal fun MenuTrailing(item: PlayerOptionItem) {
    when {
        item.control == PlayerRowControl.CHEVRON -> PlayerGlyph(
            kind = "chevron-right",
            color = PlayerTokens.chevron,
            modifier = Modifier
                .padding(start = 8.dp)
                .size(PlayerTokens.chevronSize.dp),
        )
        item.control == PlayerRowControl.TOGGLE -> PlayerTogglePill(
            on = item.on,
            modifier = Modifier.padding(start = 8.dp),
        )
        item.icon != null && item.selected -> PlayerGlyph(
            kind = "check",
            color = PlayerTokens.textPrimary,
            modifier = Modifier
                .padding(start = 8.dp)
                .size(PlayerTokens.rowCheckSize.dp),
            strokeWidth = 2.2f,
        )
    }
}

/** 滚动条：3pt 白 40%，贴右内 6pt，高度按可视比例。 */
@Composable
internal fun MenuScrollbar(fraction: Float, visible: Boolean, modifier: Modifier = Modifier) {
    if (!visible) return
    Canvas(
        modifier = modifier
            .padding(end = PlayerTokens.scrollbarInset.dp)
            .width(PlayerTokens.scrollbarWidth.dp)
            .height(72.dp),
    ) {
        val thumbH = size.height * 0.4f
        val y = (size.height - thumbH) * fraction.coerceIn(0f, 1f)
        drawRoundRect(
            color = PlayerTokens.scrollbar,
            topLeft = Offset(0f, y),
            size = Size(size.width, thumbH),
            cornerRadius = CornerRadius(size.width / 2f),
        )
    }
}
