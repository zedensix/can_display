package com.zedensix.gt86dash

import android.content.Context
import java.io.File
import java.time.Instant
import java.time.ZoneId
import java.time.format.DateTimeFormatter
import java.util.Locale

// A drive the phone recorded: its GPS fixes, a line each, from when a
// spirited drive started (or lap timing went on, on a track) to when it
// stopped. Its file is named after its start: <start ms>-<kind>.csv.
class Drive(val file: File) {
    val start: Long = file.name.substringBefore('-').toLongOrNull() ?: 0L
    val kind: String = file.name.substringAfter('-').substringBefore('.')

    // A line cut short (the app stopped mid-write) is left out.
    fun fixes(): List<Fix> = file.readLines().mapNotNull { line ->
        val p = line.split(',')
        if (p.size < 6) return@mapNotNull null
        val d = p.map { it.toDoubleOrNull() ?: return@mapNotNull null }
        Fix(d[0].toLong(), d[1], d[2], d[3].toFloat(), d[4].toFloat(), d[5].toFloat())
    }

    // Its name in a backup: gt86-drive-2026-11-25-1834.gpx.
    val backupName: String get() = "gt86-drive-${NAME.format(Instant.ofEpochMilli(start))}.gpx"

    fun title(): String = TITLE.format(Instant.ofEpochMilli(start))

    companion object {
        private val NAME = DateTimeFormatter.ofPattern("yyyy-MM-dd-HHmm").withZone(ZoneId.systemDefault())
        private val TITLE = DateTimeFormatter.ofPattern("EEE d MMM, HH:mm").withZone(ZoneId.systemDefault())
    }
}

// What a drive comes to: how long, how far, how fast, its laps on the
// start line saved nearest to it.
class DriveSummary(val drive: Drive, val fixes: List<Fix>, val line: StartLine?) {
    val ms: Long = if (fixes.size < 2) 0 else fixes.last().ms - fixes.first().ms
    val metres: Double = fixes.zipWithNext { a, b ->
        StartLine(a.lat, a.lon, 0f).distance(b.lat, b.lon)
    }.sum()
    val topSpeed: Float = fixes.maxOfOrNull { it.speed } ?: 0f
    val laps: List<Long> = line?.laps(fixes) ?: emptyList()

    companion object {
        fun of(drive: Drive, lines: List<StartLine>): DriveSummary {
            val fixes = drive.fixes()
            val line = fixes.firstNotNullOfOrNull { f -> StartLine.nearest(lines, f.lat, f.lon) }
            return DriveSummary(drive, fixes, line)
        }
    }
}

// The drives on the phone, the newest first, and the one being recorded.
object Drives {
    private fun dir(c: Context) = File(c.filesDir, "drives").apply { mkdirs() }

    fun all(c: Context): List<Drive> =
        dir(c).listFiles { f -> f.name.endsWith(".csv") }.orEmpty().map { Drive(it) }.sortedByDescending { it.start }

    fun begin(c: Context, kind: String, now: Long): Drive = Drive(File(dir(c), "$now-$kind.csv"))

    fun append(drive: Drive, f: Fix) {
        drive.file.appendText(
            String.format(Locale.ROOT, "%d,%.7f,%.7f,%.2f,%.1f,%.1f\n", f.ms, f.lat, f.lon, f.speed, f.bearing, f.accuracy),
        )
    }

    // A drive as GPX, what map apps and lap tools read: the path, its
    // speed in each point's extensions.
    fun gpx(fixes: List<Fix>): String {
        val time = DateTimeFormatter.ISO_INSTANT
        val out = StringBuilder(
            """<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="GT86 Dash" xmlns="http://www.topografix.com/GPX/1/1">
<trk><name>GT86 drive</name><trkseg>
""",
        )
        for (f in fixes) {
            out.append(String.format(Locale.ROOT, "<trkpt lat=\"%.7f\" lon=\"%.7f\">", f.lat, f.lon))
            out.append("<time>").append(time.format(Instant.ofEpochMilli(f.ms))).append("</time>")
            out.append(String.format(Locale.ROOT, "<extensions><speed>%.2f</speed></extensions></trkpt>\n", f.speed))
        }
        return out.append("</trkseg></trk>\n</gpx>\n").toString()
    }
}
