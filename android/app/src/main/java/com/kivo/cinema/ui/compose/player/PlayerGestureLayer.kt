package com.kivo.cinema.ui.compose.player

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput

/**
 * 播放页的**手势层**（player-ui-v2.md 边界规则 P1：只负责展示与手势）。
 *
 * ## 为什么手势层在这里，而不是在 PlayerScreen 里
 * 第一版把 `detectTapGestures` 放在 [com.kivo.cinema.ui.compose.PlayerScreen] 内部、
 * 用一个 `fillMaxSize()` 的 Box 铺满画面。后果是**所有点击都被它吃掉**：
 * 顶栏图标、底栏按钮全部点不动 —— 事件穿过覆盖层落到它手上，
 * 变成了"切换控制条显隐"。
 *
 * 【正确的分层】手势层必须是**最上层**、且**只在没有可交互控件时**存在。
 * 现在它挂在 [PlayerOverlay] 内部：
 * · 控制条可见 ⇒ 手势层**不启用**（pointerInput 不注册）⇒ 点按钮就是点按钮；
 * · 控制条隐藏 ⇒ 手势层启用 ⇒ 点画面唤出控制条。
 *
 * 这与参考一致：REX 在有面板/控制条时，点控制条不会误触发"收起"。
 */
@Composable
internal fun PlayerGestureLayer(
    enabled: Boolean,
    onTap: () -> Unit,
    onDoubleTap: () -> Unit,
    modifier: Modifier = Modifier,
) {
    if (!enabled) return
    Box(
        modifier = modifier
            .fillMaxSize()
            .pointerInput(Unit) {
                detectTapGestures(
                    onTap = { onTap() },
                    onDoubleTap = { onDoubleTap() },
                )
            },
    )
}
