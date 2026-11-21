package com.kivo.cinema.ui.compose.player.gesture

import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDecision.Axis
import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDecision.Point
import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDecision.Region
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class PlayerGestureDecisionTest {
    private val decision = PlayerGestureDecision()
    private val origin = Point(100f, 100f)

    @Test
    fun `movement threshold is radial and includes twelve dp`() {
        assertFalse(decision.moved(origin, Point(111.99f, 100f)))
        assertTrue(decision.moved(origin, Point(112f, 100f)))
        assertTrue(decision.moved(origin, Point(109f, 109f)))
        assertNull(decision.direction(origin, Point(108f, 108f)))
    }

    @Test
    fun `clear axes lock from twelve dp in either sign`() {
        assertEquals(Axis.HORIZONTAL, decision.direction(origin, Point(112f, 100f)))
        assertEquals(Axis.HORIZONTAL, decision.direction(origin, Point(90f, 92f)))
        assertEquals(Axis.VERTICAL, decision.direction(origin, Point(108f, 110f)))
        assertEquals(Axis.VERTICAL, decision.direction(origin, Point(100f, 88f)))
    }

    @Test
    fun `ambiguous diagonal waits until fifteen dp then chooses dominant axis`() {
        assertNull(decision.direction(origin, Point(109f, 109f)))
        assertEquals(Axis.HORIZONTAL, decision.direction(origin, Point(111f, 111f)))
        assertEquals(Axis.VERTICAL, decision.direction(origin, Point(110f, 112f)))
        assertEquals(Axis.HORIZONTAL, decision.direction(origin, Point(112f, 110f)))
    }

    @Test
    fun `all distance thresholds scale with Android density`() {
        val scaled = PlayerGestureDecision(3f)
        assertFalse(scaled.moved(origin, Point(135f, 100f)))
        assertTrue(scaled.moved(origin, Point(136f, 100f)))
        assertNull(scaled.direction(origin, Point(127f, 127f)))
        assertEquals(Axis.HORIZONTAL, scaled.direction(origin, Point(133f, 133f)))
        assertTrue(scaled.canDoubleTap(origin, Point(196f, 100f), Region.BACKWARD, Region.BACKWARD))
        assertFalse(scaled.canDoubleTap(origin, Point(196.1f, 100f), Region.BACKWARD, Region.BACKWARD))
    }

    @Test
    fun `nearby taps across a region boundary are never doubles`() {
        assertEquals(Region.BACKWARD, decision.region(33.99f, 100f))
        assertEquals(Region.TOGGLE_PLAY, decision.region(34f, 100f))
        assertEquals(Region.TOGGLE_PLAY, decision.region(66f, 100f))
        assertEquals(Region.FORWARD, decision.region(66.01f, 100f))
        assertFalse(decision.canDoubleTap(Point(33f, 10f), Point(35f, 10f), Region.BACKWARD, Region.TOGGLE_PLAY))
    }

    @Test
    fun `double tap uses two dimensional distance including thirty two dp`() {
        assertTrue(decision.canDoubleTap(origin, Point(132f, 100f), Region.FORWARD, Region.FORWARD))
        assertFalse(decision.canDoubleTap(origin, Point(132.1f, 100f), Region.FORWARD, Region.FORWARD))
        assertFalse(decision.canDoubleTap(origin, Point(125f, 125f), Region.FORWARD, Region.FORWARD))
    }
}
