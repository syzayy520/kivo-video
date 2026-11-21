package com.kivo.cinema.ui.compose.player.gesture

import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDecision.Point
import com.kivo.cinema.ui.compose.player.gesture.PlayerGestureDecision.Region
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class PlayerTapSequenceTest {
    private val events = mutableListOf<String>()
    private val taps = PlayerTapSequence(PlayerGestureDecision(), { events += "single" }, { events += "double:$it" })
    private val point = Point(100f, 100f)

    @Test
    fun `single is delayed from first up not down`() {
        taps.down(0, point, Region.BACKWARD)
        taps.up(100)
        assertEquals(340L, taps.nextDeadlineMillis)
        taps.advanceTime(339)
        assertTrue(events.isEmpty())
        taps.advanceTime(340)
        taps.advanceTime(1000)
        assertEquals(listOf("single"), events)
        assertNull(taps.nextDeadlineMillis)
    }

    @Test
    fun `qualified second down suspends first through its old deadline`() {
        firstTap()
        taps.down(200, Point(132f, 100f), Region.BACKWARD)
        assertNull(taps.nextDeadlineMillis)
        taps.advanceTime(400)
        assertTrue(events.isEmpty())
        taps.up(450)
        taps.advanceTime(1000)
        assertEquals(listOf("double:BACKWARD"), events)
    }

    @Test
    fun `second down at window boundary still qualifies`() {
        firstTap()
        taps.down(250, point, Region.BACKWARD)
        taps.up(260)
        assertEquals(listOf("double:BACKWARD"), events)
    }

    @Test
    fun `late second down produces separate singles`() {
        firstTap()
        taps.down(251, point, Region.BACKWARD)
        taps.up(260)
        assertEquals(listOf("single"), events)
        taps.advanceTime(500)
        assertEquals(listOf("single", "single"), events)
    }

    @Test
    fun `different region does not suspend first or combine taps`() {
        firstTap()
        taps.down(200, Point(101f, 100f), Region.TOGGLE_PLAY)
        assertEquals(250L, taps.nextDeadlineMillis)
        taps.advanceTime(250)
        assertEquals(listOf("single"), events)
        taps.up(300)
        taps.advanceTime(539)
        assertEquals(listOf("single"), events)
        taps.advanceTime(540)
        assertEquals(listOf("single", "single"), events)
    }

    @Test
    fun `same region beyond spatial window does not suspend first`() {
        firstTap()
        taps.down(200, Point(132.01f, 100f), Region.BACKWARD)
        taps.advanceTime(250)
        taps.up(300)
        taps.advanceTime(540)
        assertEquals(listOf("single", "single"), events)
    }

    @Test
    fun `rapid unrelated taps keep their individual deadlines`() {
        firstTap()
        taps.down(20, point, Region.TOGGLE_PLAY)
        taps.up(30)
        taps.down(40, point, Region.FORWARD)
        taps.up(50)
        taps.advanceTime(250)
        assertEquals(listOf("single"), events)
        taps.advanceTime(270)
        assertEquals(listOf("single", "single"), events)
        taps.advanceTime(290)
        assertEquals(listOf("single", "single", "single"), events)
    }

    @Test
    fun `second candidate only suspends its matching pending tap`() {
        firstTap()
        taps.down(20, point, Region.FORWARD)
        taps.up(30)
        taps.down(40, point, Region.FORWARD)
        taps.advanceTime(300)
        assertEquals(listOf("single"), events)
        taps.up(310)
        assertEquals(listOf("single", "double:FORWARD"), events)
    }

    @Test
    fun `cancel discards both suspended taps and all deadlines`() {
        firstTap()
        taps.down(200, point, Region.BACKWARD)
        taps.cancel()
        taps.up(220)
        taps.advanceTime(1000)
        assertTrue(events.isEmpty())
        assertNull(taps.nextDeadlineMillis)
    }

    @Test
    fun `same side repeated doubles emit distinct events`() {
        firstTap()
        taps.down(100, point, Region.BACKWARD)
        taps.up(110)
        taps.down(120, point, Region.BACKWARD)
        taps.up(130)
        taps.down(140, point, Region.BACKWARD)
        taps.up(150)
        taps.advanceTime(1000)
        assertEquals(listOf("double:BACKWARD", "double:BACKWARD"), events)
    }

    private fun firstTap() {
        taps.down(0, point, Region.BACKWARD)
        taps.up(10)
    }
}
