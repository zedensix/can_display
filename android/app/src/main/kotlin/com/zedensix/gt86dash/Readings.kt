package com.zedensix.gt86dash

// What the dash draws from: the car's readings at one moment, null where
// none came in lately (drawn "--"), as the Pico's state_t has them.

// The stability control's modes, as the Pico decodes 0x0D3: comfort is
// everything on, the normal layout; the others take the track layout.
enum class DriveMode(val badge: String) {
    COMFORT(""),
    DRIFT("DRIFT MODE"),
    SPORT("SPORT MODE"),
    TRACK("TRACK MODE"),
    RACE("RACE MODE"),
}

data class Readings(
    val oil: Float? = null,
    val coolant: Float? = null,
    val afr: Float? = null,
    val intake: Float? = null,
    val battery: Float? = null,
    val ethanol: Float? = null,
    val rpm: Float? = null,
    // 0 for neutral (or the clutch in), null when unknown.
    val gear: Int? = null,
    val outside: Float? = null,
    val km: Float? = null,
    val mode: DriveMode = DriveMode.COMFORT,
    // Off the throttle the AFR reads lean from the fuel cut: no alarm.
    val offThrottle: Boolean = false,
)

// The lowest and highest of each gauge since the app started (or the
// settings reset them), as the Pico's reading_t keeps them since boot: one
// for the car's screen and the phone's.
object Extremes {
    private val low = FloatArray(GAUGES.size) { Float.NaN }
    private val high = FloatArray(GAUGES.size) { Float.NaN }

    fun update(r: Readings) {
        GAUGES.forEachIndexed { i, g ->
            val v = g.value(r) ?: return@forEachIndexed
            if (low[i].isNaN() || v < low[i]) low[i] = v
            if (high[i].isNaN() || v > high[i]) high[i] = v
        }
    }

    fun reset() {
        low.fill(Float.NaN)
        high.fill(Float.NaN)
    }

    fun min(g: Gauge): Float? = low[GAUGES.indexOf(g)].takeUnless { it.isNaN() }

    fun max(g: Gauge): Float? = high[GAUGES.indexOf(g)].takeUnless { it.isNaN() }
}
