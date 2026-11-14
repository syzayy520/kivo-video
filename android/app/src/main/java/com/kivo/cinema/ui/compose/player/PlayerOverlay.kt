package com.kivo.cinema.ui.compose.player

import android.graphics.Bitmap
import androidx.compose.animation.AnimatedContent
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.scaleIn
import androidx.compose.animation.scaleOut
import androidx.compose.animation.togetherWith
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxScope
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.fillMaxSize
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
 * 播放页覆盖层编排（v3 §一/§二/§三）。
 *
 * ## 叠放顺序（从下到上）—— 功能正确性，不只是视觉
 * ```
 *   状态层（加载/失败）   ← 最上，压住一切
 *   手势闪示 / 轻提示 / HUD / 长按标签
 *   浮层面板（玻璃列表/微调卡）
 *   暂停态浮相机钮 / 顶栏 / 底栏（含内联资源槽与选集带）
 *   锁定态挂锁 / 听视频居中块
 *   手势层               ← 最下面（兜底点画面）
 * ```
 * 手势层必须在最下面：它是全屏 pointerInput，放上面会把所有按钮吃掉
 * （v1 实机：按钮全部点不动）。
 *
 * ## 整屏态
 * 听视频（形态13）**不替换控制条**：只把视频区换成黑底居中块，
 * 顶栏隐 PiP、耳机白圆亮。锁定态只画挂锁，但手势层的双击/长按通道保留
 * （锁定不是全禁）。字幕由 libmpv/libass 合成进视频帧（§1.6），
 * Compose 不再自绘字幕层。
 *
 * ## PiP 态（v3.1）
 * [pipActive] 时**整层不组合**：小窗里只有视频帧，任何 chrome/手势层
 * 都会在小窗上变成误触区。退出 PiP 后由 onPipModeChanged 恢复控制条。
 *
 * ## 过渡（§三，全部带 G1 降级）
 * 控制条 fade+位移 180/140ms；面板 fade+0.94 缩放 200/150ms；
 * Reduce Motion ⇒ 位移/缩放归零，只留 alpha。
 */
