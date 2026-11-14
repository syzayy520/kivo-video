package com.kivo.cinema.ui.compose

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
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
    // 273 v27：**待预取的落点**（拖动中每次变，落点预取只认它）。
    // 【为什么要"待"】拖动时每 50 ms 就来一个位置 ⇒ 每来一个就预取一次
    // ⇒ 真机一次拖动开了 19 条源站连接（v26 的教训：源站带宽封顶，并发互踩，
    // 落点那一跳反而从 2.5 s 变成 4.5 s）。
    // 改成"停下来 200 ms 之后再预取"：位置不变就不换窗口，也不新开连接。
    var pendingPrefetchMs by remember { mutableLongStateOf(-1L) }
    LaunchedEffect(pendingPrefetchMs) {
        if (pendingPrefetchMs >= 0L) {
            kotlinx.coroutines.delay(SCRUB_SETTLE_MS)
            urlPrep?.source?.prefetchSeek(pendingPrefetchMs)
            // 273 v30：预览**在落点窗口开起来之后**才 seek。
            // 【为什么】预览内核每 seek 一次就要自己回源站拉一份字节
            // （真机 v29：拖动 900 ms 里预览下掉 5 MiB）⇒ 与落点那一跳抢带宽，
            // 真机落点只剩 1 MB/s（源站封顶 ~3.6 MB/s）⇒ 首帧多等 1.4 s。
            // 【为什么这样就省了】窗口是**共用**的（预览与播放借同一份缓存）
            // ⇒ 预览的字节从**正在下载的落点窗口**里取，不再自己拉一份。
            // 【顺带修好"预览位置是错的"】原来预览每 50 ms 被 seek 一次，
            // 而它自己 seek+解码要 ~1 s ⇒ 浮窗显示的永远是 1–2 秒前的位置。
            if (playbackPrefs.scrubPreview) thumbnailReader?.seekTo(pendingPrefetchMs * 1000)
        }
    }
    MaterialTheme(colorScheme = darkColorScheme()) {
        Box(modifier = Modifier.fillMaxSize().background(Color.Black)) {
            PlayerScreen(
                onSurfaceReady = { holder -> maybeStartPlayback(holder) },
                onSurfaceLost = { onSurfaceLost() },
            )
            // 弹幕在控制层**之下**：弹幕只压画面，不压栏/面板/预览浮窗；
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
                showRemaining = playbackPrefs.showRemaining,
                scrubPreview = playbackPrefs.scrubPreview,
                showNextButton = playbackPrefs.showNextButton,
                showCaptureButton = playbackPrefs.showCaptureButton,
                bufferedFraction = bufferedFraction,
                thumbnailBitmap = thumbnailBitmap,
                thumbUnavailable = thumbnailUnavailableReason,
                previewMs = previewMs,
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
                        // 273 v26/v27：落点预取**不在这里直接发**。
                        // 【为什么】拖动中每 50 ms 一个位置 ⇒ 每来一个就开一条
                        // 4 MiB 的源站连接 ⇒ 真机一次拖动 19 条 ⇒ 源站连接占满、
                        // 落点那一跳更慢（v26 实测 2.5 s → 4.5 s）。
                        // 改成"停下来才预取"（见上面的 pendingPrefetchMs）；
                        // 松手那一刻由 seekTo 强制发起（force = true）。
                        pendingPrefetchMs = ms
                        // 273 v30：**不再在这里 seek 预览**。
                        // 【为什么】每 50 ms 一个位置 ⇒ 每来一个就让预览内核
                        // 回源站拉一份字节（真机 v29：一次拖动预览下掉 5 MiB），
                        // 而且它 seek+解码要 ~1 s ⇒ 浮窗显示的永远是旧位置。
                        // 预览改为跟着落点窗口走（见上面的 settle 分支）。
                        // 273 v29：拖动期间让源站给落点那一跳让路 ——
                        // 主链路的预读线程停手（别再下用户已经放弃的位置），
                        // 保温线程也别占连接；落点那一跳独占带宽与连接。
                        urlPrep?.source?.yieldSource(SCRUB_YIELD_MS)
                    }
                },
                onSeek = { ms ->
                    scrubbing = false
                    // 取消"停下来再预取"的那一次：松手这一刻由 seekTo 强制发起，
                    // 留着它会在 200 ms 后把刚落点的窗口换掉（白下一份）。
                    pendingPrefetchMs = -1L
                    seekTo(ms)
                    // 273 v30：松手这一刻预览要落在**最终位置**。
                    // 【为什么放这里】拖动途中的位置已经不再 seek 预览了
                    // （那会自己回源站拉字节，见上面）⇒ 浮窗必须在这里归位，
                    // 否则松手后浮窗还停在拖动途中的老位置。
                    // 落点窗口此时已经在下 ⇒ 预览的字节从窗口取，不再回源站。
                    if (playbackPrefs.scrubPreview) thumbnailReader?.seekTo(ms * 1000)
                },
                onScrubEnd = { scrubbing = false },
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

/**
 * 拖动停稳的判定（毫秒）。
 *
 * 【为什么是 200】拖动事件最快 50 ms 一个（真机）。200 ms 停顿 = 用户的手
 * 停了 ⇒ 那个位置多半就是要落的位置 ⇒ 这时才值得为它开一条源站连接。
 * 更快（比如 50 ms）⇒ 又回到 v26 的"每个位置一条连接"。
 */
private const val SCRUB_SETTLE_MS = 200L

/**
 * 拖动期间"让路"的时长（毫秒）。
 *
 * 【为什么是 1500】闸门只延长不缩短 ⇒ 拖动中每次位置变化都把它往后推。
 * 1.5 s 覆盖"用户手停了一下又继续拖"的常见节奏 ⇒ 闸门在整个拖动期间保持打开；
 * 松手后 1.5 s 自动失效，主链路恢复预读（那时它下的是新位置，有用）。
 */
private const val SCRUB_YIELD_MS = 1500L