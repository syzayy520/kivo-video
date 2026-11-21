package com.kivo.cinema.ui.compose.player.gesture

import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDecision.Point
import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDecision.Region

internal class PlayerTapSequence(
    private val decision: PlayerGestureDecision,
    private val onTap: () -> Unit,
    private val onDoubleTap: (Region) -> Unit,
) {
    private class Tap(val point: Point, val region: Region, val upAt: Long)
    private class Press(val point: Point, val region: Region, val secondOf: Tap?)

    private val pending = mutableListOf<Tap>()
    private var press: Press? = null

    val nextDeadlineMillis: Long?
        get() = pending.firstOrNull { it !== press?.secondOf }?.let { it.upAt + WINDOW_MS }

    fun down(now: Long, point: Point, region: Region) {
        val candidate = pending.lastOrNull()?.takeIf {
            now - it.upAt in 0..WINDOW_MS && decision.canDoubleTap(it.point, point, it.region, region)
        }
        press = Press(point, region, candidate)
        advanceTime(now)
    }

    fun up(now: Long) {
        val released = press ?: return
        press = null
        val first = released.secondOf
        if (first != null) {
            pending.remove(first)
            onDoubleTap(released.region)
        } else {
            pending += Tap(released.point, released.region, now)
        }
        advanceTime(now)
    }

    fun advanceTime(now: Long) {
        val ready = pending.filter { it !== press?.secondOf && now >= it.upAt + WINDOW_MS }
        pending.removeAll(ready.toSet())
        ready.forEach { onTap() }
    }

    fun cancel() {
        press = null
        pending.clear()
    }

    companion object {
        const val WINDOW_MS = 240L
    }
}
