package com.kivo.cinema.ui.compose.player

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
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * **微调卡壳**（v3 §1.4 形态 5/6/9）：音频延迟、跳过片段、字幕/弹幕显示设置。
 *
 * 实测：宽 357pt、top 16pt、右上 ✕ 圆钮 38pt（内边距 16）、
 * 内嵌卡 margin 15 / 圆角 10 / 黑 30%、行高 50pt、重置行居中。
 *
 * 【为什么与列表壳分开】列表壳是"选一个"，微调卡是"调一个值"：
 * 前者行尾是勾，后者行尾是 −/+ 或开关，还多一个 ✕ 与重置。
 * 硬塞进同一个壳会让行模型长出一堆互斥字段 —— 两种壳共用**行数据**就够了。
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
) {
    Box(
        modifier = modifier
            .width(width)
            .heightIn(max = 358.dp)
            .playerGlassSurface()
            .pointerInput(Unit) { detectTapGestures(onTap = {}) },
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .verticalScroll(rememberScrollState())
                .padding(PlayerTokens.innerCardMargin.dp),
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
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
                Box(
                    modifier = Modifier
                        .size(PlayerTokens.detailCloseSize.dp)
                        .clip(CircleShape)
                        .background(PlayerTokens.chipSurface)
                        .pointerInput(Unit) { detectTapGestures(onTap = { onDismiss() }) },
                    contentAlignment = Alignment.Center,
                ) {
                    PlayerGlyph(
                        kind = "close",
                        color = PlayerTokens.textPrimary,
                        modifier = Modifier.size(14.dp),
                        strokeWidth = 2.2f,
                    )
                }
            }

            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(top = 12.dp)
                    .clip(RoundedCornerShape(PlayerTokens.innerCardRadius.dp))
                    .background(PlayerTokens.innerCardSurface),
            ) {
                var lastSection: String? = null
                items.forEach { item ->
                    if (item.section.isNotBlank() && item.section != lastSection) {
                        Text(
                            text = item.section,
                            color = PlayerTokens.textSecondary,
                            fontSize = PlayerTokens.sectionHeadSize.sp,
                            maxLines = 1,
                            modifier = Modifier.padding(start = 14.dp, top = 8.dp, bottom = 2.dp),
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

            if (showReset) {
                PlayerResetRow(onReset = { onSelect(PlayerOptionItem(title = "重置", tag = "reset")) })
            }
        }
    }
}

/** 卡里的普通行（形态 6：icon + 题 16 + 副 11，末行 chevron 钻入）。 */
@Composable
private fun DetailPlainRow(item: PlayerOptionItem, onSelect: () -> Unit) {
    val tall = item.subtitle.isNotBlank()
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height((if (tall) PlayerTokens.panelRowTallHeight else PlayerTokens.detailRowHeight).dp)
            .alpha(if (item.enabled) 1f else 0.4f)
            .pointerInput(item.enabled) {
                if (item.enabled) detectTapGestures(onTap = { onSelect() })
            }
            .padding(horizontal = 14.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        if (item.icon != null) {
            PlayerGlyph(
                kind = item.icon,
                color = PlayerTokens.textPrimary,
                modifier = Modifier
                    .padding(end = 12.dp)
                    .size(PlayerTokens.rowIconSize.dp),
            )
        }
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
        if (item.selected && item.control == PlayerRowControl.CHECK) {
            PlayerGlyph(
                kind = "check",
                color = PlayerTokens.textPrimary,
                modifier = Modifier.size(PlayerTokens.rowCheckSize.dp),
                strokeWidth = 2.2f,
            )
        }
        if (item.control == PlayerRowControl.CHEVRON) {
            PlayerGlyph(
                kind = "chevron-right",
                color = PlayerTokens.chevron,
                modifier = Modifier.size(PlayerTokens.chevronSize.dp),
            )
        }
    }
}