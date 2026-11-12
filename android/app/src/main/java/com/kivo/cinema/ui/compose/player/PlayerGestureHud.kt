package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 上下滑手势的 **HUD**（参考 M03-S03）。
 *
 * ## 形态：**中上部的横向药丸**
 * 一行显示 `[图标]  45%`，下面一条细进度线表示当前值。
 * 亮度/音量是**数值型**信息，用户要读的是"多少"，不是"占多高"；
 * 横向药丸不遮画面主体，数字可以直接读出来。
 *
 * ## 动效（M03-S03：淡入 0.15 / 停止 1s / 淡出 0.3）
 * 进出由本组件自己的 [AnimatedVisibility] 负责（与 [PlayerLongPressHud] 同构）：
 * 淡入 [PlayerMotion.hudEnterMs]、淡出 [PlayerMotion.hudExitMs]，纯 alpha ⇒ 天然满足 G1。
 * 停留 1s 的计时在 `PlayerGestureRouter.onVerticalDragEnd`（[PlayerMotion.hudHoldMs]）。
 *
 * 【为什么不再用 animateFloatAsState】旧版 `targetValue = 1f` 是常量，
 * 首帧即等于目标 ⇒ 根本不产生动画，HUD 是"硬闪进硬闪出"。改成可见性驱动后，
 * [visible] 翻假时整棵子树按 300ms 淡出，而不是被 `if` 直接从树上摘掉。
 *
 * 左右分区：左半屏亮度、右半屏音量；窗口级亮度退出即失效（见 Router）。
 */
@Composable
internal fun PlayerGestureHud(
    visible: Boolean,
    kind: PlayerHudKind?,
    fraction: Float,
    modifier: Modifier = Modifier,
) {
    // 淡出的 300ms 里 kind 已变 null，不保留最后一种就会瞬间换成空图标再淡出。
    var lastKind by remember { mutableStateOf(PlayerHudKind.BRIGHTNESS) }
    LaunchedEffect(kind) { if (kind != null) lastKind = kind }
    val shownKind = kind ?: lastKind

    AnimatedVisibility(
        visible = visible,
        enter = fadeIn(tween(PlayerMotion.hudEnterMs, easing = PlayerMotion.easing)),
        exit = fadeOut(tween(PlayerMotion.hudExitMs, easing = PlayerMotion.easing)),
        modifier = modifier,
    ) {
        Column(
            modifier = Modifier
                .width(208.dp)
                .clip(RoundedCornerShape(20.dp))
                .background(Color(0xB3000000))
                .padding(horizontal = 18.dp, vertical = 12.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(10.dp),
            ) {
                PlayerGlyph(
                    kind = when (shownKind) {
                        PlayerHudKind.BRIGHTNESS -> "sun"
                        PlayerHudKind.VOLUME -> "volume"
                    },
                    color = Color.White,
                    modifier = Modifier.size(22.dp),
                )
                Text(
                    text = "${(fraction.coerceIn(0f, 1f) * 100).toInt()}%",
                    color = Color.White,
                    fontSize = 22.sp,
                    maxLines = 1,
                )
            }
            // 细进度线：数值的位置感
            Canvas(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(3.dp)
                    .padding(top = 8.dp),
            ) {
                val radius = CornerRadius(size.height / 2f)
                drawRoundRect(
                    color = Color.White.copy(alpha = 0.22f),
                    topLeft = Offset(0f, 0f),
                    size = Size(size.width, size.height),
                    cornerRadius = radius,
                )
                drawRoundRect(
                    color = Color.White.copy(alpha = 0.92f),
                    topLeft = Offset(0f, 0f),
                    size = Size(size.width * fraction.coerceIn(0f, 1f), size.height),
                    cornerRadius = radius,
                )
            }
        }
    }
}

/** HUD 的种类：亮度 / 音量。 */
internal enum class PlayerHudKind { BRIGHTNESS, VOLUME }
