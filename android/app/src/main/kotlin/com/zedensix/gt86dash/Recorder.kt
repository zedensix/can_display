package com.zedensix.gt86dash

import android.Manifest
import android.annotation.SuppressLint
import android.content.Context
import android.content.pm.PackageManager
import android.location.Location
import android.location.LocationListener
import android.location.LocationManager
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import java.util.concurrent.Executors

// What runs while the app is up for the car's screen or the phone's (they
// hold it), or a spirited drive is under way: the dash's data log pulled
// every PULL_MS while its link is live (Logs), the GPS while lap timing
// or a spirited drive wants it, the laps on the start line nearest, the
// drive being recorded (Drives), the backups when a session or a drive
// ends. RecorderService keeps the app up meanwhile, the screen off.
@SuppressLint("MissingPermission") // gps() checks it
object Recorder {
    private const val PULL_MS = 60_000L
    private const val TICK_MS = 1000L

    // A finished lap's time shows this long before the next one runs.
    const val SHOW_LAP_MS = 5000L

    val locationPermissions = arrayOf(Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION)

    private var holders = 0
    private var running = false
    private var context: Context? = null
    private val handler = Handler(Looper.getMainLooper())
    private val worker = Executors.newSingleThreadExecutor()
    private var pulledAt = -PULL_MS
    private var dashSession: Int? = null
    private var listening = false
    private var lastFix: Fix? = null
    private var lastHeading = 0f
    private var drive: Drive? = null

    // The laps on the start line nearest, while lap timing is on near one.
    @Volatile var timer: LapTimer? = null
        private set

    val recording: Boolean get() = drive != null

    val hasFix: Boolean get() = lastFix != null

    fun locationGranted(c: Context) = c.checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED

    fun hold(c: Context) {
        context = c.applicationContext
        holders++
        update()
    }

    fun release() {
        holders = (holders - 1).coerceAtLeast(0)
        update()
    }

    // Starts or stops what the holders and the settings want now: called
    // after either changes.
    fun update() {
        val c = context ?: return
        val settings = Settings(c)
        val run = holders > 0 || settings.spirited
        if (run && !running) {
            running = true
            LiveLink.start(c)
            handler.post(tick)
        } else if (!run && running) {
            running = false
            handler.removeCallbacks(tick)
            LiveLink.stop()
            Backup.run(c)
        }
        gps(c, running && (settings.lapTiming || settings.spirited) && locationGranted(c))
        if (!settings.lapTiming) timer = null
        val record = running && (settings.spirited || (settings.lapTiming && timer != null))
        if (record && drive == null) {
            drive = Drives.begin(c, if (settings.spirited) "spirited" else "track", System.currentTimeMillis())
        } else if (!record && drive != null) {
            drive = null
            Backup.run(c)
        }
        if (running) RecorderService.start(c, listening, drive != null) else RecorderService.stop(c)
    }

    private val tick = object : Runnable {
        override fun run() {
            pullIfDue()
            handler.postDelayed(this, TICK_MS)
        }
    }

    private fun pullIfDue() {
        val c = context ?: return
        if (Settings(c).collect && SystemClock.uptimeMillis() - pulledAt >= PULL_MS) pullNow(false) {}
    }

    // Pulls the dash's log now (before its firmware is updated, whatever
    // the setting): received runs once it's in, on Bluetooth's thread.
    // False if it can't be asked now (LiveLink.pull).
    fun pullNow(forUpdate: Boolean, received: () -> Unit): Boolean {
        val c = context ?: return false
        val dash = LiveLink.device ?: return false
        val asked = LiveLink.pull(Logs.from(c, dash), forUpdate) { h, bytes ->
            worker.execute { kept(c, dash, h, bytes) }
            received()
        }
        if (asked) pulledAt = SystemClock.uptimeMillis()
        return asked
    }

    // A pull in: kept, and the last session backed up once the dash is in
    // a new one (the contact was off).
    private fun kept(c: Context, dash: String, h: PullHeader, bytes: ByteArray) {
        Logs.keep(c, dash, h, bytes, System.currentTimeMillis())
        if (dashSession != null && dashSession != h.session) Backup.run(c)
        dashSession = h.session
    }

    private fun gps(c: Context, on: Boolean) {
        if (on == listening) return
        val location = c.getSystemService(LocationManager::class.java) ?: return
        if (on) {
            location.requestLocationUpdates(LocationManager.GPS_PROVIDER, 0L, 0f, listener, Looper.getMainLooper())
        } else {
            location.removeUpdates(listener)
            lastFix = null
        }
        listening = on
    }

    private val listener = LocationListener { l: Location ->
        val fix = Fix(l.time, l.latitude, l.longitude, l.speed, l.bearing, l.accuracy)
        // A heading is only worth anything on the move.
        if (l.hasBearing() && l.speed > 3f) lastHeading = l.bearing
        lastFix = fix
        val c = context ?: return@LocationListener
        val settings = Settings(c)
        if (settings.lapTiming) {
            val line = StartLine.nearest(settings.startLines, fix.lat, fix.lon)
            if (line != timer?.line) timer = line?.let { LapTimer(it) }
            timer?.add(fix)
        }
        if (drive == null && timer != null) update()
        drive?.let { d -> worker.execute { Drives.append(d, fix) } }
    }

    // The start line where the phone is, across the way it last drove:
    // replaces the one saved on that track. Says how it went.
    fun setStartLine(c: Context): String {
        val fix = lastFix ?: return if (listening) "No GPS fix yet" else "Turn lap timing on first"
        val line = StartLine(fix.lat, fix.lon, lastHeading)
        val settings = Settings(c)
        settings.startLines = settings.startLines.filter { it.distance(fix.lat, fix.lon) >= StartLine.NEAR_M } + line
        timer = LapTimer(line)
        update()
        return "Start line set"
    }

    fun forgetStartLines(c: Context) {
        Settings(c).startLines = emptyList()
        timer = null
        update()
    }

    // What the dash's strip shows of the laps at now (the phone's clock):
    // the lap just finished for SHOW_LAP_MS (green if it's the best), else
    // the lap running and the best; null while lap timing is off or no
    // line is near.
    fun lapText(now: Long, short: Boolean): Pair<String, Int>? {
        val t = timer ?: return null
        val start = t.lapStart ?: return "LAP --" to Colors.DIM
        val last = t.laps.lastOrNull()
        if (last != null && now - start < SHOW_LAP_MS) {
            return "LAP ${t.laps.size}  ${LapTimer.text(last)}" to if (last == t.best) Colors.GOOD else Colors.WARNING
        }
        val running = "LAP ${t.laps.size + 1}  ${LapTimer.text(now - start)}"
        val best = t.best ?: return running to Colors.LABEL
        return (if (short) running else "$running   BEST ${LapTimer.text(best)}") to Colors.LABEL
    }
}

// Lap timing or a spirited drive turned on from the phone: the GPS's
// permission asked if it isn't granted (ask, the page's launcher, tells the
// recorder its answer), and the recorder told.
fun Context.wantGps(ask: (Array<String>) -> Unit) {
    val settings = Settings(this)
    if ((settings.lapTiming || settings.spirited) && !Recorder.locationGranted(this)) ask(Recorder.locationPermissions)
    Recorder.update()
}
