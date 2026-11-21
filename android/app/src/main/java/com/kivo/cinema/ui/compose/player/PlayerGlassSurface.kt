package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Shape
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp

/**
 * **旧玻璃材质**（v3：黑 75% + 白 8% 描边 + 上半个面板高光）—— 已被 [playerPanelSurface] 取代。
 *
 * 【为什么保留而不删】P10 §11 要求"迁移完面板/Toast 后先 grep 确认零调用；
 * 有未知引用则保留并 @Deprecated 委托，不删"。迁移后本模块已无调用点，
 * 但 [PlayerPanelOcclusion] 的 KDoc 仍链接本符号，且工作区还有别的助手未提交的改动
 * ⇒ 保守保留。**委托而非复制**：材质只有 [playerPanelSurface] 一处实现，
 * 这条旧路径不会再长出第二套样式。
 */
@Deprecated(
    message = "P10：面板材质统一到 playerPanelSurface()（去描边、去上半个面板高光）",
    replaceWith = ReplaceWith("playerPanelSurface(radius = PlayerTokens.panelRadius.dp, elevation = elevation, shape = shape)"),
)
internal fun Modifier.playerGlassSurface(
    shape: Shape = RoundedCornerShape(PlayerTokens.panelRadius.dp),
    elevation: Dp = 18.dp,
): Modifier = playerPanelSurface(
    radius = PlayerTokens.panelRadius.dp,
    elevation = elevation,
    shape = shape,
)