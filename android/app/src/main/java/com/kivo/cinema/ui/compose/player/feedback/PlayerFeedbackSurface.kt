package com.kivo.cinema.ui.compose.player.feedback

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp

// 手势反馈专用雾面；不采样或模糊视频 Surface。
//
// 【为什么刻意不并入 playerPanelSurface】反馈 HUD 是"手一抬就消失"的瞬时层，
// 多数时候压在亮画面上，需要更高不透明度（62–76%）和更浅的冷灰；
// 设置面板则要透出视频、安静地待在角落。两者共用会逼其中一方妥协，
// 所以只共享"冷调 + 竖直渐变 + 细边"这套语言，数值各自独立。
internal fun Modifier.playerFeedbackSurface(cornerRadius: Dp = 22.dp): Modifier {
    val shape = RoundedCornerShape(cornerRadius)
    return shadow(
        elevation = 10.dp,
        shape = shape,
        clip = false,
        ambientColor = Color.Black.copy(alpha = 0.18f),
        spotColor = Color.Black.copy(alpha = 0.28f),
    )
        .clip(shape)
        .background(
            Brush.verticalGradient(
                listOf(Color(0xD932363E), Color(0xC41B1E25), Color(0xB811141A)),
            ),
        )
        .border(
            width = 0.75.dp,
            brush = Brush.verticalGradient(
                listOf(Color.White.copy(alpha = 0.22f), Color.White.copy(alpha = 0.055f)),
            ),
            shape = shape,
        )
}
