package com.kivo.cinema.ui.compose

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import com.kivo.cinema.ui.compose.player.PlayerOverlay
import com.kivo.cinema.ui.compose.player.PlayerSourceOrigin
import com.kivo.cinema.ui.compose.player.panelTitle
import com.kivo.cinema.ui.compose.player.rememberPanelItems
import com.kivo.cinema.ui.compose.player.rememberPinchScale
import com.kivo.cinema.ui.compose.player.subtitle.onSubtitleLongPress
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.ui.input.pointer.PointerEventPass
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.offset
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.compose.player.PlayerTokens
import com.kivo.cinema.ui.design.DesignTokens
import com.kivo.cinema.ui.subtitles.online.OnlineSubtitleTab
import kotlinx.coroutines.delay

@Composable
internal fun PlayerComposeActivity.PlayerRoot() {
    val handSeek = remember { HandSeekState() }
    val pinch = rememberPinchScale(screenLocked)
    LaunchedEffect(scrub.active, scrub.targetMs, scrub.revision) {
        if (scrub.active) {
            val target = scrub.targetMs
            // 稳定 200ms 才开落点窗口，避免拖动时为每个位置争抢源站连接。
            delay(200)
            urlPrep?.source?.prefetchSeek(target)
            if (playbackPrefs.scrubPreview && previewRequestedMs != target) {
                previewRequestedMs = target
                thumbnailReader?.seekTo(target * 1000)
            }
        }
    }
    LaunchedEffect(scrub.active, scrub.revision) {
        if (!scrub.active && scrub.visible) {
            delay(400)
            scrub.hide()
            thumbnailReader?.invalidate()
            previewMs = -1L
        }
    }
    DisposableEffect(Unit) {
        onDispose { handSeek.clear(); cancelScrub(); onLongPressCancel() }
    }
    MaterialTheme(colorScheme = darkColorScheme()) {
        // P9 取证（dev-only）：`--ez kivo.touchLog true` 记录每个 down 事件在
        // 根 Box 的 px 坐标 —— 校准 adb 注入坐标与界面坐标的仿射映射用。
        val touchLog = intent.getBooleanExtra("kivo.touchLog", false)
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(Color.Black)
                .then(
                    if (touchLog) {
                        // 不消费事件（awaitEachGesture 里只读不写），避免改变界面行为。
                        Modifier.pointerInput(Unit) {
                            awaitEachGesture {
                                val down = awaitPointerEvent(PointerEventPass.Final)
                                down.changes.forEach { ch ->
                                    if (ch.pressed && !ch.previousPressed) {
                                        smbTrace(
                                            "touch probe x=${ch.position.x.toInt()} " +
                                                "y=${ch.position.y.toInt()} consumed=${ch.isConsumed}",
                                        )
                                    }
                                }
                            }
                        }
                    } else {
                        Modifier
                    },
                ),
        ) {
            PlayerScreen(
                onSurfaceReady = { holder -> maybeStartPlayback(holder) },
                onSurfaceLost = { onSurfaceLost() },
                modifier = Modifier.graphicsLayer {
                    scaleX = pinch.scale
                    scaleY = pinch.scale
                },
            )
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
                mpvSession = mpvNetSession,
                title = currentTitle(),
                speedText = speedText,
                isPlaying = isPlaying,
                positionMs = positionMs,
                durationMs = durationMs,
                showRemaining = playbackPrefs.showRemaining,
                scrubPreview = playbackPrefs.scrubPreview,
                showNextButton = playbackPrefs.showNextButton,
                showCaptureButton = playbackPrefs.showCaptureButton,
                bufferedFraction = bufferedFraction,
                thumbnailBitmap = thumbnailBitmap,
                thumbUnavailable = thumbnailUnavailableReason,
                scrub = scrub,
                resolutionLabel = resolutionLabel(),
                phase = phase,
                activeAction = activeAction,
                activePanel = activePanel,
                panelTitle = panelTitle(activePanel),
                panelItems = rememberPanelItems(
                    activePanel, mpvNetSession, danmakuOn,
                    danmakuSettings, PlayerSourceOrigin.from(intent), panelRevision,
                ),
                flashZone = flashZone,
                flashEventId = flashEventId,
                toastText = toastText,
                hudKind = hudKind,
                hudFraction = hudFraction,
                hudOriginFraction = hudOriginFraction,
                hudDragging = hudDragging,
                longPressSpeedLabel = if (longPressActive || longPressLocked) formatSpeed(longPressSpeed) else "",
                longPressLocked = longPressLocked,
                longPressActive = longPressActive,
                screenLocked = screenLocked,
                gestureEpoch = gestureEpoch,
                pinch = pinch,
                lockHintVisible = lockHintVisible,
                audioOnly = audioOnly,
                danmakuOn = danmakuOn,
                pipActive = pipActive,
                onClose = { finish() },
                onHome = { onHome() },
                onDismissPanel = {
                    lastTouchMs = System.currentTimeMillis()
                    smbTrace("panelDismiss (was=$activePanel)")
                    activePanel = ""
                },
                onAction = { kind -> onTopAction(kind) },
                onPlayPause = { togglePlayPause() },
                onLongPressPlay = { onToggleLock() },
                onNext = { onNextEpisode() },
                onScrub = { ms ->
                    if (!scrub.active) gestureEpoch++
                    updateScrub(ms)
                },
                onSeek = { ms -> commitScrub(ms) },
                onScrubEnd = { cancelScrub() },
                onTextAction = { kind -> onBottomTextAction(kind) },
                onPanelSelect = { item -> onPanelSelect(item) },
                onPanelStep = { item, delta -> onPanelStep(item, delta) },
                onPanelLongPress = { item -> onSubtitleLongPress(item) },
                onCapture = { onCapture() },
                onEpisodeOrder = { onEpisodeOrder() },
                onTap = {
                    lastTouchMs = System.currentTimeMillis()
                    if (longPressLocked) unlockLongPress() else controlsVisible = !controlsVisible
                },
                onLockHint = { onLockHint() },
                onUnlock = { onToggleLock() },
                onDoubleTap = { zone -> onDoubleTap(zone) },
                onHorizontalDragStart = { onHorizontalDragStart(handSeek) },
                onHorizontalDrag = { dx -> onHorizontalDrag(handSeek, dx) },
                onHorizontalDragEnd = { commit -> onHorizontalDragEnd(handSeek, commit) },
                onVerticalDragStart = { x -> onVerticalDragStart(x) },
                onVerticalDrag = { dy -> onVerticalDrag(dy) },
                onVerticalDragEnd = { onVerticalDragEnd() },
                onLongPressStart = { x, y -> onLongPressStart(x, y) },
                onLongPressDrag = { x, y -> onLongPressDrag(x, y) },
                onLongPressEnd = { onLongPressEnd() },
                onLongPressCancel = { onLongPressCancel() },
                onRetry = { onRetry() },
            )
            // P10 §17：浮卡贴「投屏」图标下方。用**实际约束**算 x，不用 screenWidthDp ——
            // 后者在本机会比顶栏坐标系少 30dp，卡片会整体左偏。
            BoxWithConstraints(Modifier.fillMaxSize()) {
                val cardW = PlayerTokens.panelWidth
                val cardX = (maxWidth.value - 380f - cardW / 2f)
                    .coerceIn(16f, (maxWidth.value - cardW - 16f).coerceAtLeast(16f))
                com.kivo.cinema.dlna.CastSheet(
                visible = castSheetVisible && !pipActive,
                scanning = castScanning,
                devices = castDevices,
                message = castMessage,
                onSelect = { onCastSelect(it) },
                onRescan = { openCastSheet() },
                onDismiss = { castSheetVisible = false },
                // P10 §17：浮卡锚在投屏图标下方（不再全宽贴底）。
                modifier = Modifier
                    .align(Alignment.TopStart)
                    .offset(x = cardX.dp, y = 82.dp),
                )
            }
            // 在线字幕面板（subtitle/online）：全屏覆盖，数据与播放只读分离。
            if (showOnlineSubtitle) {
                Box(Modifier.fillMaxSize().background(DesignTokens.pageBackground)) {
                    OnlineSubtitleTab(onClose = { showOnlineSubtitle = false })
                }
            }
        }
    }
}
