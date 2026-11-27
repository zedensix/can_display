package com.zedensix.gt86dash

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

// The frames pico/test/test_live.c pins on the dash's side: the same bytes.
class LiveFrameTest {
    private fun bytes(vararg b: Int) = ByteArray(b.size) { b[it].toByte() }

    @Test
    fun decodesAFrame() {
        val r = LiveFrame.decode(
            bytes(0x01, 0x15, 0x04, 0x8E, 0x03, 0x1F, 0x83, 0x8E, 0x44, 0xF1, 0x14, 0x03, 0x0E, 0x7B, 0x00, 0x00, 0x13),
        )!!
        assertEquals(104.5f, r.oil!!, 0.01f)
        assertEquals(91f, r.coolant!!, 0.01f)
        assertEquals(31f, r.intake!!, 0.01f)
        assertEquals(13.1f, r.afr!!, 0.01f)
        assertEquals(14.2f, r.battery!!, 0.01f)
        assertEquals(68f, r.ethanol!!, 0.01f)
        assertEquals(5361f, r.rpm!!, 0.01f)
        assertEquals(3, r.gear)
        assertEquals(14f, r.outside!!, 0.01f)
        assertEquals(12.3f, r.km!!, 0.01f)
        assertEquals(true, r.trip)
        assertEquals(DriveMode.TRACK, r.mode)
        assertEquals(false, r.offThrottle)
        assertEquals(false, r.none)
    }

    @Test
    fun decodesNothingFresh() {
        val r = LiveFrame.decode(
            bytes(0x01, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x80, 0x7B, 0x00, 0x00, 0x10),
        )!!
        listOf(r.oil, r.coolant, r.intake, r.afr, r.battery, r.ethanol, r.rpm, r.outside).forEach { assertNull(it) }
        assertEquals(true, r.none)
        assertEquals(DriveMode.COMFORT, r.mode)
    }

    @Test
    fun refusesAnotherVersion() {
        assertNull(LiveFrame.decode(ByteArray(LiveFrame.SIZE) { 2 }))
        assertNull(LiveFrame.decode(bytes(0x01, 0x00)))
    }
}
