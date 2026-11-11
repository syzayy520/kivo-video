package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp

/**
 * 上下滑手势的 **HUD**（参考 M03-S03）。
 *
 * ## 参考的行为
 * "滑动→HUD→停止 1s→淡出"，淡入 0.15s / 淡出 0.3s；
 * **左右分区**：左半屏亮度、右半屏音量；退出要**恢复系统亮度**。
 *
 * ## 我们的实现
 * · HUD 是一条**竖向**的圆角条 + 图标，贴在触发那一侧；
 * · 停止滑动 1 秒后自动淡出；
 * · 亮度用**窗口级** `screenBrightness`（不改系统全局亮度，
 *   免得退出播放器后整台设备都暗着 —— 参考明确要求"退出需恢复系统亮度"）。
 */
@Composable
internal fun PlayerGestureHud(
    kind: PlayerHudKind,
    fraction: Float,
    modifier: Modifier = Modifier,
) {
    val alpha by animateFloatAsState(
        targetValue = 1f,
        animationSpec = tween(150),
        label = "hudAlpha",
    )
    Box(
        modifier = modifier
            .width(92.dp)
            .height(200.dp)
            .clip(RoundedCornerShape(46.dp))
            .background(Color(0x99000000).copy(alpha = Color(0x99000000).alpha * alpha)),
        contentAlignment = Alignment.Center,
    ) {
        // 竖向进度条
        Canvas(
            modifier = Modifier
                .width(8.dp)
                .fillMaxHeight()
                .padding(vertical = 18.dp),
        ) {
            val radius = CornerRadius(size.width / 2f)
            // 底槽
            drawRoundRect(
                color = Color.White.copy(alpha = 0.22f * alpha),
                topLeft = Offset(0f, 0f),
                size = Size(size.width, size.height),
                cornerRadius = radius,
            )
            // 已调到的部分（从下往上）
            val h = size.height * fraction.coerceIn(0f, 1f)
            drawRoundRect(
                color = Color.White.copy(alpha = 0.92f * alpha),
                topLeft = Offset(0f, size.height - h),
                size = Size(size.width, h),
                cornerRadius = radius,
            )
        }
        // 图标
        PlayerGlyph(
            kind = when (kind) {
                PlayerHudKind.BRIGHTNESS -> "sun"
                PlayerHudKind.VOLUME -> "volume"
            },
            color = Color.White.copy(alpha = alpha),
            modifier = Modifier.size(28.dp),
        )
    }
}

/** HUD 的种类：亮度 / 音量。 */
internal enum class PlayerHudKind { BRIGHTNESS, VOLUME }
