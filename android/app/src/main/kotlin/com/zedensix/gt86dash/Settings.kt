package com.zedensix.gt86dash

import android.content.Context

// The app's settings, kept on the phone, the same for the car's screen and
// the phone's: the shift light's RPM as the Pico's (SHIFT_RPM_* in
// settings.h), 0 for none; made-up readings instead of the dash's (demo);
// the dash to connect to, by its Bluetooth address (null: the first found),
// and its name; pulling the dash's data log; where it and the drives are
// backed up (off, a folder on the phone, a WebDAV folder such as
// Penombre's); lap timing and its start lines; a spirited drive under way.
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

    var dash: String?
        get() = prefs.getString("dash", null)
        set(address) = prefs.edit().putString("dash", address).apply()

    var dashName: String?
        get() = prefs.getString("dash_name", null)
        set(name) = prefs.edit().putString("dash_name", name).apply()

    var collect: Boolean
        get() = prefs.getBoolean("collect", true)
        set(on) = prefs.edit().putBoolean("collect", on).apply()

    var backup: Backup.Kind
        get() = Backup.Kind.entries.firstOrNull { it.name == prefs.getString("backup", null) } ?: Backup.Kind.OFF
        set(kind) = prefs.edit().putString("backup", kind.name).apply()

    // The folder picked on the phone (a document tree's URI), and its name.
    var folder: String?
        get() = prefs.getString("folder", null)
        set(uri) = prefs.edit().putString("folder", uri).apply()

    var folderName: String?
        get() = prefs.getString("folder_name", null)
        set(name) = prefs.edit().putString("folder_name", name).apply()

    // The WebDAV folder's address (Penombre's: https://host/dav/me/GT86/)
    // and the key it takes as the password (Penombre's API key).
    var webdavUrl: String
        get() = prefs.getString("webdav_url", "") ?: ""
        set(url) = prefs.edit().putString("webdav_url", url).apply()

    var webdavKey: String
        get() = prefs.getString("webdav_key", "") ?: ""
        set(key) = prefs.edit().putString("webdav_key", key).apply()

    var lapTiming: Boolean
        get() = prefs.getBoolean("lap_timing", false)
        set(on) = prefs.edit().putBoolean("lap_timing", on).apply()

    var startLines: List<StartLine>
        get() = (prefs.getString("start_lines", "") ?: "").split(';').mapNotNull { StartLine.decode(it) }
        set(lines) = prefs.edit().putString("start_lines", lines.joinToString(";") { it.encode() }).apply()

    var spirited: Boolean
        get() = prefs.getBoolean("spirited", false)
        set(on) = prefs.edit().putBoolean("spirited", on).apply()

    fun shiftRpmText(): String = if (shiftRpm == 0) "Off" else "$shiftRpm rpm"

    companion object {
        const val SHIFT_RPM_MIN = 5000
        const val SHIFT_RPM_MAX = 7600
        const val SHIFT_RPM_STEP = 100
    }
}
