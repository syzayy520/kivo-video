package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
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
import androidx.compose.runtime.getValue
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.onClick
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.compose.player.row.PlayerResetRow
import com.kivo.cinema.ui.compose.player.row.PlayerStepperRow
import com.kivo.cinema.ui.compose.player.row.PlayerToggleRow
import com.kivo.cinema.ui.motion.MotionGate

/**
 * **微调卡壳**（P10 §4/§5/§7/§11）：音频延迟、跳过片段、字幕/弹幕显示设置。
 *
 * P10 的四处结构变化：
 * · **内嵌卡删除**（旧版外层 16 圆角玻璃里再套 10 圆角黑 30% 卡 = 盒中盒）；
 * · 头部固定 56 高，✕ 去圆底、改 44 命中 + 16dp glyph（白 60%）；
 * · 行高按种类：步进 64 / 普通 56（双行 72）/ 开关 64 / 重置 48；
 * · 底部留白 64（滚动内容），**重置行固定在卡底**（保留 FAIL-2 修复）。
 *
 * 【为什么与列表壳分开】列表壳是"选一个"，微调卡是"调一个值"：
 * 前者行尾是勾，后者行尾是 −/+ 或开关，还多一个 ✕ 与重置。
 * 硬塞进同一个壳会让行模型长出一堆互斥字段 —— 两种壳共用**行数据**就够了。
 *
 * 【FAIL-2 是什么】旧版把重置行放在滚动列末尾，K40 上 358dp 卡从 top 94 起只有 283 可用，
 * 滚到最大也够不着 ⇒ 重置永远点不到。现在它 `align(BottomCenter)` 固定，滚动列只负责让位。
 */
@Composable
internal fun PlayerDetailCard(
    title: String,
    items: List<PlayerOptionItem>,
    onSelect: (PlayerOptionItem) -> Unit,
    onStep: (PlayerOptionItem, Int) -> Unit,
    onDismiss: () -> Unit,
    modifier: Modifier = Modifier,
    subtitle: String = "",
    width: Dp = PlayerTokens.detailCardWidth.dp,
    showReset: Boolean = true,
    /** 抓帧模糊背景图；null（默认）时分文不画，视觉与不带背景图完全一致。 */
    backdrop: ImageBitmap? = null,
) {
    val scroll = rememberScrollState()
    val reduceMotion = MotionGate.rememberAppReduceMotion()
    val contentAlpha = panelContentAlpha(items.isNotEmpty(), reduceMotion)
    Column(
        modifier = modifier
            .width(width)
            .heightIn(max = 358.dp)
            .playerPanelSurface(
                // 顶部受光带（头部高度内 2.5% 渐隐）：只是光，滚动时留着也不穿帮。
                headerWash = PlayerPanelStyle.headerWash,
                headerHeight = PlayerPanelStyle.headerHeight + PlayerPanelStyle.verticalPadComfort,
                backdrop = backdrop,
            )
            .pointerInput(Unit) { detectTapGestures(onTap = {}) },
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                // 视口在重置行上缘就收住（FAIL-2 的结构补完）：旧版滚动列占满整卡、
                // 重置行透明地压在它上面，行文字会从「重置」底下穿过去，12dp 渐隐也
                // 落到了卡片底缘而不是这道切口上。视口按重置行高度让位后，
                // 裁切与渐隐同时回到这条看得见的边界。
                .weight(1f, fill = false)
                .panelVerticalFade(
                    top = scroll.canScrollBackward,
                    bottom = scroll.canScrollForward,
                    fadeBottom = 12.dp,
                )
                // 内容裁到卡圆角内（fade 留在外层，DstOut 只削内容 alpha，不被裁）。
                .clip(RoundedCornerShape(PlayerPanelStyle.radius))
                .verticalScroll(scroll)
                .padding(
                    start = PlayerPanelStyle.hPadStart,
                    top = PlayerPanelStyle.verticalPadComfort,
                    // §5 的 64dp 呼吸；重置行在滚动区之外，不需要再让位。
                    bottom = PlayerPanelStyle.detailBottom,
                )
                .graphicsLayer { alpha = contentAlpha },
        ) {
            DetailHeader(
                title = title,
                subtitle = subtitle,
                onDismiss = onDismiss,
            )
            renderRows(items = items, onSelect = onSelect, onStep = onStep)
        }

        // 固定卡底、不随滚动走：永远可见可点。
        if (showReset) {
            PlayerResetRow(
                onReset = { onSelect(PlayerOptionItem(title = "重置", tag = "reset")) },
            )
        }
    }
}

