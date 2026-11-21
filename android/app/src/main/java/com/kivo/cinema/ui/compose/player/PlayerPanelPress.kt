package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.composed
import androidx.compose.ui.draw.drawWithContent
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.unit.Dp
import com.kivo.cinema.ui.motion.MotionGate

/**
 * 行按压反馈原语（P10 §7 / §10）。
 *
 * 一套实现覆盖三种用法，差量只在参数：
 * · 选择行：6% 白覆盖 + scale 0.985（90ms 进 / 140ms 出）；
 * · 步进/开关行：只做按压圆（[PressIndicator]），行本身不缩放；
 * · 底栏文字钮：28dp 圆白 8% + 文字 alpha 0.6。
 *
 * 【为什么不引 Material ripple】涟漪是圆形扩散 + 有中心点假设，放进整行可点的
 * 玻璃面板里会在行尾裁出一个半圆；参考（iOS 菜单）本身就是"整行按下去变亮一点"。
 *
 * 【FAIL-1 的同款陷阱】`pointerInput` 的闭包只在 key 变化时重建。
 * 这里的 onClick 每帧都是新 lambda（行数据随 panelRevision 重算），
 * 必须 [rememberUpdatedState]，否则记住的是首帧那个 item —— 面板读值后失效。
 */
@Composable
internal fun Modifier.panelPressable(
    enabled: Boolean,
    scaleOnPress: Boolean = true,
    onLongPress: (() -> Unit)? = null,
    onClick: () -> Unit,
): Modifier = composed {
    val reduceMotion = MotionGate.rememberAppReduceMotion()
    var pressed by remember { mutableStateOf(false) }
    val currentOnClick by rememberUpdatedState(onClick)
    val currentOnLongPress by rememberUpdatedState(onLongPress)
    val overlay by animateFloatAsState(
        targetValue = if (pressed) 1f else 0f,
        animationSpec = tween(
            if (pressed) PlayerPanelStyle.motion.pressInMs else PlayerPanelStyle.motion.pressOutMs,
            easing = PlayerPanelStyle.motion.rowEase,
        ),
        label = "rowPressOverlay",
    )
    val scale by animateFloatAsState(
        targetValue = if (pressed && scaleOnPress && !reduceMotion) PlayerPanelStyle.pressScale else 1f,
        animationSpec = tween(
            if (pressed) PlayerPanelStyle.motion.pressInMs else PlayerPanelStyle.motion.pressOutMs,
            easing = PlayerPanelStyle.motion.rowEase,
        ),
        label = "rowPressScale",
    )
    this
        .graphicsLayer {
            scaleX = scale
            scaleY = scale
        }
        .pointerInput(enabled) {
            if (!enabled) return@pointerInput
            detectTapGestures(
                onPress = {
                    pressed = true
                    tryAwaitRelease()
                    pressed = false
                },
                onTap = { currentOnClick() },
                onLongPress = currentOnLongPress?.let { { it() } },
            )
        }
        .drawWithContent {
            drawContent()
            if (overlay > 0f) {
                // 与卡圆角同半径：行贴到卡顶/卡底时，按压色不能压出圆角变成方角。
                drawRoundRect(
                    color = PlayerPanelStyle.pressScrim.copy(
                        alpha = PlayerPanelStyle.pressScrim.alpha * overlay,
                    ),
                    cornerRadius = CornerRadius(PlayerPanelStyle.radius.toPx()),
                )
            }
        }
}

/**
 * 按压圆指示（32dp 步进 / 28dp 底栏）：默认白 8%（控制栏），步进传白 12%。
 * 90ms 进 / 140ms 出。由调用方负责摆放（通常是 glyph 背后一层），本组件只画圆。
 */
@Composable
internal fun PressIndicator(
    pressed: Boolean,
    diameter: Dp = PlayerPanelStyle.pressCircle,
    color: Color = PlayerPanelStyle.pressCircleColor,
    modifier: Modifier = Modifier,
) {
    val alpha by animateFloatAsState(
        targetValue = if (pressed) 1f else 0f,
        animationSpec = tween(
            if (pressed) PlayerPanelStyle.motion.pressInMs else PlayerPanelStyle.motion.pressOutMs,
            easing = PlayerPanelStyle.motion.rowEase,
        ),
        label = "pressCircle",
    )
    if (alpha <= 0.01f) return
    Box(
        modifier = modifier
            .size(diameter)
            .graphicsLayer { this.alpha = alpha }
            .background(color, CircleShape),
    )
}