@Composable
internal fun PlayerOverlay(
    visible: Boolean,
    title: String,
    speedText: String,
    isPlaying: Boolean,
    positionMs: Long,
    durationMs: Long,
    showRemaining: Boolean,
    scrubPreview: Boolean,
    showNextButton: Boolean,
    showCaptureButton: Boolean,
    bufferedFraction: Float,
    thumbnailBitmap: Bitmap?,
    thumbUnavailable: String,
    previewMs: Long,
    resolutionLabel: String,
    phase: PlayerPhase,
    activeAction: String,
    activePanel: String,
    panelTitle: String,
    panelItems: List<PlayerOptionItem>,
    flashZone: DoubleTapZone?,
    toastText: String,
    hudKind: PlayerHudKind?,
    hudFraction: Float,
    longPressSpeedLabel: String,
    longPressLocked: Boolean,
    screenLocked: Boolean,
    lockHintVisible: Boolean,
    audioOnly: Boolean,
    danmakuOn: Boolean,
    pipActive: Boolean,
    onClose: () -> Unit,
    onHome: () -> Unit,
    onDismissPanel: () -> Unit,
    onAction: (String) -> Unit,
    onPlayPause: () -> Unit,
    onLongPressPlay: () -> Unit,
    onNext: () -> Unit,
    onScrub: (Long) -> Unit,
    onSeek: (Long) -> Unit,
    onScrubEnd: () -> Unit,
    onTextAction: (String) -> Unit,
    onPanelSelect: (PlayerOptionItem) -> Unit,
    onPanelStep: (PlayerOptionItem, Int) -> Unit,
    onCapture: () -> Unit,
    onEpisodeOrder: () -> Unit,
    onTap: () -> Unit,
    onLockHint: () -> Unit,
    onUnlock: () -> Unit,
    onDoubleTap: (DoubleTapZone) -> Unit,
    onVerticalDragStart: (Float) -> Unit,
    onVerticalDrag: (Float) -> Unit,
    onVerticalDragEnd: () -> Unit,
    onLongPressStart: (Float, Float) -> Unit,
    onLongPressDrag: (Float, Float) -> Unit,
    onLongPressEnd: () -> Unit,
    onRetry: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val reduceMotion = rememberReduceMotion()
    val inline = PlayerPanelHost.isInline(activePanel)
    val panelOpen = activePanel.isNotBlank() && !inline
    // 内联形态（资源/选集）的内容住在底栏里 ⇒ 底栏必须跟着显形。
    // 听视频态控制条保留 ⇒ 这里不再排除 audioOnly。
    val barsVisible = (visible || inline) && !screenLocked
    // 面板真实矩形（根坐标系）：FloatingPanel 实测发布，底栏据此隐去被盖住的文字。
    val panelRect = remember { mutableStateOf<Rect?>(null) }

    Box(modifier = modifier.fillMaxSize()) {
        // PiP 小窗：不画任何 chrome、不挂手势层（见文件头"PiP 态"）。
        if (pipActive) return@Box
        PlayerGestureLayer(
            onTap = {
                when {
                    panelOpen || inline -> onDismissPanel()
                    screenLocked -> onLockHint()
                    else -> onTap()
                }
            },
            onDoubleTap = onDoubleTap,
            locked = screenLocked,
            onVerticalDragStart = onVerticalDragStart,
            onVerticalDrag = onVerticalDrag,
            onVerticalDragEnd = onVerticalDragEnd,
            onLongPressStart = onLongPressStart,
            onLongPressDrag = onLongPressDrag,
            onLongPressEnd = onLongPressEnd,
        )

        if (audioOnly) {
            PlayerAudioOnly(modifier = Modifier.align(Alignment.Center))
        }
        PlayerBarsSlot(
            visible = barsVisible,
            reduceMotion = reduceMotion,
            title = title,
            speedText = speedText,
            activeAction = activeAction,
            danmakuOn = danmakuOn,
            audioOnly = audioOnly,
            isPlaying = isPlaying,
            positionMs = positionMs,
            durationMs = durationMs,
            showRemaining = showRemaining,
            scrubPreview = scrubPreview,
            showNextButton = showNextButton,
            bufferedFraction = bufferedFraction,
            thumbnailBitmap = thumbnailBitmap,
            thumbUnavailable = thumbUnavailable,
            previewMs = previewMs,
            resolutionLabel = resolutionLabel,
            expandedPanel = if (inline) activePanel else "",
            panelItems = panelItems,
            panelRect = panelRect,
            onClose = onClose,
            onHome = onHome,
            onAction = onAction,
            onPlayPause = onPlayPause,
            onLongPressPlay = onLongPressPlay,
            onNext = onNext,
            onScrub = onScrub,
            onSeek = onSeek,
            onScrubEnd = onScrubEnd,
            onTextAction = onTextAction,
            onPanelSelect = onPanelSelect,
            onCapture = onCapture,
            onEpisodeOrder = onEpisodeOrder,
        )
        if (barsVisible && !isPlaying && !audioOnly && showCaptureButton) {
            PlayerCaptureButton(
                onCapture = onCapture,
                modifier = Modifier.align(Alignment.CenterStart),
            )
        }
        FloatingPanel(
            kind = activePanel,
            open = panelOpen && !screenLocked,
            reduceMotion = reduceMotion,
            audioOnly = audioOnly,
            title = panelTitle,
            items = panelItems,
            onSelect = onPanelSelect,
            onStep = onPanelStep,
            onDismiss = onDismissPanel,
            panelRect = panelRect,
        )
        PlayerLockLayer(
            visible = screenLocked && lockHintVisible,
            onUnlock = onUnlock,
            modifier = Modifier.align(Alignment.BottomStart),
        )

        PlayerLongPressHud(
            speedLabel = longPressSpeedLabel,
            locked = longPressLocked,
            modifier = Modifier.align(Alignment.Center),
        )
        // 常挂 + visible 驱动：退出时整棵子树淡出 300ms，而不是被 if 直接摘掉。
        PlayerGestureHud(
            visible = hudKind != null && !screenLocked,
            kind = hudKind,
            fraction = hudFraction,
            modifier = Modifier
                .align(Alignment.TopCenter)
                .padding(top = PlayerTokens.topBarHeight.dp + 14.dp),
        )
        PlayerToast(
            text = toastText,
            modifier = Modifier
                .align(Alignment.BottomCenter)
                .padding(bottom = PlayerTokens.bottomScrimHeight.dp + 12.dp),
        )
        if (flashZone != null) {
            PlayerGestureFlash(zone = flashZone, modifier = Modifier.align(Alignment.Center))
        }
        PlayerStatusLayer(phase = phase, onRetry = onRetry)
    }
}
