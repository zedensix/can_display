package com.zedensix.gt86dash

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.content.res.Configuration
import android.graphics.Canvas
import android.graphics.Rect
import android.os.Bundle
import android.os.SystemClock
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.ViewGroup.LayoutParams.MATCH_PARENT
import android.view.WindowInsets
import android.widget.FrameLayout
import android.widget.ImageButton
import android.widget.ImageView
import android.widget.LinearLayout

// The dash on the phone, full screen, turned as the phone is (portrait
// unless it's turned): its buttons in a bar of their own beside it, so
// none covers a reading: under it in portrait, on either side in
// landscape. The arrows turn the views, shown with the gauges only; the
// cog opens the settings page. It asks for Bluetooth's permissions, for
// the dash's readings. Started with a scene (adb shell am start -n
// com.zedensix.gt86dash/.PreviewActivity --es scene track --ei view 1), it
// holds that scene's fixed readings, for screenshots; --ez settings true
// opens the settings page.
class PreviewActivity : Activity() {
    private lateinit var root: FrameLayout
    private lateinit var dash: DashView
    private lateinit var previous: View
    private lateinit var next: View
    private lateinit var cog: View

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        dash = DashView(this, intent.getStringExtra("scene")) { show(it) }
        repeat(intent.getIntExtra("view", 0)) { dash.dash.nextView() }
        previous = iconButton(this, R.drawable.ic_previous, BUTTON) { dash.dash.previousView() }
        next = iconButton(this, R.drawable.ic_next, BUTTON) { dash.dash.nextView() }
        cog = iconButton(this, R.drawable.ic_settings, BUTTON) { openSettings() }
        root = FrameLayout(this).apply { setBackgroundColor(Colors.BACKGROUND) }
        safeArea(root)
        layOut()
        setContentView(root)
        if (intent.getBooleanExtra("settings", false)) openSettings()
        if (!LiveLink.granted(this)) requestPermissions(LiveLink.permissions, 0)
    }

    // Turned, it lays itself out again; the manifest keeps the activity
    // through the turn (configChanges): recreated, it would drop the link.
    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        layOut()
    }

    override fun onStart() {
        super.onStart()
        LiveLink.start(this)
    }

    override fun onStop() {
        LiveLink.stop()
        super.onStop()
    }

    private fun openSettings() = startActivity(Intent(this, SettingsActivity::class.java))

    private fun show(gauges: Boolean) {
        val arrows = if (gauges) View.VISIBLE else View.INVISIBLE
        previous.visibility = arrows
        next.visibility = arrows
    }

    // Portrait: the dash, then a bar under it, an arrow at each end and the
    // cog between. Landscape: a column each side, the previous arrow on
    // the left, the cog over the next one on the right.
    private fun layOut() {
        listOf(dash, previous, next, cog).forEach { (it.parent as? ViewGroup)?.removeView(it) }
        root.removeAllViews()
        val bar = dp(this, BUTTON + 2 * MARGIN)
        val landscape = resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
        val frame = LinearLayout(this).apply { orientation = if (landscape) LinearLayout.HORIZONTAL else LinearLayout.VERTICAL }
        if (landscape) {
            frame.addView(column(previous to Gravity.CENTER), LinearLayout.LayoutParams(bar, MATCH_PARENT))
            frame.addView(dash, LinearLayout.LayoutParams(0, MATCH_PARENT, 1f))
            frame.addView(column(cog to (Gravity.TOP or Gravity.CENTER_HORIZONTAL), next to Gravity.CENTER), LinearLayout.LayoutParams(bar, MATCH_PARENT))
        } else {
            frame.addView(dash, LinearLayout.LayoutParams(MATCH_PARENT, 0, 1f))
            frame.addView(
                column(previous to (Gravity.START or Gravity.CENTER_VERTICAL), cog to Gravity.CENTER, next to (Gravity.END or Gravity.CENTER_VERTICAL)),
                LinearLayout.LayoutParams(MATCH_PARENT, bar),
            )
        }
        root.addView(frame, FrameLayout.LayoutParams(MATCH_PARENT, MATCH_PARENT))
    }

    private fun column(vararg buttons: Pair<View, Int>) =
        FrameLayout(this).apply {
            val m = dp(this@PreviewActivity, MARGIN)
            buttons.forEach { (b, gravity) ->
                addView(b, FrameLayout.LayoutParams(dp(context, BUTTON), dp(context, BUTTON), gravity).apply { setMargins(m, m, m, m) })
            }
        }

    private companion object {
        const val BUTTON = 52
        const val MARGIN = 10
    }
}

private class DashView(context: Context, private val scene: String?, private val drew: (gauges: Boolean) -> Unit) :
    View(context) {
    val dash = Dash(context)
    private val area = Rect()
    private val started = SystemClock.uptimeMillis()

    override fun onDraw(canvas: Canvas) {
        val now = SystemClock.uptimeMillis() - started
        area.set(0, 0, width, height)
        if (scene != null) {
            dash.draw(canvas, area, FakeSource.scene(scene), 0)
            drew(true)
        } else {
            drew(dash.frame(canvas, area, now))
        }
        postInvalidateOnAnimation()
    }
}

// What the phone's screens share: everything inside the safe area, clear
// of the camera's cutout and the system bars, whichever side the phone is
// turned to; a round icon button.
fun safeArea(root: View) {
    root.setOnApplyWindowInsetsListener { v, insets ->
        val safe = insets.getInsets(WindowInsets.Type.systemBars() or WindowInsets.Type.displayCutout())
        v.setPadding(safe.left, safe.top, safe.right, safe.bottom)
        WindowInsets.CONSUMED
    }
}

fun dp(c: Context, v: Int): Int = (v * c.resources.displayMetrics.density).toInt()

fun iconButton(c: Context, icon: Int, size: Int, onClick: () -> Unit) =
    ImageButton(c).apply {
        setImageResource(icon)
        setColorFilter(0xFFBDBDBD.toInt())
        setBackgroundResource(R.drawable.round_button)
        val pad = dp(c, size) / 5
        setPadding(pad, pad, pad, pad)
        scaleType = ImageView.ScaleType.FIT_CENTER
        setOnClickListener { onClick() }
    }