@Composable
private fun renderRows(
    items: List<PlayerOptionItem>,
    onSelect: (PlayerOptionItem) -> Unit,
    onStep: (PlayerOptionItem, Int) -> Unit,
) {
    var lastSection: String? = null
    items.forEach { item ->
        val newGroup = item.section.isNotBlank() && item.section != lastSection
        if (newGroup) {
            if (lastSection != null) {
                Spacer(Modifier.height(PlayerPanelStyle.groupGap))
                MenuSeparator()
            } else {
                Spacer(Modifier.height(PlayerPanelStyle.groupGap))
            }
            Text(
                text = item.section,
                color = PlayerPanelStyle.groupHeadColor,
                fontSize = PlayerPanelStyle.groupHeadSize.sp,
                lineHeight = PlayerPanelStyle.groupHeadLineHeight.sp,
                letterSpacing = PlayerPanelStyle.groupHeadTracking.sp,
                fontWeight = FontWeight.Medium,
                maxLines = 1,
                modifier = Modifier.padding(
                    top = PlayerPanelStyle.groupHeadTop,
                    bottom = PlayerPanelStyle.groupHeadBottom,
                ),
            )
        }
        lastSection = item.section
        when (item.control) {
            PlayerRowControl.STEPPER -> PlayerStepperRow(
                item = item,
                onStep = { delta -> onStep(item, delta) },
            )
            PlayerRowControl.TOGGLE -> PlayerToggleRow(
                item = item,
                onToggle = { onSelect(item) },
            )
            else -> DetailPlainRow(item = item, onSelect = { onSelect(item) })
        }
    }
}

/** 头部 56 高：18/24 题 + 12/17 副；✕ 44 命中、16dp glyph、白 60%、**无圆底**。 */
@Composable
private fun DetailHeader(title: String, subtitle: String, onDismiss: () -> Unit) {
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
        Box(
            modifier = Modifier
                .size(PlayerPanelStyle.detailCloseHit)
                .clearAndSetSemantics {
                    contentDescription = "关闭$title"
                    role = Role.Button
                    onClick { onDismiss(); true }
                }
                .pointerInput(Unit) { detectTapGestures(onTap = { onDismiss() }) },
            contentAlignment = Alignment.Center,
        ) {
            PlayerGlyph(
                kind = "close",
                color = PlayerPanelStyle.closeColor,
                modifier = Modifier.size(PlayerPanelStyle.detailCloseGlyph),
                strokeWidth = 2.2f,
            )
        }
    }
}

/** 卡里的普通行：题 16/22 + 副 12/17，行尾勾或 chevron（16dp），整行可按。 */
@Composable
private fun DetailPlainRow(item: PlayerOptionItem, onSelect: () -> Unit) {
    val currentOnSelect by rememberUpdatedState(onSelect)
    val tall = item.subtitle.isNotBlank()
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(PlayerPanelStyle.rowHeight(PlayerPanelStyle.Density.Comfort, tall))
            .alpha(if (item.enabled) 1f else 0.38f)
            .panelRowWash(selected = item.selected)
            .panelPressable(enabled = item.enabled, onClick = currentOnSelect)
            .clearAndSetSemantics {
                contentDescription = listOfNotNull(
                    item.title,
                    item.subtitle.takeIf { it.isNotBlank() },
                    item.value.takeIf { it.isNotBlank() },
                ).joinToString("，")
                role = if (item.control == PlayerRowControl.CHEVRON) Role.Button else Role.Tab
                if (item.selected) stateDescription = "已选中"
                if (item.enabled) onClick { onSelect(); true }
            }
            .padding(end = PlayerPanelStyle.rowTrailingPad),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = item.title,
                color = PlayerPanelStyle.rowTitleColor,
                fontSize = PlayerPanelStyle.rowTitleSize.sp,
                lineHeight = PlayerPanelStyle.rowTitleLineHeight.sp,
                letterSpacing = PlayerPanelStyle.rowTitleTracking.sp,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
            if (tall) {
                Text(
                    text = item.subtitle,
                    color = PlayerPanelStyle.rowSubtitleColor,
                    fontSize = PlayerPanelStyle.subtitleSize.sp,
                    lineHeight = PlayerPanelStyle.subtitleLineHeight.sp,
                    letterSpacing = PlayerPanelStyle.subtitleTracking.sp,
                    maxLines = 2,
                    overflow = TextOverflow.Ellipsis,
                )
            }
        }
        if (item.value.isNotBlank()) {
            Text(
                text = item.value,
                color = PlayerPanelStyle.valueColor,
                fontSize = PlayerPanelStyle.valueSize.sp,
                lineHeight = PlayerPanelStyle.valueLineHeight.sp,
                style = TextStyle(fontFeatureSettings = PlayerPanelStyle.valueFontFeature),
                fontWeight = FontWeight.Medium,
                textAlign = TextAlign.End,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
                modifier = Modifier
                    .padding(start = 8.dp)
                    .width(PlayerPanelStyle.valueColumn),
            )
        }
        MenuTrailing(item)
    }
}