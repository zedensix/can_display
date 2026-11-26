package com.zedensix.gt86dash

import android.content.Context
import java.io.FileNotFoundException
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.zip.CRC32

// The dash's firmware the app carries (the asset firmware.bin, the Pico
// build's raw binary, bundled by mise run android:build): its bytes, their
// CRC-32 and size, what the dash's update characteristic reports of its own.
class FirmwareImage(val bytes: ByteArray) {
    val crc: Long = CRC32().apply { update(bytes) }.value
    val size: Int get() = bytes.size

    fun runsOn(dash: DashFirmware) = dash.crc == crc && dash.size == size.toLong()

    companion object {
        private var loaded = false
        private var image: FirmwareImage? = null

        // Null in a build without it.
        @Synchronized
        fun bundled(c: Context): FirmwareImage? {
            if (!loaded) {
                loaded = true
                image = try {
                    c.assets.open("firmware.bin").use { FirmwareImage(it.readBytes()) }
                } catch (_: FileNotFoundException) {
                    null
                }
            }
            return image
        }
    }
}

// What the dash runs, read off its update characteristic: the CRC-32 of
// its firmware and its size, little-endian.
class DashFirmware(val crc: Long, val size: Long) {
    companion object {
        fun decode(b: ByteArray): DashFirmware? {
            if (b.size < 8) return null
            val buf = ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN)
            return DashFirmware(buf.getInt(0).toLong() and 0xFFFFFFFFL, buf.getInt(4).toLong() and 0xFFFFFFFFL)
        }
    }
}

// The update's commands, written to the control characteristic: 'U' asks
// for it (the driver clicks the knob to allow it), 'W' carries the image
// from an offset, 'X' cancels it.
object UpdateCommand {
    val CANCEL = byteArrayOf('X'.code.toByte())

    fun request(size: Int, crc: Long): ByteArray =
        ByteBuffer.allocate(9).order(ByteOrder.LITTLE_ENDIAN).put('U'.code.toByte()).putInt(size).putInt(crc.toInt()).array()

    fun write(offset: Int, image: ByteArray, length: Int): ByteArray =
        ByteBuffer.allocate(5 + length).order(ByteOrder.LITTLE_ENDIAN).put('W'.code.toByte()).putInt(offset)
            .put(image, offset, length).array()
}

// The update characteristic's notifications: the dash has this much of
// the image in flash and wants what follows ('A'), it checked it and
// installs it ('I'), it gave up ('E' and why).
sealed class UpdateNotice {
    class Accepted(val have: Int) : UpdateNotice()

    object Installing : UpdateNotice()

    class Failed(val code: Int) : UpdateNotice()

    companion object {
        fun decode(b: ByteArray): UpdateNotice? {
            if (b.isEmpty()) return null
            return when (b[0].toInt().toChar()) {
                'A' -> if (b.size < 5) null else Accepted(ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN).getInt(1))
                'I' -> Installing
                'E' -> Failed(if (b.size < 2) 0 else b[1].toInt() and 0xFF)
                else -> null
            }
        }
    }
}

// The image's way to the dash: writes of up to payload bytes from where
// it is, never past a sector beyond what the dash said it has (it writes
// a sector to flash, then asks for the next). Every 'A' puts it back
// where the dash wants it, a write lost or out of order sent again.
class Transfer(private val image: ByteArray, private val payload: Int) {
    var position = 0
        private set

    // What the dash last said it has.
    var have = 0
        private set

    val done: Boolean get() = have >= image.size

    fun accepted(have: Int) {
        this.have = have.coerceIn(0, image.size)
        position = this.have
    }

    // The next write, null while the sector is all sent or the image is.
    fun next(): ByteArray? {
        val end = minOf(image.size, have + WINDOW, position + payload)
        if (end <= position) return null
        val command = UpdateCommand.write(position, image, end - position)
        position = end
        return command
    }

    // A write Android didn't take: from there again.
    fun rewind(to: Int) {
        position = to
    }

    companion object {
        // One flash sector.
        const val WINDOW = 4096
    }
}
