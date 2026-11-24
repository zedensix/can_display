package com.zedensix.gt86dash

import android.app.Activity
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.content.Context
import android.content.Intent
import android.content.res.ColorStateList
import android.content.res.Configuration
import android.net.Uri
import android.os.Handler
import android.os.Looper
import android.provider.Settings.ACTION_APPLICATION_DETAILS_SETTINGS
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.ViewGroup.LayoutParams.MATCH_PARENT
import android.view.ViewGroup.LayoutParams.WRAP_CONTENT
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.Switch
import android.widget.TextView
import android.widget.Toast

// The phone's settings, a page in place of the dash (the navigation
// stays): the connection (the link's state, what it needs, and which
// dash: the first found, or one picked from those nearby or used before),
// then the gauges' (the shift light, demo, the min and max). Two columns
// in landscape. bottom leaves room under it for a floating bar, the page
// scrolling under it. Its owner says when it shows (resume, pause) and
// hands it Bluetooth's permissions as they're answered.
class SettingsPage(private val activity: Activity, private val onClose: () -> Unit) {
    private val settings = Settings(activity)
    private val picker = DashPicker(activity, settings)
    private lateinit var status: TextView
    private lateinit var detail: TextView
    private lateinit var fix: TextView
    private lateinit var shiftValue: TextView
    private val handler = Handler(Looper.getMainLooper())

    private val tick = object : Runnable {
        override fun run() {
            refresh()
            handler.postDelayed(this, TICK_MS)
        }
    }

    fun resume() {
        handler.removeCallbacks(tick)
        handler.post(tick)
    }

    fun pause() {
        handler.removeCallbacks(tick)
        picker.stop()
    }

    // Bluetooth's permissions answered: refused for good, the system asks
    // no more, its own page for them.
    fun permissionsAnswered() {
        if (!LiveLink.granted(activity) && !activity.shouldShowRequestPermissionRationale(LiveLink.permissions[0])) {
            activity.startActivity(Intent(ACTION_APPLICATION_DETAILS_SETTINGS, Uri.fromParts("package", activity.packageName, null)))
        }
    }

    private fun adapter() = activity.getSystemService(BluetoothManager::class.java)?.adapter

