package com.kivo.cinema.ui.compose.player

import android.graphics.Bitmap
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.runtime.Composable
import androidx.compose.runtime.State
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.unit.dp

/**
 * 播放页底部区（v3 §1.2 / §1.4 形态 11、12）。
 *
 * 三种形态共用一个文件，因为它们是**同一条底栏的三种内容**：
 * · 常态：`[时间] ─轨道─ [总长]` 一行 + 按钮行一行（实测轨道心 304.8 / 按钮心 345）；
 * · 资源态（点 4K）：轨道行上移、插入 chips+源卡、**按钮行消失**（实测）；
 * · 选集态（点选集）：整条换成横滑选集带（实测）。
 *
 * 文字件（TimeText / BarTextButton / formatPlayerTime）在 [PlayerBarText]。
 *
 * 【v2 的死按钮是怎么修的】v2 用固定 `height(24)` 装 17sp 文字 ——
 * 行盒比容器高 ⇒ 文字被裁，**被裁的文字连命中区一起被裁**。
 * v3：按钮行高封顶 24pt、命中盒纵向溢出（Row 不裁切），图标命中盒 44pt。
 */
@Composable
internal fun PlayerBottomBar(
    isPlaying: Boolean,
    positionMs: Long,
    durationMs: Long,
    showRemaining: Boolean,
    scrubPreview: Boolean,
    showNextButton: Boolean,
    bufferedFraction: Float,
    thumbnailBitmap: Bitmap?,
    thumbUnavailable: String,
    previewMs: Long,
    resolutionLabel: String,
    activeAction: String,
    expandedPanel: String,
    panelItems: List<PlayerOptionItem>,
    panelRect: State<Rect?>,
    title: String,
    onPlayPause: () -> Unit,
    onLongPressPlay: () -> Unit,
    onNext: () -> Unit,
    onScrub: (Long) -> Unit,
    onSeek: (Long) -> Unit,
    onScrubEnd: () -> Unit,
    onTextAction: (String) -> Unit,
    onPanelSelect: (PlayerOptionItem) -> Unit,
    onCapture: () -> Unit,
    onEpisodeOrder: () -> Unit,
    modifier: Modifier = Modifier,
) {
    var dragTargetMs by remember { mutableStateOf<Long?>(null) }

    Box(modifier = modifier.fillMaxWidth()) {
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(
                    when (expandedPanel) {
                        "episodes" -> 200.dp
                        "resource" -> 210.dp
                        else -> PlayerTokens.bottomScrimHeight.dp
                    },
                )
                .align(Alignment.BottomCenter)
                .background(
                    Brush.verticalGradient(
                        listOf(PlayerTokens.bottomScrimTop, PlayerTokens.bottomScrimBottom),
                    ),
                ),
        )

        if (expandedPanel == "episodes") {
            PlayerEpisodeStrip(
                title = title,
                items = panelItems,
                onSelect = onPanelSelect,
                onOrder = onEpisodeOrder,
                modifier = Modifier
                    .align(Alignment.BottomCenter)
                    .padding(bottom = 60.dp),
            )
            return@Box
        }

        Column(
            modifier = Modifier
                .fillMaxWidth()
                .align(Alignment.BottomCenter)
                .playerSafeArea(WindowInsetsSides.Horizontal)
                .padding(
                    start = PlayerTokens.bottomSidePad.dp,
                    end = PlayerTokens.bottomSidePad.dp,
                    bottom = PlayerTokens.bottomPad.dp,
                ),
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                TimeText(text = formatPlayerTime(dragTargetMs ?: positionMs))
                Spacer(Modifier.width(PlayerTokens.trackStartGap.dp))
                PlayerSeekTrack(
                    positionMs = positionMs,
                    durationMs = durationMs,
                    bufferedFraction = bufferedFraction,
                    thumbnailBitmap = thumbnailBitmap,
                    thumbUnavailable = thumbUnavailable,
                    previewMs = previewMs,
                    scrubPreview = scrubPreview,
                    onDragTarget = { dragTargetMs = it },
                    onScrub = onScrub,
                    onSeek = onSeek,
                    onScrubEnd = onScrubEnd,
                    onCapture = onCapture,
                    modifier = Modifier.weight(1f),
                )
                Spacer(Modifier.width(PlayerTokens.trackEndGap.dp))
                TimeText(
                    text = if (showRemaining && durationMs > 0)
                        "-" + formatPlayerTime((durationMs - (dragTargetMs ?: positionMs)).coerceAtLeast(0L))
                    else formatPlayerTime(durationMs),
                    modifier = Modifier.hideWhenCoveredByPanel(panelRect),
                )
            }

            if (expandedPanel == "resource") {
                PlayerResourcePanel(items = panelItems, onSelect = onPanelSelect)
            } else {
                Spacer(Modifier.height(PlayerTokens.buttonRowGap.dp))
                ButtonRow(
                    isPlaying = isPlaying,
                    resolutionLabel = resolutionLabel,
                    activeAction = activeAction,
                    panelRect = panelRect,
                    showNextButton = showNextButton,
                    onPlayPause = onPlayPause,
                    onLongPressPlay = onLongPressPlay,
                    onNext = onNext,
                    onTextAction = onTextAction,
                )
            }
        }
    }
}

