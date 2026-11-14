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
 * 播放页状态层（272：阶段制）。
 *
 * **存在的理由**：现有实现把 `statusText` 用一个 10sp 黑底白字浮在**左上角** ——
 * 那是调试残留，产品界面里像日志。状态必须有自己的**专门层**。
 *
 * ## 阶段 → 形态
 * 272 之前这些形态写好了，却因为判定靠文案子串而基本没出现过：
 * · [PlayerPhase.Buffering] → **中央三点波纹**（不是 Material 的转圈 spinner —— 转圈太重，
 *   在暗画面上抢视线；三点波纹安静，且"在动"本身就是"还在等"的表达）；
 * · [PlayerPhase.Failed] → 居中面板（原因 + 重试）；
 * · [PlayerPhase.Reconnecting] → **底部轻提示**，不挡画面中央
 *   （重连只需等待，不该像失败那样抢注意力）；
 * · [PlayerPhase.Ended] → 居中「播放结束」。
 *
 * 【为什么参数是阶段而不是两个布尔/字符串】两个布尔会让"缓冲中同时失败"这类
 * 组合无定义，字符串则把状态和文案绑死（改一个字就改坏一个判定）。
 */
@Composable
internal fun PlayerStatusLayer(
    phase: PlayerPhase,
    onRetry: () -> Unit,
    modifier: Modifier = Modifier,
) {
    when (phase) {
        PlayerPhase.Buffering -> BufferingRipple(modifier)
        is PlayerPhase.Failed -> FailedPanel(phase.reason, onRetry, modifier)
        PlayerPhase.Reconnecting -> ReconnectHint(modifier)
        PlayerPhase.Ended -> EndedPanel(modifier)
        PlayerPhase.Idle, PlayerPhase.Playing -> Unit
    }
}

/** 致命错误：原因（必须非空）+ 重试。 */
@Composable
private fun FailedPanel(reason: String, onRetry: () -> Unit, modifier: Modifier = Modifier) {
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
                text = reason,
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

/**
 * 网络中断重连：底部轻提示。
 *
 * 之前这条被误判成"失败"（文案里带"中断"两个字）⇒ 自动重连时弹居中失败面板。
 * 重连只需要等待，不该像失败那样抢画面中央的注意力。
 */
@Composable
private fun ReconnectHint(modifier: Modifier = Modifier) {
    Box(modifier.fillMaxSize(), contentAlignment = Alignment.BottomCenter) {
        Text(
            text = "网络中断，正在重连…",
            color = PlayerTokens.textPrimary,
            fontSize = 13.sp,
            modifier = Modifier
                .padding(bottom = PlayerTokens.bottomScrimHeight.dp + 12.dp)
                .clip(RoundedCornerShape(50))
                .background(PlayerTokens.panelSurface)
                .padding(horizontal = 16.dp, vertical = 8.dp),
        )
    }
}

/** 播完：告知结束（"重播 / 下一集"入口属后续批次）。 */
@Composable
private fun EndedPanel(modifier: Modifier = Modifier) {
    Box(modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
        Text(
            text = "播放结束",
            color = PlayerTokens.textPrimary,
            fontSize = PlayerTokens.panelTitleSize.sp,
            modifier = Modifier
                .clip(RoundedCornerShape(PlayerTokens.panelRadius.dp))
                .background(PlayerTokens.panelSurface)
                .padding(horizontal = 22.dp, vertical = 14.dp),
        )
    }
}

/**
 * 中央三点波纹（加载指示）。
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
