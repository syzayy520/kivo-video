package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.EnterTransition
import androidx.compose.animation.ExitTransition
import androidx.compose.animation.core.tween
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.layout.Box
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import kotlinx.coroutines.delay
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.slideInVertically
import androidx.compose.animation.slideOutVertically
import androidx.compose.foundation.layout.BoxScope
import androidx.compose.runtime.Composable
import androidx.compose.runtime.State
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Rect
import com.kivo.cinema.ui.motion.MotionGate

/**
 * 顶栏 + 底栏的**显隐过渡宿主**（v3 §三）。
 *
 * 从 [PlayerOverlay] 分出来：Overlay 只管层序与挂载点，
 * "两条栏共用一个显隐信号、位移方向相反、G1 降级只留 alpha" 是独立的一件事。
 */
@Composable
internal fun BoxScope.PlayerBarsSlot(
    visible: Boolean,
    reduceMotion: Boolean,
    /** 面板打开中：两条栏**不隐藏**（自动隐藏已在 SessionBridge 暂停），只压到 55%（§13）。 */
    panelOpen: Boolean,
    title: String,
    speedText: String,
    activeAction: String,
    danmakuOn: Boolean,
    audioOnly: Boolean,
    isPlaying: Boolean,
    positionMs: Long,
    durationMs: Long,
    showRemaining: Boolean,
    showNextButton: Boolean,
    bufferedFraction: Float,
    scrub: com.kivo.cinema.ui.compose.player.seek.PlayerScrubState,
    previewAnchor: com.kivo.cinema.ui.compose.player.seek.PlayerSeekPreviewAnchor,
    resolutionLabel: String,
    expandedPanel: String,
    panelItems: List<PlayerOptionItem>,
    panelRect: State<Rect?>,
    onClose: () -> Unit,
    onHome: () -> Unit,
    onAction: (String) -> Unit,
    onPlayPause: () -> Unit,
    onLongPressPlay: () -> Unit,
    onNext: () -> Unit,
    onScrub: (Long) -> Unit,
    onSeek: (Long) -> Unit,
    onScrubEnd: () -> Unit,
    onTextAction: (String) -> Unit,
    onPanelSelect: (PlayerOptionItem) -> Unit,
    onEpisodeOrder: () -> Unit,
) {
    // G1 门有两个来源：面板宿主算出的系统值，以及 App 内「减弱动画效果」开关。
    val gateReduce = MotionGate.rememberAppReduceMotion()
    val reduced = reduceMotion || gateReduce
    // ★317：改成"常驻 + 动画"，不再用 AnimatedVisibility。
    //
    // 【为什么必须改】AnimatedVisibility 在 visible 快速切换时会**同时保留新旧两棵子树**
    // （旧的在播 exit、新的已在 enter）。而 PlayerBottomBar 内部的 `thickness`
    // 是 per-instance 的 `animateFloatAsState` ⇒ 旧树停在拖动态（10dp=27.5px）、
    // 新树是常态（6dp=16.5px） ⇒ 真机上表现为**两条进度条，上面那条更粗**。
    // 进度条是常驻元素，本就不该反复进出组合 ⇒ 用常驻 + alpha/位移复刻同一套动效。
    BarSlot(
        visible = visible,
        reduceMotion = reduced,
        fromTop = true,
        dimmed = panelOpen,
        modifier = Modifier.align(Alignment.TopCenter),
    ) {
        PlayerTopBar(
            title = title,
            speedText = speedText,
            activeAction = activeAction,
            danmakuOn = danmakuOn,
            audioOnly = audioOnly,
            onClose = onClose,
            onHome = onHome,
            onAction = onAction,
        )
    }
    BarSlot(
        visible = visible,
        reduceMotion = reduced,
        fromTop = false,
        dimmed = panelOpen,
        modifier = Modifier.align(Alignment.BottomCenter),
    ) {
        PlayerBottomBar(
            isPlaying = isPlaying,
            positionMs = positionMs,
            durationMs = durationMs,
            showRemaining = showRemaining,
            showNextButton = showNextButton,
            bufferedFraction = bufferedFraction,
            scrub = scrub,
            previewAnchor = previewAnchor,
            resolutionLabel = resolutionLabel,
            activeAction = activeAction,
            expandedPanel = expandedPanel,
            panelItems = panelItems,
            panelRect = panelRect,
            title = title,
            onPlayPause = onPlayPause,
            onLongPressPlay = onLongPressPlay,
            onNext = onNext,
            onScrub = onScrub,
            onSeek = onSeek,
            onScrubEnd = onScrubEnd,
            onTextAction = onTextAction,
            onPanelSelect = onPanelSelect,
            onEpisodeOrder = onEpisodeOrder,
        )
    }
}

