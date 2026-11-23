package com.zedensix.gt86dash

import android.app.Activity
import android.content.Context
import android.content.res.ColorStateList
import android.graphics.Canvas
import android.graphics.Rect
import android.os.Bundle
import android.os.SystemClock
import android.util.TypedValue
import android.view.Gravity
import android.view.View
import android.view.WindowInsets
import android.view.ViewGroup.LayoutParams.MATCH_PARENT
import android.view.ViewGroup.LayoutParams.WRAP_CONTENT
import android.widget.FrameLayout
import android.widget.ImageButton
import android.widget.LinearLayout
import android.widget.Switch
import android.widget.TextView
import android.widget.Toast

// The dash on the phone, full screen in landscape: arrows on either side
// turn the views, the cog opens the settings panel on the right. It asks
// for Bluetooth's permissions, for the dash's readings. Started
// with a scene (adb shell am start -n com.zedensix.gt86dash/.PreviewActivity
// --es scene track --ei view 1), it holds that scene's fixed readings, for
// screenshots; --ez settings true opens the panel.
class PreviewActivity : Activity() {
    private lateinit var settings: Settings
    private lateinit var panel: View
    private lateinit var shiftValue: TextView

    private fun dp(v: Int): Int = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, v.toFloat(), resources.displayMetrics).toInt()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        settings = Settings(this)
        val dash = DashView(this, intent.getStringExtra("scene"))
        repeat(intent.getIntExtra("view", 0)) { dash.dash.nextView() }
        val root = FrameLayout(this).apply { setBackgroundColor(Colors.BACKGROUND) }
        // Everything inside the safe area: clear of the camera's cutout and
        // the system bars, whichever side the phone is turned to.
        root.setOnApplyWindowInsetsListener { v, insets ->
            val safe = insets.getInsets(WindowInsets.Type.systemBars() or WindowInsets.Type.displayCutout())
            v.setPadding(safe.left, safe.top, safe.right, safe.bottom)
            WindowInsets.CONSUMED
        }
        root.addView(dash)
        root.addView(button(R.drawable.ic_previous, 56) { dash.dash.previousView() }, at(56, Gravity.START or Gravity.CENTER_VERTICAL))
        root.addView(button(R.drawable.ic_next, 56) { dash.dash.nextView() }, at(56, Gravity.END or Gravity.CENTER_VERTICAL))
        root.addView(button(R.drawable.ic_settings, 48) { showPanel(true) }, at(48, Gravity.END or Gravity.TOP))
        panel = settingsPanel()
        root.addView(panel, FrameLayout.LayoutParams(MATCH_PARENT, MATCH_PARENT))
        showPanel(intent.getBooleanExtra("settings", false))
        setContentView(root)
        if (!LiveLink.granted(this)) requestPermissions(LiveLink.permissions, 0)
    }

    override fun onStart() {
        super.onStart()
        LiveLink.start(this)
    }

    override fun onStop() {
        LiveLink.stop()
        super.onStop()
    }

    private fun at(size: Int, gravity: Int) =
        FrameLayout.LayoutParams(dp(size), dp(size), gravity).apply { setMargins(dp(16), dp(16), dp(16), dp(16)) }

    private fun button(icon: Int, size: Int, onClick: () -> Unit) =
        ImageButton(this).apply {
            setImageResource(icon)
            setColorFilter(0xFFBDBDBD.toInt())
            setBackgroundResource(R.drawable.round_button)
            val pad = dp(size) / 5
            setPadding(pad, pad, pad, pad)
            scaleType = android.widget.ImageView.ScaleType.FIT_CENTER
            setOnClickListener { onClick() }
        }

    private fun text(s: String, size: Float, weight: Int, color: Int, spacing: Float = 0f) =
        TextView(this).apply {
            text = s
            textSize = size
            typeface = resources.getFont(weight)
            setTextColor(color)
            letterSpacing = spacing
        }

    private fun showPanel(show: Boolean) {
        panel.visibility = if (show) View.VISIBLE else View.GONE
    }

    // A dim veil over the gauges (a tap on it closes the panel), the panel
    // on the right: a title and its close button, then each setting.
    private fun settingsPanel(): View {
        val veil = FrameLayout(this).apply {
            setBackgroundColor(0x99000000.toInt())
            setOnClickListener { showPanel(false) }
        }
        val column = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundResource(R.drawable.panel)
            setPadding(dp(24), dp(16), dp(24), dp(16))
            isClickable = true
        }
        veil.addView(column, FrameLayout.LayoutParams(dp(340), MATCH_PARENT, Gravity.END))

        val header = LinearLayout(this).apply { gravity = Gravity.CENTER_VERTICAL }
        header.addView(text("SETTINGS", 22f, R.font.barlow_condensed_bold, Colors.ACCENT, 0.15f), LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f))
        header.addView(button(R.drawable.ic_close, 40) { showPanel(false) }, LinearLayout.LayoutParams(dp(40), dp(40)))
        column.addView(header)

        column.addView(label("SHIFT LIGHT"))
        val shift = LinearLayout(this).apply { gravity = Gravity.CENTER_VERTICAL }
        shiftValue = text("", 30f, R.font.barlow_condensed_bold, Colors.LABEL)
        shift.addView(button(R.drawable.ic_remove, 44) { stepShift(-1) }, LinearLayout.LayoutParams(dp(44), dp(44)))
        shift.addView(shiftValue, LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f).apply { shiftValue.gravity = Gravity.CENTER })
        shift.addView(button(R.drawable.ic_add, 44) { stepShift(1) }, LinearLayout.LayoutParams(dp(44), dp(44)))
        column.addView(shift)
        column.addView(text("The LEDs light over the last 1500 rpm before it, all blue from it", 15f, R.font.barlow_condensed_medium, Colors.DIM))
        stepShift(0)

        column.addView(label("DEMO"))
        val demo = Switch(this).apply {
            text = "Made-up readings"
            textSize = 20f
            typeface = resources.getFont(R.font.barlow_condensed_semibold)
            setTextColor(Colors.LABEL)
            isChecked = settings.demo
            val states = arrayOf(intArrayOf(android.R.attr.state_checked), intArrayOf())
            thumbTintList = ColorStateList(states, intArrayOf(Colors.ACCENT, 0xFFBDBDBD.toInt()))
            trackTintList = ColorStateList(states, intArrayOf(Colors.FRAME, 0xFF333333.toInt()))
            setOnCheckedChangeListener { _, on -> settings.demo = on }
        }
        column.addView(demo, LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT))

        column.addView(label("MIN AND MAX"))
        val reset = text("Reset every gauge's", 20f, R.font.barlow_condensed_semibold, Colors.LABEL).apply {
            setBackgroundResource(R.drawable.pill_button)
            gravity = Gravity.CENTER
            setPadding(dp(16), dp(10), dp(16), dp(10))
            setOnClickListener {
                Extremes.reset()
                Toast.makeText(this@PreviewActivity, "Min and max reset", Toast.LENGTH_SHORT).show()
            }
        }
        column.addView(reset, LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT))
        return veil
    }

    private fun label(s: String) =
        text(s, 15f, R.font.barlow_condensed_semibold, Colors.DIM, 0.15f).apply { setPadding(0, dp(24), 0, dp(8)) }

    private fun stepShift(steps: Int) {
        if (steps != 0) settings.stepShiftRpm(steps)
        shiftValue.text = settings.shiftRpmText()
    }
}

private class DashView(context: Context, private val scene: String?) : View(context) {
    val dash = Dash(context)
    private val area = Rect()
    private val started = SystemClock.uptimeMillis()

    override fun onDraw(canvas: Canvas) {
        val now = SystemClock.uptimeMillis() - started
        area.set(0, 0, width, height)
        if (scene != null) dash.draw(canvas, area, FakeSource.scene(scene), 0) else dash.frame(canvas, area, now)
        postInvalidateOnAnimation()
    }
}
