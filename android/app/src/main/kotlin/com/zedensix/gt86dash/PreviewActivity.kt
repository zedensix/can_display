package com.zedensix.gt86dash

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.content.pm.ActivityInfo
import android.graphics.Canvas
import android.graphics.Rect
import android.os.Bundle
import android.os.SystemClock
import android.view.Gravity
import android.view.View
import android.view.WindowInsets
import android.widget.FrameLayout
import android.widget.ImageButton
import android.widget.ImageView

// The dash on the phone, full screen: the gauges in landscape, arrows on
// either side turning the views; the waiting screen in portrait, until the
// dash's readings come (or demo's). The cog opens the settings page. It
// asks for Bluetooth's permissions, for the dash's readings. Started with
// a scene (adb shell am start -n com.zedensix.gt86dash/.PreviewActivity
// --es scene track --ei view 1), it holds that scene's fixed readings, for
// screenshots; --ez settings true opens the settings page.
class PreviewActivity : Activity() {
    private lateinit var arrows: List<View>
    private var gauges: Boolean? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val scene = intent.getStringExtra("scene")
        val dash = DashView(this, scene) { show(it) }
        repeat(intent.getIntExtra("view", 0)) { dash.dash.nextView() }
        val root = FrameLayout(this).apply { setBackgroundColor(Colors.BACKGROUND) }
        safeArea(root)
        root.addView(dash)
        arrows = listOf(
            iconButton(this, R.drawable.ic_previous, 56) { dash.dash.previousView() },
            iconButton(this, R.drawable.ic_next, 56) { dash.dash.nextView() },
        )
        root.addView(arrows[0], at(56, Gravity.START or Gravity.CENTER_VERTICAL))
        root.addView(arrows[1], at(56, Gravity.END or Gravity.CENTER_VERTICAL))
        root.addView(iconButton(this, R.drawable.ic_settings, 48) { openSettings() }, at(48, Gravity.END or Gravity.TOP))
        setContentView(root)
        // The first frame's orientation, so it doesn't turn once drawn.
        show(scene != null || Settings(this).demo || LiveLink.readings() != null)
        if (intent.getBooleanExtra("settings", false)) openSettings()
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

    private fun openSettings() = startActivity(Intent(this, SettingsActivity::class.java))

    // The gauges in landscape with their arrows, the waiting screen in
    // portrait without. The manifest keeps the activity through the turn
    // (configChanges): recreated, it would drop the link and wait again.
    private fun show(gauges: Boolean) {
        if (gauges == this.gauges) return
        this.gauges = gauges
        requestedOrientation =
            if (gauges) ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE else ActivityInfo.SCREEN_ORIENTATION_PORTRAIT
        arrows.forEach { it.visibility = if (gauges) View.VISIBLE else View.GONE }
    }

    private fun at(size: Int, gravity: Int) =
        FrameLayout.LayoutParams(dp(this, size), dp(this, size), gravity).apply {
            val m = dp(this@PreviewActivity, 16)
            setMargins(m, m, m, m)
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
