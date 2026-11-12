package com.kivo.cinema.ui.compose

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableLongStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import com.kivo.cinema.ui.compose.player.PlayerOverlay
import com.kivo.cinema.ui.compose.player.PlayerSourceOrigin
import com.kivo.cinema.ui.compose.player.panelTitle
import com.kivo.cinema.ui.compose.player.rememberPanelItems

/**
 * 播放页的**层叠接线**（组合根的"线"，253-v3）。
 *
 * 状态所有权仍在 [PlayerComposeActivity]（面板数据源来自会话，会话归 Activity 管）；
 * 这里只把状态翻译成 [PlayerOverlay] 的参数、把回调翻译回 Router 扩展函数。
 * 分出来的理由：Activity 要守住 ≤300 行，而接线本身就有六十余条 ——
 * 混在一起时"改一个回调"要在状态字段堆里翻找。
 *
 * 叠放顺序：画面 → 弹幕 → 控制层（[PlayerOverlay]）→ 投屏 sheet。
 * 弹幕只压画面，不压控制层；拖动时弹幕让位给预览浮窗。
 */
@Composable
internal fun PlayerComposeActivity.PlayerRoot() {
    // 拖动预览的节流锚点（毫秒）：跨重组保留，Activity 级状态不必进字段堆。
    var lastScrubMs by remember { mutableLongStateOf(-1L) }
    var lastScrubWallMs by remember { mutableLongStateOf(0L) }
    MaterialTheme(colorScheme = darkColorScheme()) {
        Box(modifier = Modifier.fillMaxSize().background(Color.Black)) {
            PlayerScreen(
                onSurfaceReady = { holder -> maybeStartPlayback(holder) },
                onSurfaceLost = { onSurfaceLost() },
            )
            // 弹幕在控制层**之下**：REX 的弹幕只压画面，不压栏/面板/预览浮窗；
            // 拖动时整体让位（scrubbing），浮窗上不许飘字。
            com.kivo.cinema.danmaku.DanmakuOverlay(
                items = danmakuItems,
                positionMs = positionMs,
                isPlaying = isPlaying,
                visible = danmakuOn && !pipActive && !scrubbing,
                settings = danmakuSettings,
                modifier = Modifier.fillMaxSize(),
            )
            PlayerOverlay(
                visible = controlsVisible,
                title = currentTitle(),
                speedText = speedText,
                isPlaying = isPlaying,
                positionMs = positionMs,
                durationMs = durationMs,
                bufferedFraction = bufferedFraction,
                thumbnailBitmap = thumbnailBitmap,
                thumbUnavailable = thumbnailUnavailableReason,
                previewMs = previewMs,
                resolutionLabel = resolutionLabel(),
                buffering = statusText.contains("缓冲") || statusText.contains("连接中"),
                errorText = if (statusText.contains("失败") || statusText.contains("中断")) statusText else "",
                activeAction = activeAction,
                activePanel = activePanel,
                panelTitle = panelTitle(activePanel),
                panelItems = rememberPanelItems(
                    activePanel, mpvNetSession, danmakuOn,
                    danmakuSettings, PlayerSourceOrigin.from(intent), panelRevision,
                ),
                flashZone = flashZone,
                toastText = toastText,
                hudKind = hudKind,
                hudFraction = hudFraction,
                longPressSpeedLabel = if (longPressActive) formatSpeed(longPressSpeed) else "",
                longPressLocked = longPressLocked,
                screenLocked = screenLocked,
                lockHintVisible = lockHintVisible,
                audioOnly = audioOnly,
                danmakuOn = danmakuOn,
                pipActive = pipActive,
                onClose = { finish() },
                onHome = { onHome() },
                onDismissPanel = {
                    // 关面板也是一次交互：不刷新计时，栏会在面板刚关时就自隐。
                    lastTouchMs = System.currentTimeMillis()
                    activePanel = ""
                },
                onAction = { kind -> onTopAction(kind) },
                onPlayPause = { togglePlayPause() },
                onLongPressPlay = { onToggleLock() },
                onNext = { onNextEpisode() },
                onScrub = { ms ->
                    lastTouchMs = System.currentTimeMillis()
                    scrubbing = true
                    // 120ms 媒体时间 + 40ms 墙钟双节流：短片源快拖时单靠媒体节流
                    // 会到 66Hz seek（每 15ms 一次），堆解码队列；墙钟底封顶 25Hz。
                    val now = android.os.SystemClock.elapsedRealtime()
                    if (lastScrubMs < 0 || (ms - lastScrubMs >= 120 && now - lastScrubWallMs >= 40)) {
                        lastScrubMs = ms
                        lastScrubWallMs = now
                        thumbnailReader?.seekTo(ms * 1000)
                    }
                },
                onSeek = { ms ->
                    scrubbing = false
                    seekTo(ms)
                },
                onTextAction = { kind -> onBottomTextAction(kind) },
                onPanelSelect = { item -> onPanelSelect(item) },
                onPanelStep = { item, delta -> onPanelStep(item, delta) },
                onCapture = { onCapture() },
                onEpisodeOrder = { onEpisodeOrder() },
                onTap = {
                    lastTouchMs = System.currentTimeMillis()
                    // 已锁定的快进：点画面 = 解除锁定并恢复原速。
                    if (longPressLocked) unlockLongPress() else controlsVisible = !controlsVisible
                },
                onLockHint = { onLockHint() },
                onUnlock = { onToggleLock() },
                onDoubleTap = { zone -> onDoubleTap(zone) },
                onVerticalDragStart = { x -> onVerticalDragStart(x) },
                onVerticalDrag = { dy -> onVerticalDrag(dy) },
                onVerticalDragEnd = { onVerticalDragEnd() },
                onLongPressStart = { x, y -> onLongPressStart(x, y) },
                onLongPressDrag = { x, y -> onLongPressDrag(x, y) },
                onLongPressEnd = { onLongPressEnd() },
                onRetry = { onRetry() },
            )
            com.kivo.cinema.dlna.CastSheet(
                visible = castSheetVisible && !pipActive,
                scanning = castScanning,
                devices = castDevices,
                message = castMessage,
                onSelect = { onCastSelect(it) },
                onRescan = { openCastSheet() },
                onDismiss = { castSheetVisible = false },
            )
        }
    }
}