package com.kivo.cinema.ui.search.voice

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.kivo.cinema.search.VoiceSearchController
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.motion.MotionGate
import com.kivo.cinema.ui.search.SearchTokens

/**
 * Kivo 风语音卡 v2：底部轻面板（family-tree §四·补）。
 * 居中巨卡 → 底部面板：不遮热搜/分类，遮罩减轻；失败给「键盘输入」退路，不死重试。
 */
@Composable
internal fun VoiceSearchDialog(
    controller: VoiceSearchController,
    onCancel: () -> Unit,
    onResult: (String) -> Unit,
    onKeyboardInput: () -> Unit,
) {
    LaunchedEffect(controller.state) {
        if (controller.state == VoiceSearchController.State.Done) {
            controller.result?.takeIf { it.isNotBlank() }?.let(onResult)
        }
    }
    val listening = controller.state == VoiceSearchController.State.Listening
    val processing = controller.state == VoiceSearchController.State.Processing
    val failed = controller.state == VoiceSearchController.State.Failed

    Box(Modifier.fillMaxSize()) {
        Box(
            Modifier.fillMaxSize().background(DesignTokens.textMain.copy(alpha = .32f))
                .clickable(onClick = onCancel),
        )
        Column(
            Modifier.align(Alignment.BottomCenter).navigationBarsPadding()
                .padding(start = 12.dp, end = 12.dp, bottom = 12.dp)
                .fillMaxWidth()
                // 面板背景必须近乎不透明：玻璃太透会把热搜文字穿进声波区（真机评审扣分）。
                .shadow(16.dp, RoundedCornerShape(28.dp))
                .background(DesignTokens.pageBackground.copy(alpha = .97f), RoundedCornerShape(28.dp))
                .border(1.dp, DesignTokens.hairline, RoundedCornerShape(28.dp))
                .clip(RoundedCornerShape(28.dp))
                .clickable(enabled = false) {}
                .padding(start = 22.dp, end = 22.dp, top = 18.dp, bottom = 16.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            Text(
                when {
                    failed -> "没有听清，请再说一次"
                    processing -> "正在识别…"
                    else -> "正在倾听"
                },
                fontSize = 17.sp, fontWeight = FontWeight.SemiBold, color = DesignTokens.textMain,
            )
            Spacer(Modifier.height(14.dp))
            VoiceWaveform(controller.rms, active = listening)
            Spacer(Modifier.height(12.dp))
            Text(
                if (failed) "靠近麦克风，语速稍慢一些；也可以直接键盘输入"
                else "说出片名、演员或导演",
                fontSize = 13.sp, color = DesignTokens.textSub, textAlign = TextAlign.Center,
            )
            Spacer(Modifier.height(18.dp))
            Row(
                Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(12.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(
                    if (failed) "键盘输入" else "取消",
                    fontSize = 14.sp, color = DesignTokens.textMain,
                    modifier = Modifier.clip(RoundedCornerShape(16.dp))
                        .background(SearchTokens.chipFill)
                        .clickable(onClick = if (failed) onKeyboardInput else onCancel)
                        .padding(horizontal = 14.dp, vertical = 10.dp),
                )
                Spacer(Modifier.weight(1f))
                ActionPill(
                    text = when {
                        failed -> "重试"
                        processing -> "识别中…"
                        else -> "说完了"
                    },
                    enabled = listening || failed,
                    modifier = Modifier.weight(1.2f),
                ) {
                    if (failed) controller.start() else controller.finish()
                }
            }
        }
    }
}

@Composable
private fun ActionPill(text: String, enabled: Boolean, modifier: Modifier, onClick: () -> Unit) {
    Box(
        modifier.height(44.dp).clip(RoundedCornerShape(22.dp))
            .background(if (enabled) DesignTokens.textMain else DesignTokens.textSub)
            .clickable(enabled = enabled, onClick = onClick),
        contentAlignment = Alignment.Center,
    ) {
        Text(
            text, fontSize = 15.sp, fontWeight = FontWeight.SemiBold,
            color = DesignTokens.canvasFill,
        )
    }
}

/** 9 条竖条：音量映射高度；未监听/失败时等高灰条（静止）。 */
@Composable
private fun VoiceWaveform(rms: Float, active: Boolean) {
    val reduce = MotionGate.rememberAppReduceMotion()
    Row(
        Modifier.height(56.dp),
        horizontalArrangement = Arrangement.spacedBy(6.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        val weights = floatArrayOf(.45f, .7f, .9f, 1f, .8f, 1f, .9f, .7f, .45f)
        weights.forEachIndexed { index, weight ->
            val target = if (active) 10f + rms * 4f * weight else 12f + (index % 3) * 6f
            val animated by animateFloatAsState(
                targetValue = if (reduce) target.coerceAtMost(40f) else target,
                animationSpec = tween(if (reduce) 0 else 120),
                label = "wave$index",
            )
            Box(
                Modifier.width(5.dp).height(animated.dp.coerceIn(8.dp, 56.dp))
                    .clip(RoundedCornerShape(3.dp))
                    .background(if (active) DesignTokens.textMain else DesignTokens.textSub),
            )
        }
    }
}