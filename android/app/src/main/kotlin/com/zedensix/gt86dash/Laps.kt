package com.zedensix.gt86dash

import kotlin.math.abs
import kotlin.math.cos
import kotlin.math.hypot
import kotlin.math.sin

// A GPS fix: the phone's clock (ms), where (degrees), its speed (m/s),
// heading (degrees from north, clockwise) and accuracy (m).
data class Fix(val ms: Long, val lat: Double, val lon: Double, val speed: Float, val bearing: Float, val accuracy: Float)

// A start/finish line: across the track at a point, square to the way
// the car drove over it (heading, degrees from north), crossed that way.
data class StartLine(val lat: Double, val lon: Double, val heading: Float) {
    // Metres east and north of the line's point, flat over a track's size.
    private fun local(lat: Double, lon: Double): Pair<Double, Double> =
        Pair((lon - this.lon) * M_PER_DEGREE * cos(Math.toRadians(this.lat)), (lat - this.lat) * M_PER_DEGREE)

    fun distance(lat: Double, lon: Double): Double = local(lat, lon).let { (x, y) -> hypot(x, y) }

    // When the car crossed the line between two fixes, interpolated
    // between them (GPS comes about once a second: at 150 km/h a fix is
    // 40 m from the next), or null if it didn't, the wrong way, too far to
    // the side, or across a gap in the fixes.
    fun crossing(a: Fix, b: Fix): Long? {
        if (b.ms - a.ms !in 1..MAX_GAP_MS) return null
        val h = Math.toRadians(heading.toDouble())
        val (ax, ay) = local(a.lat, a.lon)
        val (bx, by) = local(b.lat, b.lon)
        // Along the way it was crossed, then along the line.
        val da = ax * sin(h) + ay * cos(h)
        val db = bx * sin(h) + by * cos(h)
        if (!(da < 0 && db >= 0)) return null
        val f = -da / (db - da)
        val side = (ax + f * (bx - ax)) * cos(h) - (ay + f * (by - ay)) * sin(h)
        if (abs(side) > HALF_WIDTH_M) return null
        return a.ms + (f * (b.ms - a.ms)).toLong()
    }

    // The laps in a drive's fixes: each one's time, in ms.
    fun laps(fixes: List<Fix>): List<Long> {
        val times = LapTimer(this).also { t -> fixes.forEach { t.add(it) } }
        return times.laps
    }

    // As settings keep it.
    fun encode() = "$lat,$lon,$heading"

    companion object {
        const val M_PER_DEGREE = 111_320.0
        const val HALF_WIDTH_M = 20.0
        const val MAX_GAP_MS = 5000L

        // A track's line is used from this close.
        const val NEAR_M = 2000.0

        fun decode(s: String): StartLine? =
            s.split(',').takeIf { it.size == 3 }?.let { (lat, lon, h) ->
                StartLine(lat.toDoubleOrNull() ?: return null, lon.toDoubleOrNull() ?: return null, h.toFloatOrNull() ?: return null)
            }

        // The line nearest to a point, within NEAR_M.
        fun nearest(lines: List<StartLine>, lat: Double, lon: Double): StartLine? =
            lines.filter { it.distance(lat, lon) < NEAR_M }.minByOrNull { it.distance(lat, lon) }
    }
}

// The laps as the car drives them, one fix after another: a lap from a
// crossing to the next, a crossing sooner than MIN_LAP_MS after the last
// one ignored (GPS wandering over the line).
class LapTimer(val line: StartLine) {
    val laps = mutableListOf<Long>()
    var lapStart: Long? = null
        private set
    var crossedAt: Long? = null
        private set
    private var previous: Fix? = null

    val best: Long? get() = laps.minOrNull()

    // Returns whether that fix finished a lap.
    fun add(fix: Fix): Boolean {
        val crossed = previous?.let { line.crossing(it, fix) }
        previous = fix
        if (crossed == null) return false
        val start = lapStart
        if (start != null && crossed - start < MIN_LAP_MS) return false
        lapStart = crossed
        crossedAt = crossed
        if (start == null) return false
        laps += crossed - start
        return true
    }

    companion object {
        const val MIN_LAP_MS = 20_000L

        // A lap time as the dash shows it: 1:23.4.
        fun text(ms: Long): String = "%d:%02d.%d".format(ms / 60_000, ms / 1000 % 60, ms / 100 % 10)
    }
}
