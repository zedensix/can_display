package com.zedensix.gt86dash

import android.content.Context

// The app's settings, kept on the phone, the same for the car's screen and
// the phone's: the shift light's RPM as the Pico's (SHIFT_RPM_* in
// settings.h), 0 for none; made-up readings instead of the dash's (demo).
class Settings(context: Context) {
    private val prefs = context.getSharedPreferences("settings", Context.MODE_PRIVATE)

    val shiftRpm: Int
        get() = prefs.getInt("shift_rpm", 7000)

    // Steps the shift RPM by SHIFT_RPM_STEP: below SHIFT_RPM_MIN it's off,
    // up from off it starts at SHIFT_RPM_MIN, it stops at SHIFT_RPM_MAX.
    fun stepShiftRpm(steps: Int) {
        val rpm = when {
            shiftRpm == 0 && steps > 0 -> SHIFT_RPM_MIN
            shiftRpm == 0 -> 0
            else -> shiftRpm + steps * SHIFT_RPM_STEP
        }
        val next = if (rpm < SHIFT_RPM_MIN) 0 else rpm.coerceAtMost(SHIFT_RPM_MAX)
        prefs.edit().putInt("shift_rpm", next).apply()
    }

    var demo: Boolean
        get() = prefs.getBoolean("demo", false)
        set(on) = prefs.edit().putBoolean("demo", on).apply()

    fun shiftRpmText(): String = if (shiftRpm == 0) "Off" else "$shiftRpm rpm"

    companion object {
        const val SHIFT_RPM_MIN = 5000
        const val SHIFT_RPM_MAX = 7600
        const val SHIFT_RPM_STEP = 100
    }
}
