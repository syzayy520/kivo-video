package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.background
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
 * 播放页的**玻璃材质**（v3 §1.3，一个实现，四种壳共用）。
 *
 * 实测：黑 ≈75% + 背景模糊、圆角 ≈16pt、描边/分隔线白 8%。
 *
 * 【为什么不是纯色块】Android 12+ 的 `RenderEffect` 只能模糊**自己这一层**，
 * 做不出"模糊背后的视频"。所以用三层把质感补出来：
 * ① 投影（面板从画面上浮起来）② 黑 75% 底（实测值，不再用 v2 的 97% 灰渐变）
 * ③ 顶部高光 + 白 8% 描边（iOS 玻璃的标志性细节）。
 *
 * 【为什么 v2 的 97% 被推翻】v2 怕"背景漏出来显脏"把不透明度拉到 97%，
 * 结果面板成了一块实心黑贴纸 —— 与参考的通透感正好相反。
 * 75% 是实测值，脏不脏由**描边和高光**解决，不是靠加黑。
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
        .background(PlayerTokens.panelSurface, shape)
        .drawWithContent {
            drawContent()
            val corner = CornerRadius(radius.toPx())
            // 顶部高光：上白下淡，只画上半段。
            drawRoundRect(
                brush = Brush.verticalGradient(
                    0f to PlayerTokens.glassSheen,
                    1f to Color.Transparent,
                    startY = 0f,
                    endY = size.height * 0.5f,
                ),
                size = size,
                cornerRadius = corner,
            )
            drawRoundRect(
                color = PlayerTokens.panelBorder,
                size = size,
                cornerRadius = corner,
                style = Stroke(width = 1.dp.toPx()),
            )
        }
}