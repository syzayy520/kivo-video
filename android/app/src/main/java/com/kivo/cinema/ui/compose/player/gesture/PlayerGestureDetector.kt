package com.kivo.cinema.ui.compose.player.gesture

import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDecision.Axis
import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDecision.Point
import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDecision.Region

internal class PlayerGestureDetector(
    private val decision: PlayerGestureDecision,
    private val emit: (Action) -> Unit,
) {
    sealed interface Action {
        data object Tap : Action
        data class DoubleTap(val region: Region) : Action
        data object HorizontalStart : Action
        data class HorizontalDrag(val delta: Float) : Action
        data class HorizontalEnd(val commit: Boolean) : Action
        data class VerticalStart(val x: Float) : Action
        data class VerticalDrag(val delta: Float) : Action
        data object VerticalEnd : Action
        data class LongPressStart(val point: Point) : Action
        data class LongPressDrag(val point: Point) : Action
        data class LongPressEnd(val cancelled: Boolean) : Action
    }

    private enum class State { IDLE, PENDING, HORIZONTAL, VERTICAL, LONG_PRESS, SUPPRESSED, CANCELLED }

    private val taps = PlayerTapSequence(decision, { emit(Action.Tap) }, { emit(Action.DoubleTap(it)) })
    private var state = State.IDLE
    private var anchor = Point(0f, 0f)
    private var previous = anchor
    private var longPressAt: Long? = null
    private var displaced = false

    val ownsGesture: Boolean
        get() = state == State.HORIZONTAL || state == State.VERTICAL || state == State.LONG_PRESS

    val acceptsTap: Boolean
        get() = state == State.PENDING && !displaced

    val nextDeadlineMillis: Long?
        get() = listOfNotNull(longPressAt, taps.nextDeadlineMillis).minOrNull()

    fun down(now: Long, point: Point, width: Float) {
        if (state != State.IDLE) cancel()
        state = State.PENDING
        anchor = point
        previous = point
        displaced = false
        longPressAt = now + LONG_PRESS_MS
        taps.down(now, point, decision.region(point.x, width))
    }

    fun move(point: Point, swipeBlocked: Boolean = false) {
        if (swipeBlocked && (state == State.HORIZONTAL || state == State.VERTICAL)) {
            cancel()
            return
        }
        val last = previous
        previous = point
        when (state) {
            State.PENDING -> {
                if (decision.moved(anchor, point)) {
                    displaced = true
                    longPressAt = null
                    taps.cancel()
                }
                val axis = decision.direction(anchor, point) ?: return
                if (swipeBlocked) {
                    state = State.SUPPRESSED
                } else if (axis == Axis.HORIZONTAL) {
                    state = State.HORIZONTAL
                    emit(Action.HorizontalStart)
                    emit(Action.HorizontalDrag(point.x - anchor.x))
                } else {
                    state = State.VERTICAL
                    emit(Action.VerticalStart(anchor.x))
                    emit(Action.VerticalDrag(point.y - anchor.y))
                }
            }
            State.HORIZONTAL -> if (point.x != last.x) emit(Action.HorizontalDrag(point.x - last.x))
            State.VERTICAL -> if (point.y != last.y) emit(Action.VerticalDrag(point.y - last.y))
            State.LONG_PRESS -> if (point != last) emit(Action.LongPressDrag(point))
            else -> Unit
        }
    }

    fun up(now: Long, point: Point, swipeBlocked: Boolean = false): Boolean {
        move(point, swipeBlocked)
        advanceTime(now, swipeBlocked)
        val consumed = ownsGesture || acceptsTap
        val ended = state
        state = State.IDLE
        longPressAt = null
        when (ended) {
            State.PENDING -> if (!displaced) taps.up(now)
            State.HORIZONTAL -> emit(Action.HorizontalEnd(commit = true))
            State.VERTICAL -> emit(Action.VerticalEnd)
            State.LONG_PRESS -> emit(Action.LongPressEnd(cancelled = false))
            else -> Unit
        }
        return consumed
    }

    fun advanceTime(now: Long, swipeBlocked: Boolean = false) {
        if (swipeBlocked && (state == State.HORIZONTAL || state == State.VERTICAL)) cancel()
        taps.advanceTime(now)
        val deadline = longPressAt ?: return
        if (state == State.PENDING && now >= deadline) {
            state = State.LONG_PRESS
            longPressAt = null
            taps.cancel()
            emit(Action.LongPressStart(anchor))
        }
    }

    fun cancel() {
        val cancelled = state
        state = if (state == State.IDLE) State.IDLE else State.CANCELLED
        longPressAt = null
        taps.cancel()
        when (cancelled) {
            State.HORIZONTAL -> emit(Action.HorizontalEnd(commit = false))
            State.VERTICAL -> emit(Action.VerticalEnd)
            State.LONG_PRESS -> emit(Action.LongPressEnd(cancelled = true))
            else -> Unit
        }
    }

    companion object {
        const val LONG_PRESS_MS = 380L
    }
}
