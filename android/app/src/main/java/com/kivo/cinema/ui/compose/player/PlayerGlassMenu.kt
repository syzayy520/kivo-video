package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * **玻璃列表壳**（v3 §1.4 形态 1/2/3/4/7/8/10）。
 *
 * 七种面板共用这一个壳：字幕、倍速、画面比例、音轨、跳过方式、弹幕操作、投屏。
 * 差别全在数据（[PlayerOptionItem]）与宽度，不在代码。
 *
 * 实测规格：宽 248（投屏小卡 160）、圆角 16、行高 42（双行 60）、
 * 题 16pt 白 / 副 11pt 灰、section 头 12pt 灰、分隔线白 8% 从文本左缘起、
 * 滚动条 3pt 白 40% 贴右内 6pt、选中白勾、chevron 白 60% 12pt。
 *
 * 【为什么限高 + 滚动】面板浮在底栏之上，可用高度 ≈331pt；
 * 7 档倍速 = 7×42 + 头 ≈ 330pt 刚好，字幕 6 行双行就超了。
 * 不限高 ⇒ 最后几行被屏幕切掉、**永远点不到**（v2 实机踩过）。
 */
@Composable
internal fun PlayerGlassMenu(
    title: String,
    items: List<PlayerOptionItem>,
    onSelect: (PlayerOptionItem) -> Unit,
    modifier: Modifier = Modifier,
    subtitle: String = "",
    width: Dp = PlayerTokens.panelWidth.dp,
) {
    val scroll = rememberScrollState()
    Box(
        modifier = modifier
            .width(width)
            .heightIn(max = PlayerTokens.panelMaxHeight.dp)
            .playerGlassSurface()
            // 吃掉面板内的点击：手势层在下面兜底，不消费就会"点面板＝关面板"。
            .pointerInput(Unit) { detectTapGestures(onTap = {}) },
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .verticalScroll(scroll)
                .padding(vertical = 6.dp),
        ) {
            if (title.isNotBlank()) {
                MenuHeader(title = title, subtitle = subtitle)
            }
            var lastSection: String? = null
            items.forEach { item ->
                if (item.section.isNotBlank() && item.section != lastSection) {
                    if (lastSection != null || title.isNotBlank()) {
                        MenuSeparator()
                    }
                    Text(
                        text = item.section,
                        color = PlayerTokens.textSecondary,
                        fontSize = PlayerTokens.sectionHeadSize.sp,
                        maxLines = 1,
                        modifier = Modifier.padding(
                            start = PlayerTokens.rowTitleInset.dp,
                            top = 8.dp,
                            bottom = 2.dp,
                        ),
                    )
                }
                lastSection = item.section
                MenuRow(item = item, onSelect = { onSelect(item) })
            }
        }
        MenuScrollbar(
            fraction = if (scroll.maxValue > 0) {
                scroll.value.toFloat() / scroll.maxValue
            } else {
                0f
            },
            visible = scroll.maxValue > 0,
            modifier = Modifier.align(Alignment.CenterEnd),
        )
    }
}

/** 面板头（形态 7：题 + 副 + ⌄）。 */
@Composable
private fun MenuHeader(title: String, subtitle: String) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(start = PlayerTokens.rowTitleInset.dp, end = 14.dp, top = 8.dp, bottom = 6.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = title,
                color = PlayerTokens.textPrimary,
                fontSize = PlayerTokens.panelTitleSize.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
            if (subtitle.isNotBlank()) {
                Text(
                    text = subtitle,
                    color = PlayerTokens.textSecondary,
                    fontSize = PlayerTokens.panelSubtitleSize.sp,
                    maxLines = 1,
                )
            }
        }
        if (subtitle.isNotBlank()) {
            PlayerGlyph(
                kind = "chevron-down",
                color = PlayerTokens.chevron,
                modifier = Modifier.size(PlayerTokens.chevronSize.dp),
            )
        }
    }
}

/** 分隔线：白 8%，**从文本左缘起**（不是通栏），与实测一致。 */
@Composable
private fun MenuSeparator() {
    Box(
        modifier = Modifier
            .fillMaxWidth()
            .padding(start = PlayerTokens.rowTitleInset.dp, top = 4.dp, bottom = 4.dp)
            .height(1.dp)
            .clip(RoundedCornerShape(0.5.dp))
            .background(PlayerTokens.panelDivider),
    )
}

/** 一行。双行（有副标题）60pt，单行 42pt。 */
@Composable
private fun MenuRow(item: PlayerOptionItem, onSelect: () -> Unit) {
    val tall = item.subtitle.isNotBlank()
    val rowHeight = if (tall) PlayerTokens.panelRowTallHeight else PlayerTokens.panelRowHeight
    val ink = if (item.enabled) 1f else 0.4f
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(rowHeight.dp)
            .alpha(ink)
            .pointerInput(item.enabled) {
                if (item.enabled) detectTapGestures(onTap = { onSelect() })
            }
            .padding(end = PlayerTokens.rowTrailingPad.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        MenuLeading(item)
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = item.title,
                color = PlayerTokens.textPrimary,
                fontSize = PlayerTokens.panelTitleSize.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
            if (tall) {
                Text(
                    text = item.subtitle,
                    color = PlayerTokens.textSecondary,
                    fontSize = PlayerTokens.panelSubtitleSize.sp,
                    maxLines = 2,
                    overflow = TextOverflow.Ellipsis,
                )
            }
        }
        if (item.value.isNotBlank()) {
            Text(
                text = item.value,
                color = PlayerTokens.textSecondary,
                fontSize = PlayerTokens.panelSubtitleSize.sp,
                maxLines = 1,
                modifier = Modifier.padding(start = 8.dp),
            )
        }
        MenuTrailing(item)
    }
}

/**
 * 行首：有 icon 时 icon 心在 +30、题在 +65；
 * 无 icon 时选中勾心在 +25、题在 +43（倍速列表的实测排布）。
 */
@Composable
private fun MenuLeading(item: PlayerOptionItem) {
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
private fun MenuTrailing(item: PlayerOptionItem) {
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
private fun MenuScrollbar(fraction: Float, visible: Boolean, modifier: Modifier = Modifier) {
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