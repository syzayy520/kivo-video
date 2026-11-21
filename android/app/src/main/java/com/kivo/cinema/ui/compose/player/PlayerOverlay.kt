package com.kivo.cinema.ui.compose.player

import android.graphics.Bitmap
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.runtime.Composable
import androidx.compose.runtime.SideEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.key
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp
import com.kivo.cinema.playback.MpvPlaybackSession
import com.kivo.cinema.ui.compose.player.seek.PlayerScrubState
import com.kivo.cinema.ui.compose.player.seek.PlayerSeekPreviewAnchor
import com.kivo.cinema.ui.compose.player.seek.seekPreviewLeftTop

@Composable
internal fun PlayerOverlay(
    visible: Boolean,
    mpvSession: MpvPlaybackSession? = null,
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
    scrub: PlayerScrubState,
    resolutionLabel: String,
    phase: PlayerPhase,
    activeAction: String,
    activePanel: String,
    panelTitle: String,
    panelItems: List<PlayerOptionItem>,
    flashZone: DoubleTapZone?,
    flashEventId: Long,
    toastText: String,
    hudKind: PlayerHudKind?,
    hudFraction: Float,
    hudOriginFraction: Float,
    hudDragging: Boolean,
    longPressSpeedLabel: String,
    longPressLocked: Boolean,
    longPressActive: Boolean,
    screenLocked: Boolean,
    gestureEpoch: Long,
    pinch: PinchScaleState,
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
    onPanelLongPress: (PlayerOptionItem) -> Unit,
    onCapture: () -> Unit,
    onEpisodeOrder: () -> Unit,
    onTap: () -> Unit,
    onLockHint: () -> Unit,
    onUnlock: () -> Unit,
    onDoubleTap: (DoubleTapZone) -> Unit,
    onHorizontalDragStart: () -> Unit,
    onHorizontalDrag: (Float) -> Unit,
    onHorizontalDragEnd: (Boolean) -> Unit,
    onVerticalDragStart: (Float) -> Unit,
    onVerticalDrag: (Float) -> Unit,
    onVerticalDragEnd: () -> Unit,
    onLongPressStart: (Float, Float) -> Unit,
    onLongPressDrag: (Float, Float) -> Unit,
    onLongPressEnd: () -> Unit,
    onLongPressCancel: () -> Unit,
    onRetry: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val reduceMotion = com.kivo.cinema.ui.motion.MotionGate.rememberAppReduceMotion()
    val inline = PlayerPanelHost.isInline(activePanel)
    val panelOpen = activePanel.isNotBlank() && !inline
    // 面板背后那层"当前帧模糊版"：只在面板真正打开（且未锁屏）时抓一次，
    // 关闭立即回到 null ⇒ 面板退场动画期间仍用最后一张（§2.2 C-BACKDROP@1）。
    val backdrop = rememberPanelBackdrop(mpvSession, open = panelOpen && !screenLocked)
    val barsVisible = (visible || inline) && !screenLocked
    val panelRect = remember { mutableStateOf<Rect?>(null) }
    val previewAnchor = remember { PlayerSeekPreviewAnchor() }
    var lastHudKind by remember { mutableStateOf(PlayerHudKind.VOLUME) }
    SideEffect { if (hudKind != null) lastHudKind = hudKind }
    val levelSide = if ((hudKind ?: lastHudKind) == PlayerHudKind.BRIGHTNESS) Alignment.CenterStart else Alignment.CenterEnd

    Box(modifier = modifier.fillMaxSize()) {
        if (pipActive) return@Box
        // 兜底手势层必须低于控件；失焦或控件接管时撤销尚未派发的点击。
        key(gestureEpoch) {
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
                pinch = pinch,
                suppressSwipe = longPressActive || longPressLocked,
                onHorizontalDragStart = onHorizontalDragStart,
                onHorizontalDrag = onHorizontalDrag,
                onHorizontalDragEnd = onHorizontalDragEnd,
                onVerticalDragStart = onVerticalDragStart,
                onVerticalDrag = onVerticalDrag,
                onVerticalDragEnd = onVerticalDragEnd,
                onLongPressStart = onLongPressStart,
                onLongPressDrag = onLongPressDrag,
                onLongPressEnd = onLongPressEnd,
                onLongPressCancel = onLongPressCancel,
            )
        }
        if (audioOnly) PlayerAudioOnly(modifier = Modifier.align(Alignment.Center))
        PlayerBarsSlot(
            visible = barsVisible,
            reduceMotion = reduceMotion,
            panelOpen = panelOpen,
            title = title,
            speedText = speedText,
            activeAction = activeAction,
            danmakuOn = danmakuOn,
            audioOnly = audioOnly,
            isPlaying = isPlaying,
            positionMs = positionMs,
            durationMs = durationMs,
            showRemaining = showRemaining,
            showNextButton = showNextButton,
            bufferedFraction = bufferedFraction,
            scrub = scrub,
            previewAnchor = previewAnchor,
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
            onEpisodeOrder = onEpisodeOrder,
        )
        // 预览卡挂根 Box：轨道盒内落位被祖先加偏（真机 25–32），根 Box align+padding 实测正确。
        val previewTrack = previewAnchor.trackBounds
        if (previewTrack != null) {
            val cardAlpha by animateFloatAsState(
                targetValue = if (scrub.visible) 1f else 0f,
                animationSpec = tween(
                    if (scrub.visible) (if (reduceMotion) 100 else 120)
                    else (if (reduceMotion) 100 else 180),
                ),
                label = "seekPreviewAlpha",
            )
            if (cardAlpha > 0f) {
                val (left, top) = seekPreviewLeftTop(
                    track = previewTrack,
                    durationMs = durationMs,
                    targetMs = scrub.targetMs,
                    showFrame = scrubPreview,
                    density = LocalDensity.current.density,
                )
                PlayerSeekPreview(
                    bitmap = thumbnailBitmap,
                    unavailableReason = thumbUnavailable,
                    targetMs = scrub.targetMs,
                    originMs = scrub.originMs,
                    durationMs = durationMs,
                    showFrame = scrubPreview,
                    containerWidth = (previewTrack.width / LocalDensity.current.density).dp,
                    onCapture = onCapture,
                    modifier = Modifier.align(Alignment.TopStart)
                        .padding(start = left, top = top)
                        .graphicsLayer { alpha = cardAlpha },
                )
            }
        }
        if (barsVisible && !isPlaying && !audioOnly && showCaptureButton) {
            PlayerCaptureButton(onCapture = onCapture, modifier = Modifier.align(Alignment.CenterStart))
        }
        // 【稳定组合身份】前面 `if (previewTrack != null)` 这类块会让后面兄弟的 slot 移位，
        // 面板的 remember（alpha/快照）会随之被重置 ⇒ 退场动画永远跑不出来。key() 钉住身份。
        key("floating-panel") {
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
            onLongSelect = onPanelLongPress,
            backdrop = backdrop,
        )
        }
        PlayerLockLayer(
            visible = screenLocked && lockHintVisible,
            onUnlock = onUnlock,
            modifier = Modifier.align(Alignment.BottomStart),
        )
        PlayerLongPressHud(
            speedLabel = longPressSpeedLabel,
            locked = longPressLocked,
            modifier = Modifier.align(Alignment.TopCenter)
                .playerSafeArea(WindowInsetsSides.Top).padding(top = 12.dp),
        )
        PlayerGestureHud(
            visible = hudKind != null && !screenLocked,
            kind = hudKind,
            fraction = hudFraction,
            originFraction = hudOriginFraction,
            dragging = hudDragging,
            modifier = Modifier.align(levelSide)
                .playerSafeArea(WindowInsetsSides.Horizontal).padding(horizontal = 30.dp),
        )
        PlayerToast(
            text = toastText,
            // P10 §11：Toast 距底 122 → 126（抬高 4dp，避开新面板的 28dp 阴影外溢边）。
            modifier = Modifier.align(Alignment.BottomCenter)
                .padding(bottom = PlayerTokens.bottomScrimHeight.dp + 16.dp),
        )
        PlayerGestureFlash(zone = flashZone, eventId = flashEventId, isPlaying = isPlaying)
        PlayerStatusLayer(phase = phase, onRetry = onRetry)
    }
}
