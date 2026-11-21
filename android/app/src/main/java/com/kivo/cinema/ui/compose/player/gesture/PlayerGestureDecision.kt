package com.kivo.cinema.ui.compose.player.gesture

import kotlin.math.abs

internal class PlayerGestureDecision(density: Float = 1f) {
    enum class Axis { HORIZONTAL, VERTICAL }
    enum class Region { BACKWARD, TOGGLE_PLAY, FORWARD }
    data class Point(val x: Float, val y: Float)

    private val movementSlop = 12f * density
    private val directionSlop = 15f * density
    private val doubleTapSlop = 32f * density

    init {
        require(density.isFinite() && density > 0f)
    }

    fun moved(anchor: Point, point: Point): Boolean =
        distanceSquared(anchor, point) >= movementSlop * movementSlop

    fun direction(anchor: Point, point: Point): Axis? {
        if (!moved(anchor, point)) return null
        val x = abs(point.x - anchor.x)
        val y = abs(point.y - anchor.y)
        return when {
            x >= y * 1.2f -> Axis.HORIZONTAL
            y >= x * 1.2f -> Axis.VERTICAL
            distanceSquared(anchor, point) < directionSlop * directionSlop -> null
            x >= y -> Axis.HORIZONTAL
            else -> Axis.VERTICAL
        }
    }

    fun region(x: Float, width: Float): Region = when {
        x < width.coerceAtLeast(1f) * 0.34f -> Region.BACKWARD
        x > width.coerceAtLeast(1f) * 0.66f -> Region.FORWARD
        else -> Region.TOGGLE_PLAY
    }

    fun canDoubleTap(first: Point, second: Point, firstRegion: Region, secondRegion: Region): Boolean =
        firstRegion == secondRegion && distanceSquared(first, second) <= doubleTapSlop * doubleTapSlop

    private fun distanceSquared(first: Point, second: Point): Float {
        val dx = second.x - first.x
        val dy = second.y - first.y
        return dx * dx + dy * dy
    }
}
