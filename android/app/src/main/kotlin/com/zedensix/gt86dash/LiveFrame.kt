package com.zedensix.gt86dash

// The live readings off the dash: a frame (pico/src/app/data/live.h) decoded
// into Readings. The layout must follow live_encode's.
object LiveFrame {
    const val VERSION = 1
    const val SIZE = 17

    fun decode(b: ByteArray): Readings? {
        if (b.size < SIZE || b[0].toInt() != VERSION) return null
        fun u8(i: Int) = b[i].toInt() and 0xFF
        fun u16(i: Int) = u8(i) or (u8(i + 1) shl 8)
        fun i16(i: Int) = u16(i).toShort().toInt()
        val flags = u8(16)
        return Readings(
            oil = i16(1).takeIf { it != Short.MIN_VALUE.toInt() }?.let { it / 10f },
            coolant = i16(3).takeIf { it != Short.MIN_VALUE.toInt() }?.let { it / 10f },
            intake = b[5].toInt().takeIf { it != Byte.MIN_VALUE.toInt() }?.toFloat(),
            afr = u8(6).takeIf { it != 0 }?.let { it / 10f },
            battery = u8(7).takeIf { it != 0 }?.let { it / 10f },
            ethanol = u8(8).takeIf { it != 0xFF }?.toFloat(),
            rpm = u16(9).takeIf { it != 0xFFFF }?.toFloat(),
            gear = u8(11),
            outside = b[12].toInt().takeIf { it != Byte.MIN_VALUE.toInt() }?.toFloat(),
            km = (u8(13) or (u8(14) shl 8) or (u8(15) shl 16)) / 10f,
            trip = flags and 0x10 != 0,
            mode = DriveMode.entries.getOrElse(flags and 0x07) { DriveMode.COMFORT },
            offThrottle = flags and 0x08 != 0,
        )
    }
}