    // The page, laid out for the orientation it's in (built again when
    // the phone turns), bottom pixels of room under it.
    fun build(bottom: Int): View {
        (picker.view.parent as? ViewGroup)?.removeView(picker.view)
        val a = activity
        val column = LinearLayout(a).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(a, 24), dp(a, 8), dp(a, 24), dp(a, 24) + bottom)
        }
        val header = LinearLayout(a).apply { gravity = Gravity.CENTER_VERTICAL }
        header.addView(iconButton(a, R.drawable.ic_back, 44) { onClose() }, LinearLayout.LayoutParams(dp(a, 44), dp(a, 44)))
        header.addView(text("SETTINGS", 22f, R.font.barlow_condensed_bold, Colors.ACCENT, 0.15f).apply { setPadding(dp(context, 16), 0, 0, 0) })
        column.addView(header)

        val landscape = a.resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
        val body = LinearLayout(a).apply { orientation = if (landscape) LinearLayout.HORIZONTAL else LinearLayout.VERTICAL }
        val each = if (landscape) LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f) else LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT)
        body.addView(connection(), each)
        body.addView(View(a), LinearLayout.LayoutParams(dp(a, 32), 0))
        body.addView(gauges(), LinearLayout.LayoutParams(each))
        column.addView(body)

        val scroll = ScrollView(a).apply {
            setBackgroundColor(Colors.BACKGROUND)
            clipToPadding = false
        }
        scroll.addView(column)
        refresh()
        return scroll
    }

    private fun connection(): View {
        val c = LinearLayout(activity).apply { orientation = LinearLayout.VERTICAL }
        c.addView(label("CONNECTION"))
        status = text("", 24f, R.font.barlow_condensed_bold, Colors.LABEL)
        detail = text("", 16f, R.font.barlow_condensed_medium, Colors.DIM)
        fix = pill("") { fixLink() }
        c.addView(status)
        c.addView(detail)
        c.addView(fix, LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT).apply { topMargin = dp(activity, 12) })
        c.addView(label("DASH"))
        c.addView(picker.view)
        c.addView(help("A dash shows here while it's on and no other phone holds its link"))
        return c
    }

    private fun gauges(): View {
        val c = LinearLayout(activity).apply { orientation = LinearLayout.VERTICAL }
        c.addView(label("SHIFT LIGHT"))
        val shift = LinearLayout(activity).apply { gravity = Gravity.CENTER_VERTICAL }
        shiftValue = text("", 30f, R.font.barlow_condensed_bold, Colors.LABEL).apply { gravity = Gravity.CENTER }
        shift.addView(iconButton(activity, R.drawable.ic_remove, 44) { stepShift(-1) }, LinearLayout.LayoutParams(dp(activity, 44), dp(activity, 44)))
        shift.addView(shiftValue, LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f))
        shift.addView(iconButton(activity, R.drawable.ic_add, 44) { stepShift(1) }, LinearLayout.LayoutParams(dp(activity, 44), dp(activity, 44)))
        c.addView(shift)
        c.addView(help("The LEDs light over the last 1500 rpm before it, all blue from it"))
        stepShift(0)

        c.addView(label("DEMO"))
        val demo = Switch(activity).apply {
            text = "Made-up readings instead of the dash's"
            textSize = 20f
            typeface = activity.resources.getFont(R.font.barlow_condensed_semibold)
            setTextColor(Colors.LABEL)
            isChecked = settings.demo
            val states = arrayOf(intArrayOf(android.R.attr.state_checked), intArrayOf())
            thumbTintList = ColorStateList(states, intArrayOf(Colors.ACCENT, 0xFFBDBDBD.toInt()))
            trackTintList = ColorStateList(states, intArrayOf(Colors.FRAME, 0xFF333333.toInt()))
            setOnCheckedChangeListener { _, on -> settings.demo = on }
        }
        c.addView(demo, LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT))

        c.addView(label("MIN AND MAX"))
        c.addView(
            pill("Reset every gauge's") {
                Extremes.reset()
                Toast.makeText(activity, "Min and max reset", Toast.LENGTH_SHORT).show()
            },
        )
        return c
    }

    // The link's state and what it needs, the dashes to pick from (their
    // scan once Bluetooth is allowed and on).
    private fun refresh() {
        LiveLink.retry()
        val device = LiveLink.device
        val (title, line) = when (LiveLink.status) {
            LiveLink.Status.NO_PERMISSION -> "Bluetooth not allowed" to "The app needs it to find the dash"
            LiveLink.Status.BLUETOOTH_OFF -> "Bluetooth is off" to "The dash's readings come over it"
            LiveLink.Status.SEARCHING -> "Looking for the dash" to (settings.dash?.let { picker.name(it) } ?: "the first one found")
            LiveLink.Status.CONNECTING -> "Connecting" to (device?.let { picker.name(it) } ?: "")
            LiveLink.Status.LIVE -> "Live" to (device?.let { picker.name(it) } ?: "")
        }
        status.text = title
        status.setTextColor(
            when (LiveLink.status) {
                LiveLink.Status.LIVE -> Colors.GOOD
                LiveLink.Status.SEARCHING, LiveLink.Status.CONNECTING -> Colors.WARNING
                else -> Colors.DANGER
            },
        )
        detail.text = line
        fix.text = when (LiveLink.status) {
            LiveLink.Status.NO_PERMISSION -> "Allow Bluetooth"
            LiveLink.Status.BLUETOOTH_OFF -> "Turn Bluetooth on"
            else -> ""
        }
        fix.visibility = if (fix.text.isEmpty()) View.GONE else View.VISIBLE
        picker.refresh(LiveLink.granted(activity) && adapter()?.isEnabled == true)
    }

    private fun fixLink() {
        if (!LiveLink.granted(activity)) {
            activity.requestPermissions(LiveLink.permissions, 0)
        } else {
            activity.startActivity(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE))
        }
    }

    private fun stepShift(steps: Int) {
        if (steps != 0) settings.stepShiftRpm(steps)
        shiftValue.text = settings.shiftRpmText()
    }

    private fun text(s: String, size: Float, font: Int, color: Int, spacing: Float = 0f) = pageText(activity, s, size, font, color, spacing)

    private fun label(s: String) =
        text(s, 15f, R.font.barlow_condensed_semibold, Colors.DIM, 0.15f).apply { setPadding(0, dp(context, 24), 0, dp(context, 8)) }

    private fun help(s: String) =
        text(s, 15f, R.font.barlow_condensed_medium, Colors.DIM).apply { setPadding(0, dp(context, 4), 0, 0) }

    private fun pill(s: String, onClick: () -> Unit) = pagePill(activity, s, onClick)

    private companion object {
        const val TICK_MS = 500L
    }
}

// The settings page's text and buttons, shared with DashPicker.
fun pageText(c: Context, s: String, size: Float, font: Int, color: Int, spacing: Float = 0f) =
    TextView(c).apply {
        text = s
        textSize = size
        typeface = c.resources.getFont(font)
        setTextColor(color)
        letterSpacing = spacing
    }

fun pagePill(c: Context, s: String, onClick: () -> Unit) =
    pageText(c, s, 20f, R.font.barlow_condensed_semibold, Colors.LABEL).apply {
        setBackgroundResource(R.drawable.pill_button)
        gravity = Gravity.CENTER
        setPadding(dp(c, 16), dp(c, 10), dp(c, 16), dp(c, 10))
        setOnClickListener { onClick() }
    }
