package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.composed
import androidx.compose.ui.draw.drawWithContent
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Shape
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp

/**
 * 播放页的**玻璃面板材质**（一个实现，面板/提示共用）。
 *
 * ## 为什么单独一个文件
 * "面板看起来廉价"不是配色问题，是**材质缺层次**：
 * 纯色块贴在画面上像一张贴纸。参考（REX）的弹层是**毛玻璃**：
 * 半透明底 + 顶部高光边 + 背景模糊。
 * Android 12+ 的 `RenderEffect` 只能模糊**自己这一层**，做不出"背景模糊"，
 * 所以这里用**三层叠加**把质感做出来：
 * 1. **投影** —— 面板从画面上"浮"起来，而不是贴着；
 * 2. **垂直渐变底**（上略亮 → 下更深）—— 平色块立刻变"有厚度"；
 * 3. **顶部高光边**（上白下淡的描边渐变）—— iOS 玻璃的标志性细节。
 *
 * 另外把不透明度提到 97%：背景漏出来会让面板显脏（参考是 75% + 模糊，
 * 我们没有模糊就不能再透）。
 */
@Composable
internal fun Modifier.playerGlassSurface(
    shape: Shape = RoundedCornerShape(PlayerTokens.panelRadius.dp),
    elevation: Dp = 18.dp,
): Modifier = composed {
    val radius = PlayerTokens.panelRadius.dp
    this
        .shadow(
            elevation = elevation,
            shape = shape,
            ambientColor = Color.Black,
            spotColor = Color.Black,
            clip = false,
        )
        .background(
            brush = Brush.verticalGradient(
                listOf(PlayerTokens.glassTop, PlayerTokens.glassBottom),
            ),
            shape = shape,
        )
        .drawWithContent {
            drawContent()
            // 顶部高光边：一条"上白下淡"的渐变描边，只画上半段。
            drawRoundRect(
                brush = Brush.verticalGradient(
                    0f to Color.White.copy(alpha = 0.20f),
                    0.45f to Color.White.copy(alpha = 0.05f),
                    1f to Color.White.copy(alpha = 0f),
                    startY = 0f,
                    endY = size.height * 0.55f,
                ),
                size = size,
                cornerRadius = CornerRadius(radius.toPx()),
                style = Stroke(width = 1.dp.toPx()),
            )
        }
}
