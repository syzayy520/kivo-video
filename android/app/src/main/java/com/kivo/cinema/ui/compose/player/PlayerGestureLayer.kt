package com.kivo.cinema.ui.compose.player

import android.os.SystemClock
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.PointerEventPass
import androidx.compose.ui.input.pointer.PointerId
import androidx.compose.ui.input.pointer.pointerInput
import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDecision
import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDecision.Point
import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDecision.Region
import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDetector
import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDetector.Action

@Composable
internal fun PlayerGestureLayer(
    onTap: () -> Unit,
    onDoubleTap: (DoubleTapZone) -> Unit,
    locked: Boolean = false,
    pinch: PinchScaleState,
    suppressSwipe: Boolean = false,
    onHorizontalDragStart: () -> Unit = {},
    onHorizontalDrag: (Float) -> Unit = {},
    onHorizontalDragEnd: (Boolean) -> Unit = {},
    onVerticalDragStart: (Float) -> Unit = {},
    onVerticalDrag: (Float) -> Unit = {},
    onVerticalDragEnd: () -> Unit = {},
    onLongPressStart: (Float, Float) -> Unit = { _, _ -> },
    onLongPressDrag: (Float, Float) -> Unit = { _, _ -> },
    onLongPressEnd: () -> Unit = {},
    modifier: Modifier = Modifier,
    onLongPressCancel: () -> Unit = onLongPressEnd,
) {
    val currentOnTap by rememberUpdatedState(onTap)
    val currentOnDoubleTap by rememberUpdatedState(onDoubleTap)
    val currentLocked by rememberUpdatedState(locked)
    val currentPinch by rememberUpdatedState(pinch)
    val currentSuppressSwipe by rememberUpdatedState(suppressSwipe)
    val currentHDragStart by rememberUpdatedState(onHorizontalDragStart)
    val currentHDrag by rememberUpdatedState(onHorizontalDrag)
    val currentHDragEnd by rememberUpdatedState(onHorizontalDragEnd)
    val currentDragStart by rememberUpdatedState(onVerticalDragStart)
    val currentDrag by rememberUpdatedState(onVerticalDrag)
    val currentDragEnd by rememberUpdatedState(onVerticalDragEnd)
    val currentLongPressStart by rememberUpdatedState(onLongPressStart)
    val currentLongPressDrag by rememberUpdatedState(onLongPressDrag)
    val currentLongPressEnd by rememberUpdatedState(onLongPressEnd)
    val currentLongPressCancel by rememberUpdatedState(onLongPressCancel)

    fun swipeBlocked(): Boolean = currentLocked || currentSuppressSwipe

    Box(
        modifier = modifier
            .pinchZoom(pinch, currentLocked)
            .fillMaxSize()
            .pointerInput(Unit) {
                val detector = PlayerGestureDetector(PlayerGestureDecision(density)) { action ->
                    when (action) {
                        Action.Tap -> currentOnTap()
                        is Action.DoubleTap -> currentOnDoubleTap(when (action.region) {
                            Region.BACKWARD -> DoubleTapZone.BACKWARD
                            Region.TOGGLE_PLAY -> DoubleTapZone.TOGGLE_PLAY
                            Region.FORWARD -> DoubleTapZone.FORWARD
                        })
                        Action.HorizontalStart -> currentHDragStart()
                        is Action.HorizontalDrag -> currentHDrag(action.delta)
                        is Action.HorizontalEnd -> currentHDragEnd(action.commit)
                        is Action.VerticalStart -> currentDragStart(action.x)
                        is Action.VerticalDrag -> currentDrag(action.delta)
                        Action.VerticalEnd -> currentDragEnd()
                        is Action.LongPressStart -> currentLongPressStart(action.point.x, action.point.y)
                        is Action.LongPressDrag -> currentLongPressDrag(action.point.x, action.point.y)
                        is Action.LongPressEnd -> if (action.cancelled) {
                            currentLongPressCancel()
                        } else {
                            currentLongPressEnd()
                        }
                    }
                }
                var active: PointerId? = null
                var draining = false
                try {
                    awaitPointerEventScope {
                        while (true) {
                            val deadline = detector.nextDeadlineMillis
                            val event = if (deadline == null) {
                                awaitPointerEvent(PointerEventPass.Main)
                            } else {
                                withTimeoutOrNull((deadline - SystemClock.uptimeMillis()).coerceAtLeast(1L)) {
                                    awaitPointerEvent(PointerEventPass.Main)
                                }
                            }
                            if (event == null) {
                                if (currentPinch.pinching) {
                                    detector.cancel()
                                    active = null
                                    draining = currentEvent.changes.any { it.pressed }
                                } else {
                                    detector.advanceTime(SystemClock.uptimeMillis(), swipeBlocked())
                                }
                                continue
                            }
                            val pressed = event.changes.filter { it.pressed }
                            val change = event.changes.firstOrNull { it.id == active }
                            val interrupted = currentPinch.pinching || pressed.size > 1 ||
                                event.changes.any { it.isConsumed } ||
                                (active != null && event.changes.any {
                                    it.id != active && (it.pressed || it.previousPressed)
                                })
                            var claimed: PointerId? = null
                            when {
                                interrupted -> {
                                    detector.cancel()
                                    active = null
                                    draining = pressed.isNotEmpty()
                                }
                                draining -> draining = pressed.isNotEmpty()
                                active == null -> {
                                    val down = pressed.singleOrNull()?.takeIf { !it.previousPressed }
                                    if (down != null) {
                                        active = down.id
                                        detector.down(down.uptimeMillis, Point(down.position.x, down.position.y), size.width.toFloat())
                                    }
                                }
                                change == null -> {
                                    detector.cancel()
                                    active = null
                                    draining = pressed.isNotEmpty()
                                }
                                !change.pressed -> {
                                    if (detector.up(change.uptimeMillis, Point(change.position.x, change.position.y), swipeBlocked())) {
                                        change.consume()
                                        claimed = change.id
                                    }
                                    active = null
                                }
                                else -> {
                                    detector.move(Point(change.position.x, change.position.y), swipeBlocked())
                                    if (detector.ownsGesture) {
                                        change.consume()
                                        claimed = change.id
                                    }
                                }
                            }
                            val finalEvent = awaitPointerEvent(PointerEventPass.Final)
                            if (finalEvent.changes.any { it.isConsumed && it.id != claimed }) {
                                detector.cancel()
                                active = null
                                draining = finalEvent.changes.any { it.pressed }
                            }
                            detector.advanceTime(SystemClock.uptimeMillis(), swipeBlocked())
                        }
                    }
                } finally {
                    detector.cancel()
                }
            },
    )
}

internal enum class DoubleTapZone { BACKWARD, TOGGLE_PLAY, FORWARD }

internal val flashScrim = Color(0x99000000)
