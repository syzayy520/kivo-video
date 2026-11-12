package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.State
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.LineHeightStyle
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 底栏的**文字件**（v3.1 自 [PlayerBottomBar] 拆出，守单文件 ≤300 行）。
 *
 * 【字形下缘被裁的根因与修法（2026-11-12 真机实证）】
 * 本机 CJK 字体的 ink 超出字盛行盒（ascent+descent≈1em），material3 Text
 * 默认 `overflow = Clip` 沿布局边界裁切 ⇒「1080P / 选集 / 倍速」下缘被吃。
 * lineHeight 参数在这台机器上**撑不大布局盒**（uiautomator 实测恒 38px），
 * 唯一稳的修法是 `overflow = TextOverflow.Visible`；
 * 1.25em 行盒 + Center/Trim.Both 只负责与播放键同中心。
 * 证据：docs/evidence/p240/player-ui/v3/text-clip/。
 */

/** 时间：13pt semibold 等宽（实测 ink 9.3pt）。 */
@Composable
internal fun TimeText(text: String, modifier: Modifier = Modifier) {
    Text(
        text = text,
        color = PlayerTokens.textPrimary,
        fontSize = PlayerTokens.timeSize.sp,
        lineHeight = (PlayerTokens.timeSize * 1.25f).sp,
        style = TextStyle(
            lineHeightStyle = LineHeightStyle(
                alignment = LineHeightStyle.Alignment.Center,
                trim = LineHeightStyle.Trim.Both,
            ),
        ),
        fontFamily = FontFamily.Monospace,
        fontWeight = FontWeight.SemiBold,
        maxLines = 1,
        overflow = TextOverflow.Visible,
        modifier = modifier,
    )
}

/** 右排文字钮（1080P / 选集 / 倍速）：命中区随文字 + 内边距，被面板盖住时隐去。 */
@Composable
internal fun BarTextButton(
    text: String,
    active: Boolean,
    panelRect: State<Rect?>,
    onClick: () -> Unit,
) {
    Text(
        text = text,
        color = if (active) PlayerTokens.activeAccent else PlayerTokens.textPrimary,
        fontSize = PlayerTokens.textButtonSize.sp,
        lineHeight = (PlayerTokens.textButtonSize * 1.25f).sp,
        style = TextStyle(
            lineHeightStyle = LineHeightStyle(
                alignment = LineHeightStyle.Alignment.Center,
                trim = LineHeightStyle.Trim.Both,
            ),
        ),
        maxLines = 1,
        overflow = TextOverflow.Visible,
        modifier = Modifier
            .pointerInput(text) { detectTapGestures(onTap = { onClick() }) }
            .padding(horizontal = PlayerTokens.textButtonPad.dp, vertical = 5.dp)
            .hideWhenCoveredByPanel(panelRect),
    )
}

/**
 * 时间格式化：`mm:ss`（不足 1 小时）或 `h:mm:ss`。
 * 分钟补零 —— 参考显示 `00:08` / `45:28`，`0:08` 看着像没对齐。
 */
internal fun formatPlayerTime(ms: Long): String {
    val totalSec = (ms / 1000).coerceAtLeast(0)
    val h = totalSec / 3600
    val m = (totalSec % 3600) / 60
    val s = totalSec % 60
    return if (h > 0) {
        String.format("%d:%02d:%02d", h, m, s)
    } else {
        String.format("%02d:%02d", m, s)
    }
}