package com.kivo.cinema.ui.compose.player

import android.graphics.Bitmap
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

/**
 * 播放页覆盖层编排（player-ui-v2.md §二 的四层信息架构）。
 *
 * **它只做编排，不含任何业务逻辑**（边界规则 P1）：
 * 画面层在外面（[com.kivo.cinema.ui.compose.PlayerScreen]），本组件负责
 * 顶栏 / 底栏 / 状态层三者的**显隐与叠放顺序**。
 *
 * 叠放顺序（从下到上）：
 * ```
 *   状态层（加载/失败）   ← 最上，压住一切
 *   底栏 + 字幕
 *   顶栏
 * ```
 * 字幕与底栏**同层但不相交** —— 字幕在底栏渐变区之上（距底 24pt），
 * 底栏的按钮行在其下。这是本项目踩过的坑（控制条遮住字幕）的结构性修复。
 */
@Composable
internal fun PlayerOverlay(
    visible: Boolean,
    title: String,
    speedText: String,
    isPlaying: Boolean,
    positionMs: Long,
    durationMs: Long,
    subtitleText: String,
    thumbnailBitmap: Bitmap?,
    previewMs: Long,
    resolutionLabel: String,
    buffering: Boolean,
    errorText: String,
    activeAction: String?,
    panelTitle: String,
    panelItems: List<PlayerOptionItem>,
    panelOpen: Boolean,
    onClose: () -> Unit,
    onDismissPanel: () -> Unit,
    onAction: (String) -> Unit,
    onPlayPause: () -> Unit,
    onNext: () -> Unit,
    onScrub: (Long) -> Unit,
    onSeek: (Long) -> Unit,
    onTextAction: (String) -> Unit,
    onPanelSelect: (PlayerOptionItem) -> Unit,
    onTap: () -> Unit,
    onDoubleTap: () -> Unit,
    onRetry: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Box(modifier = modifier.fillMaxSize()) {
        if (visible) {
            // ---- 字幕层（在底栏**之上**，两者不相交）----
            //
            // 【为什么每一层都要自己包一个 Box】
            // `Modifier.align()` **只在同一个 BoxScope 的直接子元素上有效**。
            // 把它作为参数传进子组件后，在子组件自己的 Box 里就**失效了**
            // ⇒ 底栏不会贴到屏幕底，而是按内容高度排在上面 ⇒ 按钮被裁 ⇒ 点不到。
            // 所以：每层的 align 都写在本层的 BoxScope 里（下面的 Box(...)）。
            if (subtitleText.isNotBlank()) {
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .align(Alignment.BottomCenter)
                        .padding(bottom = PlayerTokens.bottomScrimHeight.dp),
                ) {
                    PlayerSubtitleLayer(text = subtitleText)
                }
            }

            // ---- 底栏 ----
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .align(Alignment.BottomCenter),
            ) {
                PlayerBottomBar(
                    isPlaying = isPlaying,
                    positionMs = positionMs,
                    durationMs = durationMs,
                    thumbnailBitmap = thumbnailBitmap,
                    previewMs = previewMs,
                    resolutionLabel = resolutionLabel,
                    activeAction = activeAction,
                    onPlayPause = onPlayPause,
                    onNext = onNext,
                    onScrub = onScrub,
                    onSeek = onSeek,
                    onTextAction = onTextAction,
                )
            }

            // ---- 顶栏 ----
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .align(Alignment.TopCenter),
            ) {
                PlayerTopBar(
                    title = title,
                    speedText = speedText,
                    activeAction = activeAction,
                    onClose = onClose,
                    onAction = onAction,
                )
            }
        }

        // ---- 面板（右侧贴边，盖在底栏之上）----
        //
        // 【为什么空列表也要显示面板】铁律：不许静默降级。
        // 会话还没起播时音轨/字幕列表本来就是空的 —— 但那不等于"没有音轨"，
        // 而等于"还读不到"。所以显示一行明确说明，而不是假装没这个功能。
        //
        // 【为什么要包一层 Box】同底栏/顶栏：`Modifier.align()` 只在**同一个
        // BoxScope 的直接子元素**上有效，作为参数传进子组件后就失效了。
        if (panelOpen) {
            Box(
                modifier = Modifier
                    .align(Alignment.CenterEnd)
                    .padding(
                        end = 0.dp,
                        top = PlayerTokens.topBarHeight.dp,
                        bottom = PlayerTokens.bottomScrimHeight.dp,
                    ),
            ) {
                PlayerOptionPanel(
                    title = panelTitle,
                    items = if (panelItems.isEmpty()) {
                        listOf(PlayerOptionItem(title = "暂无可选项（内核尚未就绪）"))
                    } else {
                        panelItems
                    },
                    onSelect = onPanelSelect,
                    onDismiss = onDismissPanel,
                )
            }
        }

        // ---- 手势层（**最上层**，但只在没有可交互控件时启用）----
        //
        // 【为什么放在最后】它必须盖住一切，否则底栏的按钮会被它吃掉。
        // 【为什么只在控制条隐藏时启用】控制条可见时，它的按钮/进度条才是有效点击目标；
        // 此时若手势层也在，单击按钮会**同时**触发"收起控制条"，按钮就永远按不准。
        PlayerGestureLayer(
            // 面板开着时也禁用手势层：否则点面板外面会**同时**收起控制条，
            // 用户想"关面板"却连控制条一起没了。
            enabled = !visible && !panelOpen,
            onTap = onTap,
            onDoubleTap = onDoubleTap,
        )

        // ---- 状态层（永远在最上：加载/失败必须压住一切）----
        PlayerStatusLayer(
            buffering = buffering,
            errorText = errorText,
            onRetry = onRetry,
        )
    }
}
