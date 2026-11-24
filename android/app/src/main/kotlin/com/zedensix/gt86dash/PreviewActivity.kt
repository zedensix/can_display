package com.zedensix.gt86dash

import android.app.Activity
import android.content.Context
import android.content.res.Configuration
import android.graphics.Canvas
import android.graphics.Rect
import android.os.Bundle
import android.os.SystemClock
import android.view.Gravity
import android.view.RoundedCorner
import android.view.View
import android.view.ViewGroup
import android.view.ViewGroup.LayoutParams.MATCH_PARENT
import android.view.ViewGroup.LayoutParams.WRAP_CONTENT
import android.view.WindowInsets
import android.widget.FrameLayout
import android.widget.ImageButton
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.TextView
import android.window.OnBackInvokedCallback
import android.window.OnBackInvokedDispatcher

// The dash on the phone, full screen, turned as the phone is (portrait
// unless it's turned), its buttons clear of every reading; the settings
// page in its place on Settings, the buttons still there (Back closes
// it). Portrait: a navigation bar floating as glass along the bottom,
// Home (the main view), the previous and the next view, Settings, the
// page shown lit. Landscape: a column each side, the previous arrow on
// the left with Home over it off the main view, the cog over the next
// one on the right. The views' buttons work with the gauges only, and go
// back to them from the settings. It asks for Bluetooth's permissions,
// for the dash's readings. Started with a scene (adb shell am start -n
// com.zedensix.gt86dash/.PreviewActivity --es scene track --ei view 1), it
// holds that scene's fixed readings, for screenshots; --ez settings true
// opens the settings.
class PreviewActivity : Activity() {
    private lateinit var root: FrameLayout
    private lateinit var dash: DashView
    private lateinit var settingsPage: SettingsPage
    private var settings: View? = null
    private var gauges = false
    private val back = OnBackInvokedCallback { closeSettings() }

