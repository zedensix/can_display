package com.zedensix.gt86dash

import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.time.Instant
import java.time.ZoneId
import java.time.format.DateTimeFormatter
import java.util.Locale

// A record of the dash's data log: its session (a boot), the milliseconds
// since that boot, then a reading per column, null where it had none.
class Sample(val session: Int, val ms: Long, val values: List<Float?>)

// The dash's data log (pico/src/app/store/datalog.h), read as
// web/datalog.ts reads it: this must follow its layout. 64-byte records
// (kind 2); the 32-byte ones an older firmware wrote (kind 1) and erased
// flash are skipped, a 32-byte slot at a time.
object Records {
    const val SLOT = 32
    const val RECORD = 64
    private const val SAMPLE = 2
    private const val NONE16 = -32768

    // Each column in the record's order: its CSV name and its scale.
    val COLUMNS = listOf(
        "rpm" to 1f, "coolant_c" to 10f, "oil_c" to 10f, "intake_c" to 10f, "afr" to 100f,
        "battery_v" to 100f, "ethanol_pct" to 10f, "throttle_pct" to 10f, "pedal_pct" to 10f,
        "load_pct" to 10f, "timing_deg" to 10f, "stft_pct" to 10f, "ltft_pct" to 10f,
        "afr_target" to 100f, "rail_mpa" to 100f, "speed_kmh" to 1f, "outside_c" to 10f,
    )

    private fun buffer(b: ByteArray) = ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN)

    // The 64-byte records in bytes, with each one's raw bytes, in order.
    fun walk(bytes: ByteArray): List<Pair<Sample, ByteArray>> {
        val b = buffer(bytes)
        val out = mutableListOf<Pair<Sample, ByteArray>>()
        var at = 0
        while (at + SLOT <= bytes.size) {
            val session = b.getShort(at).toInt() and 0xFFFF
            val whole = session != 0xFFFF && bytes[at + 2].toInt() == SAMPLE && at + RECORD <= bytes.size
            if (whole) {
                val values = COLUMNS.mapIndexed { i, (_, scale) ->
                    b.getShort(at + 8 + 2 * i).toInt().takeIf { it != NONE16 }?.let { it / scale }
                }
                val ms = b.getInt(at + 4).toLong() and 0xFFFFFFFFL
                out += Sample(session, ms, values) to bytes.copyOfRange(at, at + RECORD)
            }
            at += if (whole) RECORD else SLOT
        }
        return out
    }

    fun parse(bytes: ByteArray): List<Sample> = walk(bytes).map { it.first }

    // The samples as a CSV file's text: the time of day when the boot's is
    // known (bootAt, the phone's clock), the seconds since the boot, then
    // the readings; a dot for the decimals whatever the phone's language.
    fun csv(samples: List<Sample>, bootAt: Long?): String {
        val clock = DateTimeFormatter.ISO_LOCAL_DATE_TIME.withZone(ZoneId.systemDefault())
        val out = StringBuilder("time,session,time_s,")
        out.append(COLUMNS.joinToString(",") { it.first }).append('\n')
        for (s in samples) {
            if (bootAt != null) out.append(clock.format(Instant.ofEpochMilli(bootAt + s.ms)))
            out.append(',').append(s.session).append(',').append(String.format(Locale.ROOT, "%.1f", s.ms / 1000.0))
            s.values.forEach { v -> out.append(',').append(v?.let { String.format(Locale.ROOT, "%.2f", it).trimEnd('0').trimEnd('.') } ?: "") }
            out.append('\n')
        }
        return out.toString()
    }
}
