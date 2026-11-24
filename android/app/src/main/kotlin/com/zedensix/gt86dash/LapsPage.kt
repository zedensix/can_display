package com.zedensix.gt86dash

import android.app.Activity
import android.content.res.Configuration
import android.os.Handler
import android.os.Looper
import android.view.Gravity
import android.view.View
import android.view.ViewGroup.LayoutParams.MATCH_PARENT
import android.view.ViewGroup.LayoutParams.WRAP_CONTENT
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast

// Lap timing, a page in place of the dash: the lap running, big, the
// best and the last under it, what it waits for when it can't time
// (lap timing off, no GPS fix, no start line near, no crossing yet); the
// switch, the start line set where the phone is, the laps so far, the
// best in green. Two columns in landscape.
class LapsPage(private val activity: Activity, private val onClose: () -> Unit) : Page {
    private val settings = Settings(activity)
    private val handler = Handler(Looper.getMainLooper())
    private lateinit var lap: TextView
    private lateinit var line: TextView
    private lateinit var laps: LinearLayout
    private var shownLaps = -1

    private val tick = object : Runnable {
        override fun run() {
            refresh()
            handler.postDelayed(this, TICK_MS)
        }
    }

    override fun resume() {
        handler.removeCallbacks(tick)
        handler.post(tick)
    }

    override fun pause() {
        handler.removeCallbacks(tick)
    }

    override fun build(bottom: Int): View {
        val a = activity
        val landscape = a.resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
        val column = LinearLayout(a).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(a, 24), dp(a, 8), dp(a, 24), dp(a, 24) + bottom)
        }
        column.addView(pageHeader(a, "LAP TIMING", onClose))

        val live = LinearLayout(a).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_HORIZONTAL
        }
        lap = pageText(a, "", 88f, R.font.barlow_condensed_bold, Colors.LABEL).apply { gravity = Gravity.CENTER }
        line = pageText(a, "", 22f, R.font.barlow_condensed_semibold, Colors.DIM).apply { gravity = Gravity.CENTER }
        live.addView(lap)
        live.addView(line)

        val controls = LinearLayout(a).apply { orientation = LinearLayout.VERTICAL }
        controls.addView(
            pageSwitch(a, "Lap times from the phone's GPS", settings.lapTiming) {
                settings.lapTiming = it
                a.wantGps()
            },
        )
        controls.addView(
            pagePill(a, "Set the start line here") { Toast.makeText(a, Recorder.setStartLine(a), Toast.LENGTH_SHORT).show() },
            LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT).apply { topMargin = dp(a, 12) },
        )
        controls.addView(help("Tap it crossing the line: it lies across the way you drive, one per track, found again when you're back"))
        controls.addView(pageText(a, "LAPS", 15f, R.font.barlow_condensed_semibold, Colors.DIM, 0.15f).apply { setPadding(0, dp(a, 24), 0, dp(a, 8)) })
        laps = LinearLayout(a).apply { orientation = LinearLayout.VERTICAL }
        controls.addView(laps)
        controls.addView(
            pagePill(a, "Forget the start lines") {
                Recorder.forgetStartLines(a)
                Toast.makeText(a, "Start lines forgotten", Toast.LENGTH_SHORT).show()
            },
            LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT).apply { topMargin = dp(a, 24) },
        )

        val body = LinearLayout(a).apply { orientation = if (landscape) LinearLayout.HORIZONTAL else LinearLayout.VERTICAL }
        if (landscape) {
            body.addView(live, LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f).apply { topMargin = dp(a, 32) })
            body.addView(View(a), LinearLayout.LayoutParams(dp(a, 32), 0))
            body.addView(controls, LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f))
        } else {
            body.addView(live, LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT).apply { setMargins(0, dp(a, 24), 0, dp(a, 24)) })
            body.addView(controls, LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT))
        }
        column.addView(body)
        shownLaps = -1
        refresh()
        return ScrollView(a).apply {
            setBackgroundColor(Colors.BACKGROUND)
            clipToPadding = false
            addView(column)
        }
    }

    private fun refresh() {
        val t = Recorder.timer
        val start = t?.lapStart
        val now = System.currentTimeMillis()
        val last = t?.laps?.lastOrNull()
        when {
            !settings.lapTiming -> show("--:--.-", Colors.DIM, "Lap timing is off")
            !Recorder.locationGranted(activity) -> show("--:--.-", Colors.DIM, "The GPS isn't allowed")
            !Recorder.hasFix -> show("--:--.-", Colors.DIM, "Waiting for the GPS")
            t == null -> show("--:--.-", Colors.DIM, "No start line near: set one crossing it")
            start == null -> show("0:00.0", Colors.DIM, "Waiting for the first crossing")
            last != null && now - start < Recorder.SHOW_LAP_MS ->
                show(LapTimer.text(last), if (last == t.best) Colors.GOOD else Colors.WARNING, "LAP ${t.laps.size}${if (last == t.best) "  ·  BEST" else ""}")
            else -> show(LapTimer.text(now - start), Colors.LABEL, listOfNotNull("LAP ${t.laps.size + 1}", t.best?.let { "BEST ${LapTimer.text(it)}" }, last?.let { "LAST ${LapTimer.text(it)}" }).joinToString("  ·  "))
        }
        val times = t?.laps.orEmpty()
        if (times.size == shownLaps) return
        shownLaps = times.size
        laps.removeAllViews()
        if (times.isEmpty()) laps.addView(help("None yet"))
        val best = times.minOrNull()
        times.forEachIndexed { i, ms ->
            laps.addView(pageText(activity, "LAP ${i + 1}   ${LapTimer.text(ms)}", 24f, R.font.barlow_condensed_semibold, if (ms == best) Colors.GOOD else Colors.LABEL))
        }
    }

    private fun show(time: String, color: Int, text: String) {
        lap.text = time
        lap.setTextColor(color)
        line.text = text
    }

    private fun help(s: String) =
        pageText(activity, s, 15f, R.font.barlow_condensed_medium, Colors.DIM).apply { setPadding(0, dp(context, 4), 0, 0) }

    private companion object {
        const val TICK_MS = 100L
    }
}