private fun barEnter(reduceMotion: Boolean, fromTop: Boolean): EnterTransition =
    if (reduceMotion) fadeIn(tween(PlayerMotion.reduceMotionMs))
    else fadeIn(tween(PlayerMotion.barEnterMs, easing = PlayerMotion.easing)) +
        slideInVertically(tween(PlayerMotion.barEnterMs, easing = PlayerMotion.easing)) { if (fromTop) -it / 6 else it / 6 }

private fun barExit(reduceMotion: Boolean, fromTop: Boolean): ExitTransition =
    if (reduceMotion) fadeOut(tween(PlayerMotion.reduceMotionMs))
    else fadeOut(tween(PlayerMotion.barExitMs, easing = PlayerMotion.easing)) +
        slideOutVertically(tween(PlayerMotion.barExitMs, easing = PlayerMotion.easing)) { if (fromTop) -it / 6 else it / 6 }

/**
 * 317 · 常驻的控制栏槽位（替代 `AnimatedVisibility`）。
 *
 * 【为什么自己写】见调用处注释：`AnimatedVisibility` 会同时保留新旧子树，
 * 导致控制栏内部 per-instance 的动画状态（进度条粗细）出现两份。
 * 这里改成**单个实例常驻**，用 alpha + translationY 复刻原动效：
 * · 淡入淡出时长取 [PlayerMotion.barEnterMs] / [PlayerMotion.barExitMs]（与原实现一致）；
 * · 位移量取自身高度的 1/6（与原 `slideInVertically { it / 6 }` 一致）；
 * · `reduceMotion` 时只做淡入淡出（与原实现一致）。
 */
@Composable
private fun BarSlot(
    visible: Boolean,
    reduceMotion: Boolean,
    fromTop: Boolean,
    dimmed: Boolean = false,
    modifier: Modifier = Modifier,
    content: @Composable () -> Unit,
) {
    var heightPx by remember { mutableFloatStateOf(0f) }
    val durMs = if (visible) PlayerMotion.barEnterMs else PlayerMotion.barExitMs
    val animMs = if (reduceMotion) PlayerMotion.reduceMotionMs else durMs
    val alpha by animateFloatAsState(
        targetValue = if (visible) 1f else 0f,
        animationSpec = tween(animMs, easing = PlayerMotion.easing),
        label = "barAlpha",
    )
    val t by animateFloatAsState(
        targetValue = if (visible) 0f else 1f,
        animationSpec = tween(animMs, easing = PlayerMotion.easing),
        label = "barShift",
    )

    // 审查 P1-2：alpha=0 的常驻栏仍参与命中，透明进度条吞掉全屏横滑。
    // 正解：退场动画结束后卸载内容（不再命中），入场时立即挂回。
    // 不用 AnimatedVisibility —— 那会在切换瞬间同时保留两棵树，正是"双进度条"的根因；
    // 这里在动画**结束后**才卸载，任何时刻都只有一棵内容树。
    var mounted by remember { mutableStateOf(visible) }
    LaunchedEffect(visible) {
        if (visible) {
            mounted = true
        } else {
            delay(animMs.toLong())
            mounted = false
        }
    }

    // 位移方向：顶部栏向上退、底部栏向下退，各 1/6 自身高度。
    val sign = if (fromTop) -1f else 1f
    Box(
        modifier = modifier
            .onSizeChanged { heightPx = it.height.toFloat() }
            .graphicsLayer {
                // 面板打开时压暗（§10/§13）：×0.55 只改透明度，动画/装卸机制不变。
                this.alpha = alpha * if (dimmed) PlayerPanelStyle.barDimmedAlpha else 1f
                // G1：减弱动画时位移归零，只留 α —— 否则 100ms 内仍有 height/6 的滑移。
                translationY = if (reduceMotion) 0f else sign * (heightPx / 6f) * t
            },
    ) {
        if (mounted) content()
    }
}
