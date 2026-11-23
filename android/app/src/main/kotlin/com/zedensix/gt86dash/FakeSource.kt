package com.zedensix.gt86dash

import kotlin.math.pow
import kotlin.math.sin

// Made-up readings until the dash sends real ones: a lap of pulls through
// the gears, warm, a stretch on the track layout every other minute, as
// the simulator's tour does (sim/simdata.c). scene() gives fixed ones,
// for screenshots.
object FakeSource {
    fun at(ms: Long): Readings {
        val t = ms / 80f
        val phase = (t % 75) / 75
        val rpm = 3600 + 4000 * phase.pow(1.3f)
        val minute = (ms / 60_000) % 2
        return Readings(
            oil = 104 + 7 * sin(t / 90),
            coolant = 91 + sin(t / 120),
            afr = 12.6f + 1.7f * sin(t / 9),
            intake = 31 + 2 * sin(t / 50),
            battery = 14.2f + 0.1f * sin(t / 30),
            ethanol = 68f,
            rpm = rpm,
            gear = 2 + (t / 75).toInt() % 4,
            outside = 14f,
            km = 37412f,
            mode = if (minute == 1L && ms % 60_000 < 30_000) DriveMode.SPORT else DriveMode.COMFORT,
            offThrottle = phase < 0.05f,
        )
    }

    fun scene(name: String): Readings {
        val street = Readings(
            oil = 104f, coolant = 91f, afr = 13.1f, intake = 31f, battery = 14.2f, ethanol = 68f,
            rpm = 6100f, gear = 3, outside = 14f, km = 37412f,
        )
        return when (name) {
            "track" -> street.copy(oil = 116f, afr = 12.4f, coolant = 96f, intake = 38f, rpm = 6650f, gear = 4, mode = DriveMode.TRACK)
            "race" -> street.copy(oil = 121f, coolant = 99f, rpm = 7100f, gear = 3, mode = DriveMode.RACE)
            "cold" -> street.copy(oil = 52f, coolant = 61f, rpm = 1500f, gear = 0)
            "alarm" -> street.copy(oil = 134f)
            "stale" -> Readings(oil = 92f, coolant = 88f)
            else -> street
        }
    }
}
