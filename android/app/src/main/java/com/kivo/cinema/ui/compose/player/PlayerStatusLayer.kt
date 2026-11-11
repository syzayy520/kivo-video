package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 播放页状态层（player-ui-v2.md §3.5）。
 *
 * **存在的理由**：现有实现把 `statusText` 用一个 10sp 黑底白字浮在**左上角** ——
 * 那是调试残留，产品界面里像日志。状态必须有自己的**专门层**，
 * 且形态要与参考一致：
 * · 加载/缓冲 → **中央三点波纹**（不是 Material 的转圈 spinner —— 转圈太重，
 *   在暗画面上抢视线；三点波纹安静，且"在动"本身就是"还在等"的表达）；
 * · 失败 → 居中面板（图标 + 原因 + 重试）；
 * · 网络中断 → 轻提示，落在底部渐变区里，**不挡画面中央**。
 */
@Composable
internal fun PlayerStatusLayer(
    buffering: Boolean,
    errorText: String,
    onRetry: () -> Unit,
    modifier: Modifier = Modifier,
) {
    if (buffering && errorText.isBlank()) {
        BufferingRipple(modifier)
        return
    }
    if (errorText.isNotBlank()) {
        Box(modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                modifier = Modifier
                    .padding(horizontal = 40.dp)
                    .clip(RoundedCornerShape(PlayerTokens.panelRadius.dp))
                    .background(PlayerTokens.panelSurface)
                    .padding(horizontal = 24.dp, vertical = 20.dp),
            ) {
                Text(
                    text = "播放失败",
                    color = PlayerTokens.textPrimary,
                    fontSize = PlayerTokens.panelTitleSize.sp,
                )
                Spacer(Modifier.height(6.dp))
                Text(
                    text = errorText,
                    color = PlayerTokens.textSecondary,
                    fontSize = PlayerTokens.panelSubtitleSize.sp,
                    textAlign = TextAlign.Center,
                    maxLines = 3,
                )
                Spacer(Modifier.height(16.dp))
                Box(
                    modifier = Modifier
                        .clip(RoundedCornerShape(PlayerTokens.panelRadius.dp))
                        .background(Color.White)
                        .clickable(onClick = onRetry)
                        .padding(horizontal = 22.dp, vertical = 9.dp),
                ) {
                    Text(
                        text = "重试",
                        color = Color.Black,
                        fontSize = PlayerTokens.panelTitleSize.sp,
                    )
                }
            }
        }
    }
}

/**
 * 中央三点波纹（REX 的加载指示）。
 *
 * 三个点**错相**做缩放与透明度变化 —— 这是"呼吸"而不是"跳动"，
 * 视觉噪音比 spinner 小得多。
 */
@Composable
private fun BufferingRipple(modifier: Modifier = Modifier) {
    val transition = rememberInfiniteTransition(label = "buffering")
    val phases = listOf(0f, 140f, 280f)
    val scales = phases.map { phase ->
        transition.animateFloat(
            initialValue = 0.6f,
            targetValue = 1f,
            animationSpec = infiniteRepeatable(
                animation = tween(
                    durationMillis = 900,
                    delayMillis = phase.toInt(),
                    easing = androidx.compose.animation.core.FastOutSlowInEasing,
                ),
                repeatMode = RepeatMode.Restart,
            ),
            label = "dot$phase",
        )
    }
    val alphas = phases.map { phase ->
        transition.animateFloat(
            initialValue = 0.35f,
            targetValue = 1f,
            animationSpec = infiniteRepeatable(
                animation = tween(durationMillis = 900, delayMillis = phase.toInt()),
                repeatMode = RepeatMode.Restart,
            ),
            label = "alpha$phase",
        )
    }

    Box(modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
        Row(horizontalArrangement = Arrangement.spacedBy(9.dp)) {
            scales.forEachIndexed { index, scale ->
                val alpha by alphas[index]
                Box(
                    modifier = Modifier
                        .size(9.dp)
                        .graphicsLayer {
                            scaleX = scale.value
                            scaleY = scale.value
                        }
                        .clip(CircleShape)
                        .background(Color.White.copy(alpha = alpha)),
                )
            }
        }
    }
}
