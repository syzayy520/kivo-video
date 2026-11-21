package com.kivo.cinema.ui.compose.player.gesture

import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDecision.Point
import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDecision.Region
import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDetector.Action
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class PlayerGestureDetectorTest {
    private val events = mutableListOf<Action>()
    private val detector = PlayerGestureDetector(PlayerGestureDecision()) { events += it }
    private val origin = Point(100f, 100f)

    @Test
    fun `detector integrates delayed single and suspended double`() {
        firstTap()
        detector.advanceTime(249)
        assertTrue(events.isEmpty())
        detector.down(250, origin, 1000f)
        detector.advanceTime(500)
        assertTrue(events.isEmpty())
        detector.up(510, origin)
        detector.advanceTime(1000)
        assertEquals(listOf(Action.DoubleTap(Region.BACKWARD)), events)
    }

    @Test
    fun `cross boundary taps are two delayed singles`() {
        detector.down(0, Point(339f, 100f), 1000f)
        detector.up(10, Point(339f, 100f))
        detector.down(20, Point(341f, 100f), 1000f)
        detector.up(30, Point(341f, 100f))
        detector.advanceTime(250)
        assertEquals(listOf(Action.Tap), events)
        detector.advanceTime(270)
        assertEquals(listOf(Action.Tap, Action.Tap), events)
    }

    @Test
    fun `horizontal lock never becomes vertical or long press`() {
        detector.down(0, origin, 1000f)
        detector.move(Point(112f, 100f))
        detector.move(Point(110f, 160f))
        detector.advanceTime(1000)
        detector.up(1100, Point(110f, 160f))
        assertEquals(listOf(
            Action.HorizontalStart, Action.HorizontalDrag(12f), Action.HorizontalDrag(-2f),
            Action.HorizontalEnd(true),
        ), events)
    }

    @Test
    fun `vertical side uses down anchor even after crossing midpoint`() {
        val down = Point(499f, 100f)
        detector.down(0, down, 1000f)
        detector.move(Point(501f, 113f))
        detector.move(Point(550f, 111f))
        detector.up(100, Point(550f, 111f))
        assertEquals(listOf(
            Action.VerticalStart(499f), Action.VerticalDrag(13f), Action.VerticalDrag(-2f), Action.VerticalEnd,
        ), events)
    }

    @Test
    fun `ambiguous displacement irreversibly cancels long press and tap`() {
        detector.down(0, origin, 1000f)
        detector.move(Point(109f, 109f))
        assertFalse(detector.ownsGesture)
        assertFalse(detector.acceptsTap)
        assertNull(detector.nextDeadlineMillis)
        detector.move(origin)
        detector.advanceTime(1000)
        detector.up(1100, origin)
        detector.advanceTime(2000)
        assertTrue(events.isEmpty())
    }

    @Test
    fun `ambiguous movement can later choose axis using original anchor`() {
        detector.down(0, origin, 1000f)
        detector.move(Point(109f, 109f))
        detector.move(Point(111f, 111f))
        detector.up(100, Point(111f, 111f))
        assertEquals(listOf(Action.HorizontalStart, Action.HorizontalDrag(11f), Action.HorizontalEnd(true)), events)
    }

    @Test
    fun `long press at 380ms stays exclusive and uses absolute drag positions`() {
        detector.down(0, origin, 1000f)
        detector.move(Point(103f, 104f))
        detector.advanceTime(379)
        assertTrue(events.isEmpty())
        detector.advanceTime(380)
        detector.move(Point(150f, 200f), swipeBlocked = true)
        detector.up(500, Point(150f, 200f), swipeBlocked = true)
        assertEquals(listOf(
            Action.LongPressStart(origin), Action.LongPressDrag(Point(150f, 200f)), Action.LongPressEnd(false),
        ), events)
    }

    @Test
    fun `second tap turning into drag cancels entire pending sequence`() {
        firstTap()
        detector.down(200, origin, 1000f)
        detector.move(Point(112f, 100f))
        detector.up(230, Point(112f, 100f))
        detector.advanceTime(1000)
        assertEquals(listOf(Action.HorizontalStart, Action.HorizontalDrag(12f), Action.HorizontalEnd(true)), events)
    }

    @Test
    fun `second ambiguous movement cancels pending tap without starting a drag`() {
        firstTap()
        detector.down(200, origin, 1000f)
        detector.move(Point(109f, 109f))
        detector.move(origin)
        detector.advanceTime(1000)
        detector.up(1100, origin)
        assertTrue(events.isEmpty())
    }

    @Test
    fun `second tap turning into long press never emits first tap`() {
        firstTap()
        detector.down(200, origin, 1000f)
        detector.advanceTime(580)
        detector.up(600, origin)
        detector.advanceTime(1000)
        assertEquals(listOf(Action.LongPressStart(origin), Action.LongPressEnd(false)), events)
    }

    @Test
    fun `disabled direction cannot revive or fall back to tap`() {
        detector.down(0, origin, 1000f)
        detector.move(Point(112f, 100f), swipeBlocked = true)
        detector.move(Point(140f, 100f), swipeBlocked = false)
        assertFalse(detector.up(100, Point(140f, 100f)))
        detector.advanceTime(1000)
        assertTrue(events.isEmpty())
    }

    @Test
    fun `swipe block preserves taps and leaves long press permission to router`() {
        detector.down(0, origin, 1000f)
        assertTrue(detector.up(10, origin, swipeBlocked = true))
        detector.advanceTime(250, swipeBlocked = true)
        detector.down(300, origin, 1000f)
        detector.advanceTime(680, swipeBlocked = true)
        detector.up(700, origin, swipeBlocked = true)
        assertEquals(listOf(Action.Tap, Action.LongPressStart(origin), Action.LongPressEnd(false)), events)
    }

    @Test
    fun `disabling an active swipe always cleans up without commit`() {
        detector.down(0, origin, 1000f)
        detector.move(Point(112f, 100f))
        detector.advanceTime(100, swipeBlocked = true)
        detector.up(200, Point(112f, 100f), swipeBlocked = true)
        detector.advanceTime(1000)
        assertEquals(listOf(Action.HorizontalStart, Action.HorizontalDrag(12f), Action.HorizontalEnd(false)), events)
    }

    @Test
    fun `interruption cancels active horizontal once and never emits tap`() {
        detector.down(0, origin, 1000f)
        detector.move(Point(112f, 100f))
        detector.cancel()
        detector.cancel()
        detector.up(100, Point(112f, 100f))
        detector.advanceTime(1000)
        assertEquals(listOf(Action.HorizontalStart, Action.HorizontalDrag(12f), Action.HorizontalEnd(false)), events)
    }

    @Test
    fun `interruption ends vertical once`() {
        detector.down(0, origin, 1000f)
        detector.move(Point(100f, 112f))
        detector.cancel()
        detector.cancel()
        detector.up(100, Point(100f, 112f))
        assertEquals(listOf(Action.VerticalStart(100f), Action.VerticalDrag(12f), Action.VerticalEnd), events)
    }

    @Test
    fun `interruption uses distinct long press cancel exactly once`() {
        detector.down(0, origin, 1000f)
        detector.advanceTime(380)
        detector.cancel()
        detector.cancel()
        detector.up(500, origin)
        assertEquals(listOf(Action.LongPressStart(origin), Action.LongPressEnd(true)), events)
    }

    @Test
    fun `interruption clears released and suspended taps but allows next gesture`() {
        firstTap()
        detector.cancel()
        detector.advanceTime(1000)
        assertTrue(events.isEmpty())
        detector.down(1100, origin, 1000f)
        detector.up(1110, origin)
        detector.down(1200, origin, 1000f)
        detector.cancel()
        detector.up(1210, origin)
        detector.advanceTime(2000)
        assertTrue(events.isEmpty())
        detector.down(2100, origin, 1000f)
        detector.up(2110, origin)
        detector.advanceTime(2350)
        assertEquals(listOf(Action.Tap), events)
    }

    @Test
    fun `release movement is checked before treating it as tap`() {
        detector.down(0, origin, 1000f)
        assertTrue(detector.up(100, Point(112f, 100f)))
        detector.advanceTime(1000)
        assertEquals(listOf(Action.HorizontalStart, Action.HorizontalDrag(12f), Action.HorizontalEnd(true)), events)
    }

    private fun firstTap() {
        detector.down(0, origin, 1000f)
        detector.up(10, origin)
    }
}
