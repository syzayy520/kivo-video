package com.kivo.cinema.ui.compose.player

import android.graphics.Bitmap
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.scaleIn
import androidx.compose.animation.scaleOut
import androidx.compose.animation.slideInVertically
import androidx.compose.animation.slideOutVertically
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

/**
 * 播放页覆盖层编排（player-ui-v2.md §二 的四层信息架构）。
 *
 * ## 叠放顺序（从下到上）—— 这一条是**功能正确性**，不只是视觉
 * ```
 *   状态层（加载/失败）      ← 最上，压住一切
 *   面板（倍速/音轨/画质）    ← 盖在控制条之上
 *   底栏 + 顶栏
 *   字幕层
 *   手势层                  ← **最下面**（兜底点画面）
 * ```
 * 手势层必须在**最下面**：它是全屏的 pointerInput，
 * 放上面会把顶栏/底栏/面板的点击全吃掉（第一版实机：按钮全部点不动）。
 * 放下面之后它们在**上层**，先拿到事件；点画面才落到手势层兜底。
 *
 * ## 过渡（REX 交互 M08-03 / M05-S01 的同族做法）
 * · 控制条：淡入淡出 + 轻微纵向位移（180ms）
 * · 面板：淡入 + 0.96→1 缩放（200ms）
 * · 手势闪示：居中图标 0.5s 后淡出
 * —— 第一版是"瞬现瞬消"，观感很硬，这一轮补上。
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
    flashZone: DoubleTapZone?,
    toastText: String,
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
    onDoubleTap: (DoubleTapZone) -> Unit,
    onRetry: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Box(modifier = modifier.fillMaxSize()) {
        // ---- 手势层（最下面，兜底"点画面"）----
        // 面板开着时，点画面 = **关掉面板**（而不是收起控制条）——
        // 用户想关面板却连控制条一起没了，是很恼人的。
        PlayerGestureLayer(
            onTap = { if (panelOpen) onDismissPanel() else onTap() },
            onDoubleTap = onDoubleTap,
        )

        // ---- 字幕层（在底栏**之上**，两者不相交）----
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

        // ---- 顶栏 + 底栏（带淡入淡出）----
        AnimatedVisibility(
            visible = visible,
            enter = fadeIn(tween(180)) + slideInVertically(tween(180)) { -it / 6 },
            exit = fadeOut(tween(140)) + slideOutVertically(tween(140)) { -it / 6 },
            modifier = Modifier.align(Alignment.TopCenter),
        ) {
            PlayerTopBar(
                title = title,
                speedText = speedText,
                activeAction = activeAction,
                onClose = onClose,
                onAction = onAction,
            )
        }

        AnimatedVisibility(
            visible = visible,
            enter = fadeIn(tween(180)) + slideInVertically(tween(180)) { it / 6 },
            exit = fadeOut(tween(140)) + slideOutVertically(tween(140)) { it / 6 },
            modifier = Modifier.align(Alignment.BottomCenter),
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

        // ---- 面板（右侧贴边，盖在控制条之上）----
        //
        // 【为什么空列表也要显示面板】铁律：不许静默降级。
        // 会话还没起播时音轨/字幕列表本来就是空的 —— 但那不等于"没有音轨"，
        // 而等于"还读不到"。所以显示一行明确说明，而不是假装没这个功能。
        //
        // 【为什么要包一层 Box】`Modifier.align()` 只在**同一个 BoxScope 的
        // 直接子元素**上有效，作为参数传进子组件后就失效了。
        AnimatedVisibility(
            visible = panelOpen,
            enter = fadeIn(tween(200)) + scaleIn(tween(200), initialScale = 0.96f),
            exit = fadeOut(tween(150)) + scaleOut(tween(150), targetScale = 0.96f),
            modifier = Modifier
                .align(Alignment.CenterEnd)
                .padding(
                    end = 0.dp,
                    top = PlayerTokens.topBarHeight.dp,
                    // 【为什么盖住底栏】参考的面板就是浮在底栏之上的。
                    // 让开底栏会让可用高度只剩 253dp —— 7 档倍速（7×42+标题≈330dp）
                    // 装不下，最后一档被切一半、还得滚动才能选到，很别扭。
                    bottom = 16.dp,
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

        // ---- 轻提示（浮在底栏之上，不遮挡画面中央）----
        PlayerToast(
            text = toastText,
            modifier = Modifier
                .align(Alignment.BottomCenter)
                .padding(bottom = PlayerTokens.bottomScrimHeight.dp + 12.dp),
        )

        // ---- 手势闪示（双击快进/快退/播放暂停的确认）----
        if (flashZone != null) {
            PlayerGestureFlash(
                zone = flashZone,
                modifier = Modifier.align(Alignment.Center),
            )
        }

        // ---- 状态层（永远在最上：加载/失败必须压住一切）----
        PlayerStatusLayer(
            buffering = buffering,
            errorText = errorText,
            onRetry = onRetry,
        )
    }
}
