package com.kivo.cinema.ui.design

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.sp

/**
 * 统一空图占位（P0-04）：token 底色 + 4% 微光，**不画任何 glyph**。
 *
 * 图片位不是播放器，给空图画播放三角会让整屏都像播放按钮、非常别扭（用户反馈）。
 * 因此默认只保留柔和的浅色底，加载出图后即被覆盖；只有演职员空头像用首字 [initial]。
 *
 * - 零裸色：底色/微光均取自 [DesignTokens]。
 * - 透明底模式：`bg.alpha == 0f` 时不画底色与微光（供 logo 内缩区规避不透明底）。
 */
@Composable
fun PosterPlaceholder(
    modifier: Modifier = Modifier,
    bg: Color = DesignTokens.canvasFill,
    initial: String? = null,
) {
    Box(modifier) {
        if (bg.alpha > 0f) {
            Box(Modifier.fillMaxSize().background(bg))
            // 4% 白微光：canvasFill 在浅态即白，copy(alpha) 避免写死白色字面量。
            Box(Modifier.fillMaxSize().background(DesignTokens.canvasFill.copy(alpha = .04f)))
        }
        if (initial != null) {
            Text(initial, color = DesignTokens.textSub.copy(alpha = .62f), fontSize = 20.sp,
                fontWeight = FontWeight.Medium, modifier = Modifier.align(Alignment.Center))
        }
    }
}
