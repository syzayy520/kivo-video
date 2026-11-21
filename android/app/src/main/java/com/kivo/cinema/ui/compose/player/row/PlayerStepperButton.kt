package com.kivo.cinema.ui.compose.player.row

import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.size
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.hapticfeedback.HapticFeedbackType
import androidx.compose.ui.input.pointer.AwaitPointerEventScope
import androidx.compose.ui.input.pointer.PointerInputChange
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalHapticFeedback
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.onClick
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.compose.player.PlayerGlyph
import com.kivo.cinema.ui.compose.player.PlayerPanelStyle
import com.kivo.cinema.ui.compose.player.PressIndicator
import kotlinx.coroutines.withTimeoutOrNull

/** −/+ 按钮：44 命中、32 圆底 + 按下圆 + 16dp glyph；点击/长按连续步进与边界触感。 */
@Composable
internal fun StepperButton(
    direction: Int,
    label: String,
    enabled: Boolean,
    currentValue: () -> String,
    onStep: (Int) -> Unit,
    onStuck: () -> Unit,
) {
    // 与 FAIL-1 同源：手势协程只建一次，回调必须经 rememberUpdatedState 取最新。
    val currentOnStep by rememberUpdatedState(onStep)
    val currentOnStuck by rememberUpdatedState(onStuck)
    val currentValueState by rememberUpdatedState(currentValue)
    var pressed by remember { mutableStateOf(false) }
    val haptic = LocalHapticFeedback.current
    val hapticType = HapticFeedbackType.LongPress
    Box(
        modifier = Modifier
            .size(PlayerPanelStyle.stepperHit)
            .semantics {
                contentDescription = label
                role = Role.Button
                if (enabled) onClick { currentOnStep(direction); true }
            }
            .pointerInput(enabled) {
                if (!enabled) return@pointerInput
                val cancelPx = PlayerPanelStyle.motion.dragCancelDp.dp.toPx()
                awaitEachGesture {
                    val down = awaitFirstDown(requireUnconsumed = false)
                    pressed = true
                    val startValue = currentValueState()
                    when (awaitHoldOrRelease(down, cancelPx, PlayerPanelStyle.motion.holdDelayMs)) {
                        HoldResult.Up -> {
                            pressed = false
                            currentOnStep(direction)
                        }
                        HoldResult.Cancel -> pressed = false
                        HoldResult.Timeout -> {
                            val boundary = repeatSteps(
                                down = down,
                                cancelPx = cancelPx,
                                startValue = startValue,
                                currentValue = currentValueState,
                                step = { currentOnStep(direction) },
                            )
                            pressed = false
                            if (boundary) {
                                haptic.performHapticFeedback(hapticType)
                                currentOnStuck()
                            }
                        }
                    }
                }
            },
        contentAlignment = Alignment.Center,
    ) {
        StepperCircle()
        PressIndicator(
            pressed = pressed,
            diameter = PlayerPanelStyle.pressCircle,
            color = PlayerPanelStyle.stepperPressColor,
        )
        PlayerGlyph(
            kind = if (direction < 0) "minus" else "plus",
            color = PlayerPanelStyle.stepperGlyphColor,
            modifier = Modifier.size(PlayerPanelStyle.stepperGlyphSize),
            strokeWidth = 2.2f,
        )
    }
}

/**
 * 步进按钮的常驻圆底（32dp，白 7% 填充 + 1dp 白 12% 描边）。
 *
 * 【为什么常驻】v1 只有按下才画圆 ⇒ 静止时 −/+ 是两个裸符号，用户看不出
 * "这里是一个可点的按钮"（REX/SenPlayer 都是常驻圆底）。圆底同时给行尾一个
 * 稳定的节奏点，数值列不会和符号糊在一起。
 */
@Composable
private fun StepperCircle() {
    Box(
        modifier = Modifier
            .size(PlayerPanelStyle.pressCircle)
            .drawBehind {
                val hairline = 1.dp.toPx()
                val radius = size.minDimension / 2f - hairline / 2f
                drawCircle(color = PlayerPanelStyle.stepperCircleFill, radius = radius)
                drawCircle(
                    color = PlayerPanelStyle.stepperCircleStroke,
                    radius = radius,
                    style = Stroke(hairline),
                )
            },
    )
}

private enum class HoldResult { Up, Cancel, Timeout }

/** 按下后到 [timeoutMs] 内：抬起 = 单步；位移/被消费 = 取消；超时 = 进入长按连续。 */
private suspend fun AwaitPointerEventScope.awaitHoldOrRelease(
    down: PointerInputChange,
    cancelPx: Float,
    timeoutMs: Long,
): HoldResult {
    val timedOut = withTimeoutOrNull(timeoutMs) {
        var outcome: HoldResult? = null
        while (outcome == null) {
            val event = awaitPointerEvent()
            val change = event.changes.firstOrNull { it.id == down.id }
            outcome = when {
                change == null -> HoldResult.Cancel
                change.isConsumed -> HoldResult.Cancel
                (change.position - down.position).getDistance() > cancelPx -> HoldResult.Cancel
                !change.pressed -> HoldResult.Up
                else -> null
            }
        }
        outcome
    }
    return timedOut ?: HoldResult.Timeout
}

/**
 * 长按连续步进。返回 true = 到边界（连续 3 步值没变）。
 * 每一拍都等下一帧事件：手指抬起/滑走/被滚动接管都会立刻停止，不需要额外监听。
 */
private suspend fun AwaitPointerEventScope.repeatSteps(
    down: PointerInputChange,
    cancelPx: Float,
    startValue: String,
    currentValue: () -> String,
    step: () -> Unit,
): Boolean {
    var elapsed = PlayerPanelStyle.motion.holdDelayMs
    var fired = 0
    while (true) {
        step()
        fired++
        val interval = if (elapsed >= PlayerPanelStyle.motion.holdFastAfterMs) {
            PlayerPanelStyle.motion.holdFastStepMs
        } else {
            PlayerPanelStyle.motion.holdStepMs
        }
        val event = withTimeoutOrNull(interval) { awaitPointerEvent() }
        if (event != null) {
            val change = event.changes.firstOrNull { it.id == down.id } ?: return false
            if (change.isConsumed || !change.pressed) return false
            if ((change.position - down.position).getDistance() > cancelPx) return false
        }
        elapsed += interval
        if (fired % 3 == 0 && currentValue() == startValue) return true
    }
}