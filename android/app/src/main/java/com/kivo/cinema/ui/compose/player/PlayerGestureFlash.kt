package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.scale
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp

/**
 * 双击手势的**闪示图标**（REX M03-S01："图标闪现 0.5"，0→1→0）。
 *
 * 【为什么要它】双击快进/快退时画面在动，用户**看不出这次点击干了什么**。
 * REX 用一个居中的图标闪 0.5 秒来确认 —— 这是"零成本的确认"，不打断观看。
 *
 * 【实现】用 [LaunchedEffect] 计时 500ms 后自动隐藏；
 * 出现/消失用透明度+缩放过渡（REX 的 easeOut 族）。
 */
@Composable
internal fun PlayerGestureFlash(
    zone: DoubleTapZone?,
    modifier: Modifier = Modifier,
) {
    var showing by remember { mutableStateOf(false) }
    var icon by remember { mutableStateOf<DoubleTapZone?>(null) }

    LaunchedEffect(zone) {
        if (zone == null) {
            showing = false
            return@LaunchedEffect
        }
        icon = zone
        showing = true
        kotlinx.coroutines.delay(500)
        showing = false
    }

    if (icon == null) return
    val scale by animateFloatAsState(
        targetValue = if (showing) 1f else 0.86f,
        animationSpec = tween(150),
        label = "flashScale",
    )
    val alpha by animateFloatAsState(
        targetValue = if (showing) 1f else 0f,
        animationSpec = tween(150),
        label = "flashAlpha",
    )

    Box(
        modifier = modifier
            .size(72.dp)
            .scale(scale)
            .clip(CircleShape)
            .background(flashScrim.copy(alpha = flashScrim.alpha * alpha)),
        contentAlignment = Alignment.Center,
    ) {
        PlayerGlyph(
            kind = when (icon) {
                DoubleTapZone.FORWARD -> "forward"
                DoubleTapZone.BACKWARD -> "backward"
                else -> "pause"
            },
            color = Color.White.copy(alpha = alpha),
            modifier = Modifier.size(34.dp),
        )
    }
}