@Composable
private fun ButtonRow(
    isPlaying: Boolean,
    resolutionLabel: String,
    activeAction: String,
    panelRect: State<Rect?>,
    showNextButton: Boolean,
    onPlayPause: () -> Unit,
    onLongPressPlay: () -> Unit,
    onNext: () -> Unit,
    onTextAction: (String) -> Unit,
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            // 行高**封顶** 24pt：44pt 命中盒与文字 padding 都允许纵向溢出
            // （Row 不裁切），否则命中盒会把行撑到 44pt、轨道被顶高 20pt
            // （真机实测 287dp vs 基线 304.8pt）。
            .height(PlayerTokens.buttonRowHeight.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Row(
            modifier = Modifier.offset(x = -PlayerTokens.playGroupInset.dp),
            horizontalArrangement = Arrangement.spacedBy(
                (PlayerTokens.playButtonPitch - PlayerTokens.actionHitBox).dp,
            ),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            BarIconButton(
                kind = if (isPlaying) "pause" else "play",
                glyphSize = 24f,
                onClick = onPlayPause,
                // 长按播放键 = 屏幕锁定（◐ 入口，未录到；见设计稿 §六）。
                onLongClick = onLongPressPlay,
            )
            if (showNextButton) BarIconButton(kind = "next", glyphSize = 18f, onClick = onNext)
        }

        Spacer(Modifier.weight(1f))

        Row(
            horizontalArrangement = Arrangement.spacedBy(PlayerTokens.textButtonGap.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            if (resolutionLabel.isNotBlank()) {
                BarTextButton(
                    text = resolutionLabel,
                    active = activeAction == "resource",
                    panelRect = panelRect,
                    onClick = { onTextAction("resource") },
                )
            }
            BarTextButton(
                text = "选集",
                active = activeAction == "episodes",
                panelRect = panelRect,
                onClick = { onTextAction("episodes") },
            )
            BarTextButton(
                text = "倍速",
                active = activeAction == "speed",
                panelRect = panelRect,
                onClick = { onTextAction("speed") },
            )
        }
    }
}

@Composable
private fun BarIconButton(
    kind: String,
    glyphSize: Float,
    onClick: () -> Unit,
    onLongClick: (() -> Unit)? = null,
) {
    var pressed by remember { mutableStateOf(false) }
    Box(
        modifier = Modifier
            .size(PlayerTokens.actionHitBox.dp)
            .pointerInput(Unit) {
                detectTapGestures(
                    onPress = { pressed = true; tryAwaitRelease(); pressed = false },
                    onTap = { onClick() },
                    onLongPress = { onLongClick?.invoke() },
                )
            },
        contentAlignment = Alignment.Center,
    ) {
        PlayerGlyph(
            kind = kind,
            color = PlayerTokens.textPrimary,
            modifier = Modifier
                .size(glyphSize.dp)
                .alpha(if (pressed) PlayerTokens.pressAlpha else 1f),
        )
    }
}