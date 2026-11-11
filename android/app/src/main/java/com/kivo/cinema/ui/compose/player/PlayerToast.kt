package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 播放页的**轻提示**（REX 交互 M08-03）。
 *
 * 【为什么要它】铁律：不许静默降级。
 * 任何"点了但做不了"的功能（下一集没接资源库、画质比例暂时不支持…）
 * 都必须给用户**看得见的回应**，而不是毫无反应。
 * 用浮层提示而不是 Toast 弹窗：Toast 会打断观感，且在横屏下位置别扭。
 *
 * 【动效】淡入 0.2s → 停留（由调用方计时）→ 淡出 0.25s。
 */
@Composable
internal fun PlayerToast(
    text: String,
    modifier: Modifier = Modifier,
) {
    AnimatedVisibility(
        visible = text.isNotBlank(),
        enter = fadeIn(tween(200)),
        exit = fadeOut(tween(250)),
        modifier = modifier,
    ) {
        Box(
            modifier = Modifier
                .playerGlassSurface(elevation = 10.dp)
                .padding(horizontal = 16.dp, vertical = 10.dp),
            contentAlignment = Alignment.Center,
        ) {
            Text(
                text = text,
                color = PlayerTokens.textPrimary,
                fontSize = 14.sp,
                maxLines = 2,
            )
        }
    }
}
