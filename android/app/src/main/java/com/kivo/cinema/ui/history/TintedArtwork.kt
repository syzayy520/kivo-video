package com.kivo.cinema.ui.history

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxScope
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.blur
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.drawWithContent
import androidx.compose.ui.graphics.BlendMode
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.CompositingStrategy
import androidx.compose.ui.graphics.Shape
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.Dp

import androidx.compose.ui.unit.sp
import com.kivo.cinema.ui.design.DesignTokens

/**
 * 取色模糊渐变（视觉稿 tint.css）：剧照只画一次为底图，底部叠它的模糊副本 + 自下而上的渐暗层，
 * 颜色来自画面本身，不另外取色。
 *
 * [artwork] 为 null 时画首字母占位（诚实空态，不伪造缩略图）。
 * API < 31 `Modifier.blur` 无效，自动退化为只剩渐暗层 —— 白字仍可读（家族树 §2）。
 */
@Composable
internal fun TintedArtwork(
    artwork: (@Composable () -> Unit)?,
    fallbackLetter: String,
    veilFraction: Float,
    blurDp: Dp,
    shape: Shape,
    modifier: Modifier = Modifier,
    content: @Composable BoxScope.() -> Unit = {},
) {
    Box(modifier.clip(shape)) {
        ArtworkLayer(artwork, fallbackLetter, Modifier.fillMaxSize())
        // 底部渐层区：高 = 容器高 × veilFraction（大卡 74% / 缩略图 72%，视觉稿 hero .veil / .thumb .veil）
        Box(
            Modifier
                .fillMaxWidth()
                .fillMaxHeight(veilFraction)
                .align(Alignment.BottomCenter),
        ) {
            // 模糊副本 + 垂直 mask（视觉稿 veil::before 的 mask: to top #000 40% → transparent）：
            // 上部无模糊、底部渐进显现 —— 海报上半部保持清晰，不被「玻璃罩」盖住。
            // Offscreen 合成 + DstIn 是 Compose 标准 mask 手法；mask 只看 alpha，用 canvasFill 作源。
            if (artwork != null) Box(
                Modifier.fillMaxSize()
                    .graphicsLayer { compositingStrategy = CompositingStrategy.Offscreen }
                    .drawWithContent {
                        drawContent()
                        drawRect(
                            Brush.verticalGradient(
                                0f to Color.Transparent,
                                .55f to DesignTokens.canvasFill,
                                1f to DesignTokens.canvasFill,
                            ),
                            blendMode = BlendMode.DstIn,
                        )
                    }
                    .blur(blurDp),
            ) { ArtworkLayer(artwork, fallbackLetter, Modifier.fillMaxSize()) }
            // 渐暗层：减淡版 v2（.22/.50），颜色由模糊副本透出，近黑只保白字可读、不压画面层次
            Box(
                Modifier.fillMaxSize().background(
                    Brush.verticalGradient(
                        0f to DesignTokens.textMain.copy(alpha = 0f),
                        .45f to DesignTokens.textMain.copy(alpha = .22f),
                        1f to DesignTokens.textMain.copy(alpha = .50f),
                    ),
                ),
            )
            content()
        }
    }
}

@Composable
private fun ArtworkLayer(
    artwork: (@Composable () -> Unit)?,
    fallbackLetter: String,
    modifier: Modifier = Modifier,
) {
    if (artwork != null) {
        Box(modifier) { artwork() }
    } else {
        // 诚实空态：中性深底 + 首字母（视觉稿 .letter），不伪造缩略图
        Column(
            modifier.background(DesignTokens.textMain.copy(alpha = .55f)),
            verticalArrangement = Arrangement.Center,
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            Text(
                text = fallbackLetter,
                color = DesignTokens.canvasFill,
                fontSize = 26.sp,
                fontWeight = FontWeight.SemiBold,
            )
        }
    }
}