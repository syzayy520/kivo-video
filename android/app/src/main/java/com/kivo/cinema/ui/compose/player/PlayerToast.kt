package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.sp

/**
 * 播放页的**轻提示**。
 *
 * 【为什么要它】铁律：不许静默降级。
 * 任何"点了但做不了"的功能（下一集没接资源库、画质比例暂时不支持…）
 * 都必须给用户**看得见的回应**，而不是毫无反应。
 * 用浮层提示而不是 Toast 弹窗：Toast 会打断观感，且在横屏下位置别扭。
 *
 * 【P10 材质】与面板共用 [playerPanelSurface]（一次阴影 + 一次填充）：
 * `#E60E0E10`、圆角 14、阴影 8、内边距 14×9、13sp、**无高光**。
 * 错误态在文字前加 6dp 琥珀点 —— 只加标记，不改文字色（§3）。
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
        enter = fadeIn(tween(PlayerMotion.toastEnterMs, easing = PlayerMotion.easing)),
        exit = fadeOut(tween(PlayerMotion.toastExitMs, easing = PlayerMotion.easing)),
        modifier = modifier,
    ) {
        Row(
            modifier = Modifier
                .playerPanelSurface(
                    radius = PlayerPanelStyle.toastRadius,
                    elevation = PlayerPanelStyle.toastShadow,
                    top = PlayerPanelStyle.toastSurface,
                    bottom = PlayerPanelStyle.toastSurface,
                    sheen = Color.Transparent,
                    // Toast 不画任何边：它 8dp 阴影、面积小，一条亮边会像"贴了胶带"。
                    edge = Color.Transparent,
                    edgeTop = Color.Transparent,
                    edgeBottom = Color.Transparent,
                )
                .padding(
                    horizontal = PlayerPanelStyle.toastPadH,
                    vertical = PlayerPanelStyle.toastPadV,
                ),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            if (toastIsError(text)) {
                Box(
                    modifier = Modifier
                        .padding(end = PlayerPanelStyle.toastDotGap)
                        .size(PlayerPanelStyle.toastDotSize)
                        .background(PlayerPanelStyle.toastDot, CircleShape),
                )
            }
            Text(
                text = text,
                color = PlayerTokens.textPrimary,
                fontSize = PlayerPanelStyle.toastTextSize.sp,
                maxLines = 2,
            )
        }
    }
}

/**
 * 失败提示判据（**全工程唯一处**）：文案含这些词 ⇒ 错误态（琥珀点 + 2.6s 停留）。
 *
 * 【为什么按文案判定，而不是加一个 severity 字段】提示的严重度来自"这次操作有没有做成"，
 * 而这句话在写文案时就已确定；播放页状态里没有（也不该为 UI 新开）severity 位。
 * 判据与停留时长必须同源：否则会出现"带琥珀点但 1.8s 就消失"或反过来的错配。
 * 只在**动作失败/被拒**的措辞上命中；"正在…""已…"这类陈述态不进表。
 */
private val TOAST_ERROR_MARKERS = listOf(
    "失败", "无法", "不能", "无效", "尚未", "未找到", "不支持", "已失效", "拒绝",
)

internal fun toastIsError(text: String): Boolean = TOAST_ERROR_MARKERS.any { text.contains(it) }