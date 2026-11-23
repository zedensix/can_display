package com.zedensix.gt86dash

// The gauges, their bands and colors: a copy of the firmware's *_RANGE
// macros and ui_tiles (src/app/ui/ui.c), which must stay in step.

object Colors {
    const val BACKGROUND = 0xFF000000.toInt()
    const val FRAME = 0xFF4A1208.toInt()
    const val ACCENT = 0xFFFF5A1E.toInt()
    const val LABEL = 0xFFF0F0F0.toInt()
    const val DIM = 0xFF9A9A9A.toInt()
    const val GOOD = 0xFF3CE05A.toInt()
    const val WARNING = 0xFFFFB41E.toInt()
    const val DANGER = 0xFFFF2010.toInt()
    const val ALARM = 0xFFB81008.toInt()
    const val ALARM_TEXT = 0xFFFFF0E0.toInt()
    const val COLD = 0xFF2E8CFF.toInt()
}

// A gauge's scale and bands, as range_t: below dangerLow or from
// dangerHigh is danger, below warnLow or from warnHigh warns, below cold
// is an engine not warm yet; -1 where a band doesn't exist.
data class Range(
    val min: Float,
    val max: Float,
    val warnLow: Float,
    val warnHigh: Float,
    val dangerLow: Float,
    val dangerHigh: Float,
    val cold: Float,
) {
    fun color(v: Float): Int =
        when {
            (dangerLow >= 0 && v < dangerLow) || v >= dangerHigh -> Colors.DANGER
            (warnLow >= 0 && v < warnLow) || v >= warnHigh -> Colors.WARNING
            cold >= 0 && v < cold -> Colors.COLD
            else -> Colors.GOOD
        }

    fun danger(v: Float): Boolean = color(v) == Colors.DANGER

    // Where v sits on the scale, 0 to 1.
    fun fraction(v: Float): Float = ((v - min) / (max - min)).coerceIn(0f, 1f)

    // The bands from the bottom of the scale up, each with the value it
    // starts at (the scale's min for the first): the legend of a gauge
    // alone.
    fun bands(): List<Pair<Float, Int>> {
        val edges = listOf(min, dangerLow, warnLow, cold, warnHigh, dangerHigh)
            .filter { it > min && it < max } + min
        return edges.distinct().sorted().map { it to color(it) }
            .fold(listOf<Pair<Float, Int>>()) { out, b -> if (out.lastOrNull()?.second == b.second) out else out + b }
    }
}

class Gauge(
    val label: String,
    val short: String,
    val unit: String,
    val digits: Int,
    val range: Range,
    val value: (Readings) -> Float?,
) {
    fun text(v: Float?): String = if (v == null) "--" else "%.${digits}f".format(v)
}

val OIL = Gauge("OIL TEMP", "OIL", "°C", 0, Range(40f, 150f, -1f, 110f, -1f, 120f, 80f)) { it.oil }
val COOLANT = Gauge("COOLANT", "COOLANT", "°C", 0, Range(40f, 130f, -1f, 105f, -1f, 115f, 70f)) { it.coolant }
val AFR = Gauge("AFR", "AFR", "", 1, Range(10f, 18f, 12f, 16f, 11f, 17f, -1f)) { it.afr }
val INTAKE = Gauge("INTAKE", "INTAKE", "°C", 0, Range(-10f, 80f, -1f, 45f, -1f, 60f, -1f)) { it.intake }
val BATTERY = Gauge("BATTERY", "BATTERY", "V", 1, Range(10f, 16f, 12f, 15f, 11.5f, 15.5f, -1f)) { it.battery }
val ETHANOL = Gauge("ETHANOL", "ETHANOL", "%", 0, Range(0f, 100f, -1f, 101f, -1f, 101f, -1f)) { it.ethanol }

// The order of the views of one gauge alone, after the main one.
val GAUGES = listOf(OIL, COOLANT, AFR, INTAKE, BATTERY, ETHANOL)

// From OIL_CRITICAL the oil takes the whole screen; from LIMITER_FLASH_RPM
// the page flashes red with SHIFT across it (ui.c).
const val OIL_CRITICAL = 130f
const val LIMITER_FLASH_RPM = 7300f
const val LIMITER_FLASH_MS = 100L

// The shift light: SHIFT_LEDS / 2 pairs over the last SHIFT_SPAN_RPM
// before the shift RPM, all flashing blue from it.
const val SHIFT_LEDS = 20
const val SHIFT_SPAN_RPM = 1500f
const val SHIFT_BLINK_MS = 60L

// A reading in danger blinks this fast.
const val DANGER_BLINK_MS = 250L
