package com.kivo.cinema.ui.compose.player.row

import androidx.compose.animation.AnimatedContent
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.togetherWith
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.compose.player.PlayerOptionItem
import com.kivo.cinema.ui.compose.player.PlayerPanelStyle
import kotlinx.coroutines.delay

/**
 * 微调卡里的三种行（P10 §7）：**步进行 / 开关行 / 重置行**。
 *
 * 步进的四处交互变化（规范允许的两项交互变化之一）：
 * ① **按下改松开触发** —— 旧版按下即写，从 −/+ 起手想滚动会误触；
 * ② 按下后位移 > 8dp 或被滚动接管 ⇒ **取消**；
 * ③ 长按 400ms 后 80ms/步，2s 后 60ms/步；
 * ④ 步进值连续 3 次没变化 ⇒ 判为边界/失败：一次拒绝触感 + 行内「调整失败」1.5s，不弹全局 Toast。
 *
 * 【必须保留的两条】· FAIL-1：闭包用 [rememberUpdatedState]，`pointerInput` 不能拿旧 item；
 * · 不本地累加：每步都用 `item.step`（内核真值）+ delta，由写侧落盘后 revision 重读。
 *
 * 视觉：无胶囊底、无内部竖线；−/+ 各 44 命中、glyph 16dp 白 75%、按下 32dp 圆白 12%（行不缩放）；
 * 数值列固定 64 右对齐、15sp Medium + tnum、变化时 100ms alpha 交叉。
 */
@Composable
internal fun PlayerStepperRow(
    item: PlayerOptionItem,
    onStep: (Int) -> Unit,
    modifier: Modifier = Modifier,
) {
    val currentOnStep by rememberUpdatedState(onStep)
    val latestValue by rememberUpdatedState(item.value)
    // 写侧是异步的（IO 线程写 mpv → revision++ → IO 重读）。1.2s 内值没变 = 这一步没落地。
    var pendingBefore by remember { mutableStateOf("") }
    var requests by remember { mutableIntStateOf(0) }
    var stuck by remember { mutableStateOf(false) }

    val requestStep: (Int) -> Unit = { delta ->
        pendingBefore = latestValue
        requests++
        currentOnStep(delta)
    }

    LaunchedEffect(requests) {
        if (requests == 0) return@LaunchedEffect
        delay(1_200)
        if (latestValue == pendingBefore) stuck = true
    }
    LaunchedEffect(stuck) {
        if (stuck) {
            delay(1_500)
            stuck = false
        }
    }

    Row(
        modifier = modifier
            .fillMaxWidth()
            .height(PlayerPanelStyle.stepperRowHeight)
            .alpha(if (item.enabled) 1f else 0.38f)
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
            // 可选副题（如「底部边距」的"仅文本字幕"）：与开关行同口径；字号不动、行高仍 64。
            if (item.subtitle.isNotBlank()) {
                Text(
                    text = item.subtitle,
                    color = PlayerPanelStyle.rowSubtitleColor,
                    fontSize = PlayerPanelStyle.subtitleSize.sp,
                    lineHeight = PlayerPanelStyle.subtitleLineHeight.sp,
                    letterSpacing = PlayerPanelStyle.subtitleTracking.sp,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )
            }
        }
        StepperValue(
            value = if (stuck) "调整失败" else item.value,
            failed = stuck,
        )
        StepperButton(
            direction = -1,
            label = "减少",
            enabled = item.enabled,
            currentValue = { latestValue },
            onStep = requestStep,
            onStuck = { stuck = true },
        )
        StepperButton(
            direction = 1,
            label = "增加",
            enabled = item.enabled,
            currentValue = { latestValue },
            onStep = requestStep,
            onStuck = { stuck = true },
        )
    }
}

/** 数值列：固定宽 64、右对齐、tnum；值变化 100ms alpha 交叉。 */
@Composable
private fun StepperValue(value: String, failed: Boolean) {
    Box(
        modifier = Modifier
            .width(PlayerPanelStyle.valueColumn)
            .padding(start = 8.dp),
        contentAlignment = Alignment.CenterEnd,
    ) {
        AnimatedContent(
            targetState = value,
            transitionSpec = {
                fadeIn(tween(PlayerPanelStyle.motion.valueFadeMs)) togetherWith
                    fadeOut(tween(PlayerPanelStyle.motion.valueFadeMs))
            },
            contentAlignment = Alignment.CenterEnd,
            label = "stepperValue",
        ) { text ->
            Text(
                text = text,
                color = if (failed) PlayerPanelStyle.toastDot else PlayerPanelStyle.valueColor,
                fontSize = PlayerPanelStyle.valueSize.sp,
                lineHeight = PlayerPanelStyle.valueLineHeight.sp,
                style = TextStyle(fontFeatureSettings = PlayerPanelStyle.valueFontFeature),
                fontWeight = FontWeight.Medium,
                textAlign = TextAlign.End,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis,
            )
        }
    }
}