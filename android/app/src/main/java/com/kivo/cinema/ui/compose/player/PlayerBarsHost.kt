package com.kivo.cinema.ui.compose.player

import android.graphics.Bitmap
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.EnterTransition
import androidx.compose.animation.ExitTransition
import androidx.compose.animation.core.tween
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
    title: String,
    speedText: String,
    activeAction: String,
    danmakuOn: Boolean,
    audioOnly: Boolean,
    isPlaying: Boolean,
    positionMs: Long,
    durationMs: Long,
    showRemaining: Boolean,
    scrubPreview: Boolean,
    bufferedFraction: Float,
    thumbnailBitmap: Bitmap?,
    thumbUnavailable: String,
    previewMs: Long,
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
    onTextAction: (String) -> Unit,
    onPanelSelect: (PlayerOptionItem) -> Unit,
    onCapture: () -> Unit,
    onEpisodeOrder: () -> Unit,
) {
    AnimatedVisibility(
        visible = visible,
        enter = barEnter(reduceMotion, fromTop = true),
        exit = barExit(reduceMotion, fromTop = true),
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
    AnimatedVisibility(
        visible = visible,
        enter = barEnter(reduceMotion, fromTop = false),
        exit = barExit(reduceMotion, fromTop = false),
        modifier = Modifier.align(Alignment.BottomCenter),
    ) {
        PlayerBottomBar(
            isPlaying = isPlaying,
            positionMs = positionMs,
            durationMs = durationMs,
            showRemaining = showRemaining,
            scrubPreview = scrubPreview,
            bufferedFraction = bufferedFraction,
            thumbnailBitmap = thumbnailBitmap,
            thumbUnavailable = thumbUnavailable,
            previewMs = previewMs,
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
            onTextAction = onTextAction,
            onPanelSelect = onPanelSelect,
            onCapture = onCapture,
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