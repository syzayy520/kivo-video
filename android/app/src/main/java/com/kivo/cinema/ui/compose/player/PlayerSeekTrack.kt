package com.kivo.cinema.ui.compose.player

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.spring
import androidx.compose.animation.core.tween
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.boundsInWindow
import androidx.compose.ui.layout.onGloballyPositioned
import androidx.compose.ui.semantics.ProgressBarRangeInfo
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.progressBarRangeInfo
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.setProgress
import androidx.compose.ui.unit.dp
import com.kivo.cinema.ui.compose.player.seek.PlayerScrubState
import com.kivo.cinema.ui.compose.player.seek.PlayerSeekPreviewAnchor
import com.kivo.cinema.ui.compose.player.seek.PlayerSeekRail
import com.kivo.cinema.ui.motion.MotionGate
import kotlin.math.roundToLong

/**
 * 进度轨道：命中、拖拽手势与轨道本体。
 * 预览卡不在此渲染 —— 它挂 overlay 根 Box，靠 [previewAnchor] 回传的窗口坐标落位。
 */
@Composable
internal fun PlayerSeekTrack(
    positionMs: Long,
    durationMs: Long,
    bufferedFraction: Float,
    scrub: PlayerScrubState,
    previewAnchor: PlayerSeekPreviewAnchor,
    onScrub: (Long) -> Unit,
    onSeek: (Long) -> Unit,
    onScrubEnd: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val currentDuration by rememberUpdatedState(durationMs)
    val currentScrub by rememberUpdatedState(onScrub)
    val currentSeek by rememberUpdatedState(onSeek)
    val currentEnd by rememberUpdatedState(onScrubEnd)
    val reduced = MotionGate.rememberAppReduceMotion()
    val press by animateFloatAsState(
        targetValue = if (scrub.active) 1f else 0f,
        animationSpec = if (reduced || scrub.active) tween(0)
            else spring(dampingRatio = 1f, stiffness = 900f),
        label = "seekGrip",
    )
    val shownMs = if (scrub.visible) scrub.targetMs else positionMs
    val fraction = if (durationMs > 0L) shownMs.toFloat() / durationMs else 0f
    val origin = if (durationMs > 0L) scrub.originMs.toFloat() / durationMs else 0f
    val hitHeight = (PlayerTokens.trackHitHeight + 24f).dp

    BoxWithConstraints(
        modifier = modifier.fillMaxWidth().height(hitHeight)
            .onGloballyPositioned { previewAnchor.trackBounds = it.boundsInWindow() }
            .semantics {
                contentDescription = if (durationMs > 0) "播放进度" else "播放进度：时长尚未就绪"
                if (durationMs > 0) {
                    progressBarRangeInfo = ProgressBarRangeInfo(fraction.coerceIn(0f, 1f), 0f..1f)
                    setProgress { target ->
                        val ms = (target.coerceIn(0f, 1f) * durationMs).roundToLong()
                        onScrub(ms)
                        onSeek(ms)
                        true
                    }
                }
            }
            .pointerInput(Unit) {
                awaitEachGesture {
                    val down = awaitFirstDown()
                    down.consume()
                    var committed = false
                    fun update(x: Float) {
                        val width = (size.width - 14.dp.toPx()).coerceAtLeast(1f)
                        val value = ((x - 7.dp.toPx()) / width).coerceIn(0f, 1f)
                        val target = (value.toDouble() * currentDuration).roundToLong()
                        currentScrub(target)
                    }
                    try {
                        update(down.position.x)
                        while (true) {
                            val event = awaitPointerEvent()
                            if (event.changes.count { it.pressed } > 1) break
                            val change = event.changes.firstOrNull { it.id == down.id } ?: break
                            if (change.isConsumed) break
                            update(change.position.x)
                            change.consume()
                            if (!change.pressed) {
                                if (currentDuration > 0L) {
                                    currentSeek(scrub.targetMs)
                                    committed = true
                                }
                                break
                            }
                        }
                    } finally {
                        if (!committed) currentEnd()
                    }
                }
            },
    ) {
        PlayerSeekRail(
            fraction = fraction,
            bufferedFraction = bufferedFraction,
            originFraction = origin,
            feedbackVisible = scrub.visible,
            press = press,
            modifier = Modifier.fillMaxWidth().height(hitHeight),
        )
    }
}