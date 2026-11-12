package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.RowScope
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
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 微调卡里的三种行（v3 §1.4 形态 5/9）：**−/+ pill 行、开关行、重置行**。
 *
 * 实测：行高 50pt、题 16pt、pill 93×30pt（−/+ 各半、中间一道竖分隔）、
 * 重置行居中。pill 与开关都自己画 —— Material 的 Switch/Slider 是另一套语言，
 * 放进 iOS 玻璃卡里会立刻看出"不是同一个 app"。
 */
@Composable
internal fun PlayerStepperRow(
    item: PlayerOptionItem,
    onStep: (Int) -> Unit,
    modifier: Modifier = Modifier,
) {
    Row(
        modifier = modifier
            .fillMaxWidth()
            .height(PlayerTokens.detailRowHeight.dp)
            .alpha(if (item.enabled) 1f else 0.4f)
            .padding(horizontal = 14.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(
            text = item.title,
            color = PlayerTokens.textPrimary,
            fontSize = PlayerTokens.panelTitleSize.sp,
            maxLines = 1,
            modifier = Modifier.weight(1f),
        )
        Text(
            text = item.value,
            color = PlayerTokens.textSecondary,
            fontSize = PlayerTokens.sourceMetaSize.sp,
            maxLines = 1,
            modifier = Modifier.padding(end = 12.dp),
        )
        Row(
            modifier = Modifier
                .width(PlayerTokens.stepperWidth.dp)
                .height(PlayerTokens.stepperHeight.dp)
                .clip(RoundedCornerShape(PlayerTokens.stepperHeight.dp / 2))
                .background(PlayerTokens.chipSurface),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            StepperHalf(glyph = "minus", enabled = item.enabled) { onStep(-1) }
            Box(
                modifier = Modifier
                    .width(1.dp)
                    .height(16.dp)
                    .background(Color.White.copy(alpha = 0.2f)),
            )
            StepperHalf(glyph = "plus", enabled = item.enabled) { onStep(1) }
        }
    }
}

@Composable
private fun RowScope.StepperHalf(glyph: String, enabled: Boolean, onClick: () -> Unit) {
    Box(
        modifier = Modifier
            .weight(1f)
            .height(PlayerTokens.stepperHeight.dp)
            .pointerInput(enabled) {
                // 按下即步进（iOS UIStepper 语义，REX 同源）：不等 tap 识别窗，
                // 真机实测偶发"点了没反应"集中在 tap 判定上，press 通道无此问题。
                if (enabled) detectTapGestures(onPress = { onClick() })
            },
        contentAlignment = Alignment.Center,
    ) {
        PlayerGlyph(
            kind = glyph,
            color = PlayerTokens.textPrimary,
            modifier = Modifier.size(14.dp),
            strokeWidth = 2.2f,
        )
    }
}

/** 开关行（形态 9 的"固定转滚动"）。 */
@Composable
internal fun PlayerToggleRow(
    item: PlayerOptionItem,
    onToggle: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Row(
        modifier = modifier
            .fillMaxWidth()
            .height(PlayerTokens.detailRowHeight.dp)
            .alpha(if (item.enabled) 1f else 0.4f)
            .pointerInput(item.enabled) {
                if (item.enabled) detectTapGestures(onTap = { onToggle() })
            }
            .padding(horizontal = 14.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(
            text = item.title,
            color = PlayerTokens.textPrimary,
            fontSize = PlayerTokens.panelTitleSize.sp,
            maxLines = 1,
            modifier = Modifier.weight(1f),
        )
        PlayerTogglePill(on = item.on)
    }
}

/** 开关 pill（46×28pt）：微调卡与玻璃列表共用同一个视觉零件。 */
@Composable
internal fun PlayerTogglePill(on: Boolean, modifier: Modifier = Modifier) {
    Box(
        modifier = modifier
            .width(46.dp)
            .height(28.dp)
            .clip(RoundedCornerShape(14.dp))
            .background(
                if (on) PlayerTokens.textPrimary else Color.White.copy(alpha = 0.2f),
            ),
        contentAlignment = if (on) Alignment.CenterEnd else Alignment.CenterStart,
    ) {
        Box(
            modifier = Modifier
                .padding(2.dp)
                .size(24.dp)
                .clip(CircleShape)
                .background(if (on) Color(0xFF1C1C1E) else Color.White),
        )
    }
}

/** 重置行：居中文字按钮（实测在卡底居中）。 */
@Composable
internal fun PlayerResetRow(onReset: () -> Unit, modifier: Modifier = Modifier) {
    Box(
        modifier = modifier
            .fillMaxWidth()
            .height(PlayerTokens.minTouchTarget.dp)
            .pointerInput(Unit) { detectTapGestures(onTap = { onReset() }) },
        contentAlignment = Alignment.Center,
    ) {
        Text(
            text = "重置",
            color = PlayerTokens.chevron,
            fontSize = PlayerTokens.textButtonSize.sp,
            textAlign = TextAlign.Center,
        )
    }
}