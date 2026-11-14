package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.AnimatedContent
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.scaleIn
import androidx.compose.animation.scaleOut
import androidx.compose.animation.togetherWith
import androidx.compose.foundation.layout.BoxScope
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.padding
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.MutableState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.layout.boundsInRoot
import androidx.compose.ui.layout.onGloballyPositioned
import androidx.compose.ui.unit.dp

/**
 * 浮层面板（倍速 / 音轨 / 字幕 / 画质…）。
 *
 * 【为什么从 PlayerOverlay 拆出来】覆盖层编排管的是**叠放顺序**，
 * 面板管的是**开合过渡**；两者混在一个文件里，改过渡动画要在一堆层序代码里翻。
 * 拆开后这里只剩"面板怎么出现/消失"，锚点与壳仍问 [PlayerPanelHost]。
 */
@Composable
internal fun BoxScope.FloatingPanel(
    kind: String,
    open: Boolean,
    reduceMotion: Boolean,
    audioOnly: Boolean,
    title: String,
    items: List<PlayerOptionItem>,
    onSelect: (PlayerOptionItem) -> Unit,
    onStep: (PlayerOptionItem, Int) -> Unit,
    onDismiss: () -> Unit,
    panelRect: MutableState<Rect?>,
) {
    // 关闭即清空：底栏被盖的文字随之淡回（面板淡出时不再误隐）。
    LaunchedEffect(open) { if (!open) panelRect.value = null }
    // 记住最后打开的形态：淡出的 150ms 里 kind 已变空，
    // 不保留的话面板会瞬间变成空壳再淡出（肉眼可见的"闪没"）。
    var lastKind by remember { mutableStateOf("") }
    LaunchedEffect(kind) { if (kind.isNotBlank()) lastKind = kind }
    val shownKind = if (kind.isNotBlank()) kind else lastKind
    AnimatedVisibility(
        visible = open,
        enter = if (reduceMotion) fadeIn(tween(PlayerMotion.reduceMotionMs))
        else fadeIn(PlayerMotion.panelSpring) +
            scaleIn(PlayerMotion.panelSpring, initialScale = 0.94f),
        exit = if (reduceMotion) fadeOut(tween(PlayerMotion.reduceMotionMs))
        else fadeOut(tween(PlayerMotion.panelExitMs, easing = PlayerMotion.panelExitEasing)) +
            scaleOut(tween(PlayerMotion.panelExitMs, easing = PlayerMotion.panelExitEasing), targetScale = 0.94f),
        modifier = Modifier
            .align(Alignment.TopEnd)
            .playerSafeArea(WindowInsetsSides.Horizontal)
            .padding(
                top = PlayerPanelHost.topPad(shownKind).dp,
                end = PlayerPanelHost.trailingPad(shownKind, audioOnly).dp,
            ),
    ) {
        // 钻入换页（倍速→微调卡等）：交叉淡化而不是整壳重建，锚点不动。
        AnimatedContent(
            targetState = shownKind,
            transitionSpec = {
                fadeIn(tween(PlayerMotion.panelSwapEnterMs, easing = PlayerMotion.easing)) togetherWith
                    fadeOut(tween(PlayerMotion.panelSwapExitMs, easing = PlayerMotion.easing))
            },
            label = "panelSwap",
        ) { targetKind ->
            PlayerPanelShell(
                kind = targetKind,
                title = title,
                items = items,
                onSelect = onSelect,
                onStep = onStep,
                onDismiss = onDismiss,
                // 在壳上量真实矩形（含壳自身宽高），只在打开时发布。
                modifier = Modifier.onGloballyPositioned {
                    if (open) panelRect.value = it.boundsInRoot()
                },
            )
        }
    }
}
