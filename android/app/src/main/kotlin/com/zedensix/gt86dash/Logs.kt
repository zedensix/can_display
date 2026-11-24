package com.zedensix.gt86dash

import android.content.Context
import java.io.File
import java.io.FileOutputStream
import java.io.RandomAccessFile

// The dash's data log as the app pulls it, kept on the phone: each
// session (a boot of the dash) in a file of its raw records, oldest
// first; when it booted by the phone's clock, for the sessions the phone
// saw running; where the next pull starts, for each dash.
object Logs {
    class Session(val number: Int, val file: File, val bootAt: Long?) {
        val backupName: String get() = "gt86-log-%05d.csv".format(number)

        fun samples(): List<Sample> = Records.parse(file.readBytes())
    }

    private fun dir(c: Context) = File(c.filesDir, "logs").apply { mkdirs() }

    private fun prefs(c: Context) = c.getSharedPreferences("logs", Context.MODE_PRIVATE)

    private fun file(c: Context, session: Int) = File(dir(c), "session-$session.bin")

    // The newest first.
    fun sessions(c: Context): List<Session> {
        val p = prefs(c)
        return dir(c).listFiles { f -> f.name.startsWith("session-") }.orEmpty().mapNotNull { f ->
            val n = f.name.removePrefix("session-").removeSuffix(".bin").toIntOrNull() ?: return@mapNotNull null
            Session(n, f, p.getLong("boot_$n", 0L).takeIf { it != 0L })
        }.sortedByDescending { it.number }
    }

    fun from(c: Context, dash: String): Long = prefs(c).getLong("from_$dash", Pull.FROM_OLDEST)

    // When the last pull came in, by the phone's clock (0: never).
    fun pulledAt(c: Context): Long = prefs(c).getLong("pulled_at", 0L)

    // Keeps a pull from dash, at now: each session's records newer than
    // what its file holds appended to it. Returns the sessions that got
    // some.
    fun keep(c: Context, dash: String, h: PullHeader, bytes: ByteArray, now: Long): Set<Int> {
        val got = mutableSetOf<Int>()
        for ((session, records) in Records.walk(bytes).groupBy { it.first.session }) {
            val f = file(c, session)
            val last = lastMs(f)
            val fresh = records.filter { last == null || it.first.ms > last }
            if (fresh.isEmpty()) continue
            FileOutputStream(f, true).buffered().use { out -> fresh.forEach { out.write(it.second) } }
            got += session
        }
        val p = prefs(c).edit().putLong("from_$dash", h.next).putLong("pulled_at", now)
        if (!prefs(c).contains("boot_${h.session}")) p.putLong("boot_${h.session}", now - h.nowMs)
        p.apply()
        return got
    }

    private fun lastMs(f: File): Long? {
        if (f.length() < Records.RECORD) return null
        RandomAccessFile(f, "r").use { r ->
            val raw = ByteArray(Records.RECORD)
            r.seek(f.length() - Records.RECORD)
            r.readFully(raw)
            return Records.parse(raw).firstOrNull()?.ms
        }
    }
}
