package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.AnimatedContent
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.slideInVertically
import androidx.compose.animation.slideOutVertically
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.tween
import androidx.compose.animation.togetherWith
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxScope
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.offset
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
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.layout.boundsInRoot
import androidx.compose.ui.layout.onGloballyPositioned
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp

/**
 * 浮层面板（倍速 / 音轨 / 字幕 / 画质…）。
 *
 * 【为什么从 PlayerOverlay 拆出来】覆盖层编排管的是**叠放顺序**，
 * 面板管的是**开合过渡**；两者混在一个文件里，改过渡动画要在一堆层序代码里翻。
 * 拆开后这里只剩"面板怎么出现/消失"，kind → 壳与宽度仍问 [PlayerPanelHost]。
 *
 * 【位置（B4-T1）】旧版是 `align(TopEnd) + 固定 topPad` —— 所有面板一律右上角。
 * 现在按 [anchor] 贴到**触发控件**旁，坐标系仍以整屏 Box 为准：
 * ```
 * TopIcon(i)   → TopStart + offset(x = 图标中心 − 面板宽/2, y = 顶栏高 + 6)
 * BottomAction → BottomEnd + padding(end = 12, bottom = 按钮行高 + 33 + 6)
 * Center       → 直接居中
 * ```
 * 屏幕宽度由 [BoxWithConstraints] 取（父 Box 是 `fillMaxSize` 的整屏），
 * 不用 `LocalConfiguration`：后者在分屏/画中画下给的是**整个窗口**宽度而不是本层宽度。
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
    onLongSelect: (PlayerOptionItem) -> Unit = {},
    backdrop: ImageBitmap? = null,
) {
    // 关闭即清空：底栏被盖的文字随之淡回（面板淡出时不再误隐）。
    LaunchedEffect(open) { if (!open) panelRect.value = null }
    // 记住最后打开的形态：淡出的 150ms 里 kind 已变空，
    // 不保留的话面板会瞬间变成空壳再淡出（肉眼可见的"闪没"）。
    var lastKind by remember { mutableStateOf("") }
    // 【关闭闪一下（2026-11-21 真机复现）】关闭瞬间 activePanel 变为空：
    // · anchorFor("") 落 else → Center ⇒ 淡出的面板会**瞬移到屏幕正中**（录屏 60fps 抓到 1 帧：
    //   屏幕中间亮度 129.5→166.9）；
    // · items 随即清空 ⇒ 即使位置对了，淡出的也是一块空玻璃。
    // 所以 kind / title / items 三个都要留最后一次快照，且 anchor 必须由 shownKind 派生。
    var lastTitle by remember { mutableStateOf("") }
    var lastItems by remember { mutableStateOf(emptyList<PlayerOptionItem>()) }
    LaunchedEffect(kind) { if (kind.isNotBlank()) lastKind = kind }
    LaunchedEffect(title, items) {
        if (title.isNotBlank() || items.isNotEmpty()) {
            lastTitle = title
            lastItems = items
        }
    }
    // 判据用 open 而不是 kind：退场时 kind 由 panelKindFallback 保住（非空），但 title/items
    // 已经清空 —— 若按 kind 判，面板会拿空标题/空行去画，高度塌成 0（实测退场仍 1 帧消失）。
    val shownKind = if (kind.isNotBlank()) kind else lastKind
    val shownTitle = if (open) title else lastTitle
    val shownItems = if (open) items else lastItems
    // 锚点只跟 shownKind 走：关闭的那 160ms 里位置不能跳。
    val effectiveAnchor = PlayerPanelHost.anchorFor(shownKind)

    BoxWithConstraints(modifier = Modifier.matchParentSize()) {
        val panelWidth = PlayerPanelHost.panelWidth(shownKind).value
        // 淡出期间 shownKind 不变 ⇒ 位置也不跳。
        val placement = panelPlacement(
            anchor = effectiveAnchor,
            screenWidth = maxWidth.value,
            panelWidth = panelWidth,
            audioOnly = audioOnly,
        )
        // 锚点以下的可用高度：壳自身的 maxHeight（330/358dp）按屏幕裁剪。
        // 不裁的话卡底会伸出屏幕（K40 上 358dp 卡从 top 94dp 起只有 283dp 可用），
        // 卡内"滚动到底"时最后几行仍在屏外 —— 重置行因此永远点不到（FAIL-2）。
        val availableHeight = when (effectiveAnchor) {
            is PlayerPanelAnchor.TopIcon ->
                maxHeight.value - PlayerPanelGeometry.topIconTop() - PlayerPanelGeometry.edgeGap
            is PlayerPanelAnchor.BottomAction ->
                maxHeight.value - PlayerPanelGeometry.bottomActionBottom() - PlayerPanelGeometry.edgeGap
            PlayerPanelAnchor.Center -> maxHeight.value - PlayerPanelGeometry.edgeGap * 2
        }.coerceAtLeast(120f)
        // P10 §8：入场 240ms alpha + 10dp 升起（顶栏锚点 −8→0），退场 160ms 向锚点回落 6dp，
        // **不缩放**（旧的 0.94 scale 就是"贴纸感"的一部分）。减弱动效只留 alpha。
        // 顶栏锚点的面板吊在图标下方，"朝锚点"是向上 ⇒ 位移取负。
        val topAnchored = effectiveAnchor is PlayerPanelAnchor.TopIcon
        val density = LocalDensity.current
        val enterRisePx = with(density) {
            (if (topAnchored) PlayerPanelStyle.motion.enterRiseTopIconDp
            else PlayerPanelStyle.motion.enterRiseDp).dp.roundToPx()
        }
        val exitDropPx = with(density) {
            PlayerPanelStyle.motion.exitDropDp.dp.roundToPx() * (if (topAnchored) -1 else 1)
        }
        val swapShiftPx = with(density) { PlayerPanelStyle.motion.swapShiftDp.dp.roundToPx() }
        // 【为什么不用 AnimatedVisibility】实测：把 exitMs 临时改到 800ms，关闭仍是 1 帧硬切
        // （60fps 录屏逐帧，见 docs/evidence/p9-subtitle-reaccept/BUG-DEVICE.md §5）—— 它的 exit
        // 在这条链路上根本没执行。改成 alpha + 位移两条手动动画，退场必然可见、且时长可验证。
        val panelAlpha by animateFloatAsState(
            targetValue = if (open) 1f else 0f,
            animationSpec = tween(
                if (reduceMotion) PlayerPanelStyle.motion.reduceMs
                else if (open) PlayerPanelStyle.motion.enterMs else PlayerPanelStyle.motion.exitMs,
                easing = if (open) PlayerPanelStyle.motion.enterEase else PlayerPanelStyle.motion.exitEase,
            ),
            label = "panelAlpha",
        )
        val panelShift = remember { androidx.compose.animation.core.Animatable(0f) }
        LaunchedEffect(open, reduceMotion) {
            if (open) {
                // 入场：从"朝锚点方向偏移"的初值升到 0（显式初值，这是 AnimatedVisibility 的 slide 语义）。
                panelShift.snapTo(if (reduceMotion) 0f else enterRisePx.toFloat())
                panelShift.animateTo(
                    0f,
                    tween(
                        if (reduceMotion) PlayerPanelStyle.motion.reduceMs
                        else PlayerPanelStyle.motion.enterMs,
                        easing = PlayerPanelStyle.motion.enterEase,
                    ),
                )
            } else if (!reduceMotion) {
                panelShift.animateTo(
                    exitDropPx.toFloat(),
                    tween(PlayerPanelStyle.motion.exitMs, easing = PlayerPanelStyle.motion.exitEase),
                )
            }
        }
        // 退出期间保留一帧都不能少：alpha 归零前继续画，归零后才真正移除。
        if (open || panelAlpha > 0.001f) {
            Box(
                modifier = placement
                    .graphicsLayer {
                    alpha = panelAlpha
                    translationY = panelShift.value
                },
            ) {
            // 钻入换页（倍速→微调卡等）：140/120ms 交叉淡化 + 4dp 纵向位移，锚点不动。
            // 滚动位置随旧内容一起销毁 ⇒ 返回时归零（§9），无需额外复位。
            AnimatedContent(
                targetState = shownKind,
                transitionSpec = {
                    fadeIn(
                        tween(PlayerPanelStyle.motion.swapEnterMs, easing = PlayerPanelStyle.motion.rowEase),
                    ) + slideInVertically(
                        tween(PlayerPanelStyle.motion.swapEnterMs, easing = PlayerPanelStyle.motion.rowEase),
                    ) { swapShiftPx } togetherWith
                        fadeOut(
                            tween(PlayerPanelStyle.motion.swapExitMs, easing = PlayerPanelStyle.motion.rowEase),
                        ) + slideOutVertically(
                            tween(PlayerPanelStyle.motion.swapExitMs, easing = PlayerPanelStyle.motion.rowEase),
                        ) { -swapShiftPx }
                },
                label = "panelSwap",
            ) { targetKind ->
                Box(modifier = Modifier.heightIn(max = availableHeight.dp)) {
                    PlayerPanelShell(
                        kind = targetKind,
                        title = shownTitle,
                        items = shownItems,
                        onSelect = onSelect,
                        onStep = onStep,
                        onDismiss = onDismiss,
                        onLongSelect = onLongSelect,
                        backdrop = backdrop,
                        // 在壳上量真实矩形（含壳自身宽高），只在打开时发布。
                        modifier = Modifier.onGloballyPositioned {
                            if (open) panelRect.value = it.boundsInRoot()
                        },
                    )
                }
            }
            }
        }
    }
}

/**
 * 锚点 → 摆放 Modifier。
 *
 * 三种锚点用的**不是同一个 align**，这正是把语义留在类型里的原因：
 * 顶栏要 TopStart + 精确 offset，底栏只需要贴住右下两条边（宽度自适应、高度向下长）。
 * `playerSafeArea` 不再挂在这里 —— 它只加 start/top 而锚点要的是两侧对称的
 * [PlayerPanelGeometry.edgeGap]，两者叠在一起会把顶栏面板推偏 47pt（横屏挖孔）。
 */
private fun BoxScope.panelPlacement(
    anchor: PlayerPanelAnchor,
    screenWidth: Float,
    panelWidth: Float,
    audioOnly: Boolean,
): Modifier = when (anchor) {
    is PlayerPanelAnchor.TopIcon -> Modifier
        .align(Alignment.TopStart)
        .offset(
            x = PlayerPanelGeometry.topIconLeft(screenWidth, panelWidth, anchor.index, audioOnly).dp,
            y = PlayerPanelGeometry.topIconTop().dp,
        )
    is PlayerPanelAnchor.BottomAction -> Modifier
        .align(Alignment.BottomEnd)
        .padding(
            end = PlayerPanelGeometry.edgeGap.dp,
            bottom = PlayerPanelGeometry.bottomActionBottom().dp,
        )
    PlayerPanelAnchor.Center -> Modifier.align(Alignment.Center)
}