    // The buttons of the layout in use: those that turn the views, Home,
    // and the icons and labels lit for the page shown.
    private var views = listOf<View>()
    private var home: View? = null
    private var homeLit = listOf<View>()
    private var settingsLit = listOf<View>()
    private var navigation = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        dash = DashView(this, intent.getStringExtra("scene")) { show(it) }
        repeat(intent.getIntExtra("view", 0)) { dash.dash.nextView() }
        settingsPage = SettingsPage(this) { closeSettings() }
        root = FrameLayout(this).apply { setBackgroundColor(Colors.BACKGROUND) }
        safeArea(root)
        setContentView(root)
        if (intent.getBooleanExtra("settings", false)) openSettings() else layOut()
        if (!LiveLink.granted(this)) requestPermissions(LiveLink.permissions, 0)
    }

    // Turned, it lays itself out again; the manifest keeps the activity
    // through the turn (configChanges): recreated, it would drop the link.
    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        if (settings != null) settings = settingsPage.build(barRoom())
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

    override fun onResume() {
        super.onResume()
        if (settings != null) settingsPage.resume()
    }

    override fun onPause() {
        settingsPage.pause()
        super.onPause()
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String>, results: IntArray) {
        if (settings != null) settingsPage.permissionsAnswered()
    }

    private fun portrait() = resources.configuration.orientation != Configuration.ORIENTATION_LANDSCAPE

    // What the floating bar takes off the bottom in portrait.
    private fun barRoom() = if (portrait()) dp(this, BAR + 2 * BAR_MARGIN) else 0

    private fun openSettings() {
        if (settings != null) return
        settings = settingsPage.build(barRoom())
        settingsPage.resume()
        onBackInvokedDispatcher.registerOnBackInvokedCallback(OnBackInvokedDispatcher.PRIORITY_DEFAULT, back)
        layOut()
    }

    private fun closeSettings() {
        if (settings == null) return
        settings = null
        settingsPage.pause()
        onBackInvokedDispatcher.unregisterOnBackInvokedCallback(back)
        layOut()
    }

    private fun show(gauges: Boolean) {
        if (gauges == this.gauges) return
        this.gauges = gauges
        update()
    }

    // A view's button: back to the gauges, then that.
    private fun turn(action: () -> Unit) {
        closeSettings()
        action()
        update()
    }

    // What the buttons show for the page, the view and the readings.
    private fun update() {
        val main = dash.dash.view == 0 && settings == null
        views.forEach {
            it.isEnabled = gauges
            it.alpha = if (gauges) 1f else 0.35f
        }
        if (!navigation) home?.visibility = if (main) View.INVISIBLE else View.VISIBLE
        homeLit.forEach { tint(it, if (main && gauges) Colors.ACCENT else ICON) }
        settingsLit.forEach { tint(it, if (settings != null) Colors.ACCENT else ICON) }
    }

    private fun tint(v: View, color: Int) {
        if (v is ImageView) v.setColorFilter(color) else if (v is TextView) v.setTextColor(color)
    }

    private fun layOut() {
        listOf<View?>(dash, settings).forEach { v -> (v?.parent as? ViewGroup)?.removeView(v) }
        root.removeAllViews()
        val content = settings ?: dash
        if (portrait()) {
            val frame = GlassFrame(this)
            val bar = navigationBar()
            frame.addView(content, FrameLayout.LayoutParams(MATCH_PARENT, MATCH_PARENT).apply { bottomMargin = if (content === dash) barRoom() else 0 })
            frame.addView(
                bar,
                FrameLayout.LayoutParams(MATCH_PARENT, dp(this, BAR), Gravity.BOTTOM).apply {
                    val m = dp(this@PreviewActivity, BAR_MARGIN)
                    setMargins(m + dp(this@PreviewActivity, 8), m, m + dp(this@PreviewActivity, 8), m)
                },
            )
            frame.bar = bar
            root.addView(frame, FrameLayout.LayoutParams(MATCH_PARENT, MATCH_PARENT))
        } else {
            root.addView(columns(content), FrameLayout.LayoutParams(MATCH_PARENT, MATCH_PARENT))
        }
        update()
    }

    private fun columns(content: View): View {
        val frame = LinearLayout(this)
        val side = dp(this, BUTTON + 2 * MARGIN)
        val previous = iconButton(this, R.drawable.ic_previous, BUTTON) { turn { dash.dash.previousView() } }
        val next = iconButton(this, R.drawable.ic_next, BUTTON) { turn { dash.dash.nextView() } }
        val homeButton = iconButton(this, R.drawable.ic_home, BUTTON) { turn { dash.dash.home() } }
        val cog = iconButton(this, R.drawable.ic_settings, BUTTON) { if (settings == null) openSettings() else closeSettings() }
        views = listOf(previous, next, homeButton)
        home = homeButton
        homeLit = listOf(homeButton)
        settingsLit = listOf(cog)
        navigation = false
        frame.addView(column(homeButton to (Gravity.TOP or Gravity.CENTER_HORIZONTAL), previous to Gravity.CENTER), LinearLayout.LayoutParams(side, MATCH_PARENT))
        frame.addView(content, LinearLayout.LayoutParams(0, MATCH_PARENT, 1f))
        frame.addView(column(cog to (Gravity.TOP or Gravity.CENTER_HORIZONTAL), next to Gravity.CENTER), LinearLayout.LayoutParams(side, MATCH_PARENT))
        return frame
    }

    // Four items across the floating bar, each an icon over its label.
    private fun navigationBar(): View {
        val bar = LinearLayout(this).apply {
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(context, 6), 0, dp(context, 6), 0)
        }
        fun item(icon: Int, label: String, onClick: () -> Unit): List<View> {
            val image = ImageView(this).apply {
                setImageResource(icon)
                setColorFilter(ICON)
            }
            val text = pageText(this, label, 12f, R.font.barlow_condensed_semibold, ICON).apply { gravity = Gravity.CENTER }
            val item = LinearLayout(this).apply {
                orientation = LinearLayout.VERTICAL
                gravity = Gravity.CENTER
                setBackgroundResource(R.drawable.navigation_item)
                setOnClickListener { onClick() }
                addView(image, LinearLayout.LayoutParams(dp(context, 24), dp(context, 24)))
                addView(text)
            }
            bar.addView(item, LinearLayout.LayoutParams(0, MATCH_PARENT, 1f))
            return listOf(item, image, text)
        }
        val homeItem = item(R.drawable.ic_home, "Home") { turn { dash.dash.home() } }
        val previous = item(R.drawable.ic_previous, "Previous") { turn { dash.dash.previousView() } }
        val next = item(R.drawable.ic_next, "Next") { turn { dash.dash.nextView() } }
        val settingsItem = item(R.drawable.ic_settings, "Settings") { openSettings() }
        views = listOf(previous[0], next[0])
        home = homeItem[0]
        homeLit = homeItem.drop(1)
        settingsLit = settingsItem.drop(1)
        navigation = true
        return bar
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
        const val BAR = 64
        const val BAR_MARGIN = 12
        const val ICON = 0xFFE6E6E6.toInt()
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
// of the camera's cutout, the system bars and the screen's rounded
// corners, whichever side the phone is turned to; a round icon button.
fun safeArea(root: View) {
    root.setOnApplyWindowInsetsListener { v, insets ->
        val safe = insets.getInsets(WindowInsets.Type.systemBars() or WindowInsets.Type.displayCutout())
        // A corner of radius r cuts into a square at it up to 0.3 r from
        // both edges: each side clear of its two corners' cut.
        fun cut(a: Int, b: Int) = (CORNER_CUT * maxOf(insets.getRoundedCorner(a)?.radius ?: 0, insets.getRoundedCorner(b)?.radius ?: 0)).toInt()
        v.setPadding(
            maxOf(safe.left, cut(RoundedCorner.POSITION_TOP_LEFT, RoundedCorner.POSITION_BOTTOM_LEFT)),
            maxOf(safe.top, cut(RoundedCorner.POSITION_TOP_LEFT, RoundedCorner.POSITION_TOP_RIGHT)),
            maxOf(safe.right, cut(RoundedCorner.POSITION_TOP_RIGHT, RoundedCorner.POSITION_BOTTOM_RIGHT)),
            maxOf(safe.bottom, cut(RoundedCorner.POSITION_BOTTOM_LEFT, RoundedCorner.POSITION_BOTTOM_RIGHT)),
        )
        WindowInsets.CONSUMED
    }
}

private const val CORNER_CUT = 0.3f

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
