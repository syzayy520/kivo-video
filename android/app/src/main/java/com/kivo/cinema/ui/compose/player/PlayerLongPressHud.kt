package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.scaleIn
import androidx.compose.animation.scaleOut
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 长按快进的 **HUD**（"倍速标签淡入"，标签 0.9→1，进入 0.2s / 退出 0.15s）。
 *
 * ## 参考的行为链
 * `长按 → 标签淡入 →（左右滑调速）→（上滑锁定）→ 松手恢复`
 *
 * 标签显示当前快进速度；**锁定**后额外显示一个挂锁图标
 * （提示"松手不会恢复"）。
 */
@Composable
internal fun PlayerLongPressHud(
    speedLabel: String,
    locked: Boolean,
    modifier: Modifier = Modifier,
) {
    AnimatedVisibility(
        visible = speedLabel.isNotBlank(),
        enter = fadeIn(tween(200)) + scaleIn(tween(200), initialScale = 0.9f),
        exit = fadeOut(tween(150)) + scaleOut(tween(150), targetScale = 0.9f),
        modifier = modifier,
    ) {
        val scale by animateFloatAsState(
            targetValue = 1f,
            animationSpec = tween(150),
            label = "longPressScale",
        )
        Row(
            modifier = Modifier
                .playerGlassSurface(elevation = 14.dp)
                .padding(horizontal = 22.dp, vertical = 14.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(10.dp),
        ) {
            Text(
                text = speedLabel,
                color = PlayerTokens.textPrimary,
                fontSize = 26.sp,
                maxLines = 1,
            )
            if (locked) {
                PlayerGlyph(
                    kind = "lock",
                    color = PlayerTokens.activeAccent,
                    modifier = Modifier.size(20.dp),
                )
            }
        }
    }
}
