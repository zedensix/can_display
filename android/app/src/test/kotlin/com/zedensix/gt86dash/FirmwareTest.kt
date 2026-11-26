package com.zedensix.gt86dash

import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import java.nio.ByteBuffer
import java.nio.ByteOrder

// The firmware update's bytes as the dash's ble.c takes and sends them,
// and the image's flow control.
class FirmwareTest {
    private fun bytes(vararg b: Int) = ByteArray(b.size) { b[it].toByte() }

    private fun offset(write: ByteArray) = ByteBuffer.wrap(write).order(ByteOrder.LITTLE_ENDIAN).getInt(1)

    @Test
    fun encodesTheCommands() {
        assertArrayEquals(
            bytes(0x55, 0x00, 0x40, 0x09, 0x00, 0xEF, 0xBE, 0xAD, 0xDE),
            UpdateCommand.request(0x094000, 0xDEADBEEFL),
        )
        val image = ByteArray(16) { it.toByte() }
        assertArrayEquals(bytes(0x57, 0x0A, 0x00, 0x00, 0x00, 0x0A, 0x0B, 0x0C), UpdateCommand.write(10, image, 3))
        assertArrayEquals(bytes(0x58), UpdateCommand.CANCEL)
    }

    @Test
    fun decodesTheNotifications() {
        assertEquals(0x12345, (UpdateNotice.decode(bytes(0x41, 0x45, 0x23, 0x01, 0x00)) as UpdateNotice.Accepted).have)
        assertEquals(UpdateNotice.Installing, UpdateNotice.decode(bytes(0x49)))
        assertEquals(3, (UpdateNotice.decode(bytes(0x45, 0x03)) as UpdateNotice.Failed).code)
        assertNull(UpdateNotice.decode(bytes(0x41, 0x00)))
        assertNull(UpdateNotice.decode(bytes(0x5A)))
        assertNull(UpdateNotice.decode(bytes()))
    }

    @Test
    fun comparesWithTheDashsFirmware() {
        val image = FirmwareImage("123456789".toByteArray())
        assertEquals(0xCBF43926L, image.crc) // CRC-32's check value
        val dash = DashFirmware.decode(bytes(0x26, 0x39, 0xF4, 0xCB, 0x09, 0x00, 0x00, 0x00))!!
        assertEquals(0xCBF43926L, dash.crc)
        assertTrue(image.runsOn(dash))
        assertFalse(image.runsOn(DashFirmware(0xCBF43926L, 10)))
        assertNull(DashFirmware.decode(bytes(0x01, 0x02)))
    }

    @Test
    fun sendsASectorAtATime() {
        val image = ByteArray(10_000)
        val t = Transfer(image, 239)
        t.accepted(0)
        var sent = 0
        while (true) {
            val w = t.next() ?: break
            assertEquals(sent, offset(w))
            sent += w.size - 5
        }
        assertEquals(Transfer.WINDOW, sent)
        t.accepted(Transfer.WINDOW)
        assertEquals(Transfer.WINDOW, offset(t.next()!!))
    }

    @Test
    fun anAcceptRewinds() {
        val t = Transfer(ByteArray(10_000), 239)
        t.accepted(0)
        repeat(5) { t.next() }
        t.accepted(239)
        assertEquals(239, offset(t.next()!!))
        assertEquals(239 * 2, t.position)
    }

    @Test
    fun aRefusedWriteIsSentAgain() {
        val t = Transfer(ByteArray(1000), 239)
        t.accepted(0)
        t.next()
        val from = t.position
        t.next()
        t.rewind(from)
        assertEquals(from, offset(t.next()!!))
    }

    @Test
    fun doneOnceTheDashHasItAll() {
        val t = Transfer(ByteArray(300), 239)
        t.accepted(0)
        assertEquals(239, t.next()!!.size - 5)
        assertEquals(61, t.next()!!.size - 5)
        assertNull(t.next())
        assertFalse(t.done)
        t.accepted(300)
        assertTrue(t.done)
        assertNull(t.next())
    }
}
