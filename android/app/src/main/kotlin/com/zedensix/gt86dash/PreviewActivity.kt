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
// unless it's turned), a swipe across it turning the views as the Pico's
// encoder does; the lap timing, the map or the settings page in its place
// (a Page; Back closes it). Its buttons, clear of every reading: in
// portrait a navigation bar floating as glass along the bottom, Home
// (back to the gauges; there, the main view), Laps, Map, Settings, the
// one shown lit; in landscape a column each side, Home over Laps on the
// left, Settings over Map on the right. It asks for Bluetooth's
// permissions, for the dash's readings. Started with a scene (adb shell am
// start -n com.zedensix.gt86dash/.PreviewActivity --es scene track --ei
// view 1), it holds that scene's fixed readings, for screenshots; --ez
// settings true opens the settings.
class PreviewActivity : Activity() {
    private lateinit var root: FrameLayout
    private lateinit var dash: DashView
    private lateinit var settingsPage: SettingsPage
    private lateinit var drivesPage: DrivesPage
    private lateinit var lapsPage: LapsPage

    // The page shown in the dash's place, and its view.
    private var page: Page? = null
    private var pageView: View? = null
    private val back = OnBackInvokedCallback { closePage() }

    // The icons and labels of the layout in use, each lit while its page
    // shows (null: the gauges).
    private var lit = listOf<Pair<Page?, View>>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        dash = DashView(this, intent.getStringExtra("scene"))
        repeat(intent.getIntExtra("view", 0)) { dash.dash.nextView() }
        settingsPage = SettingsPage(this) { closePage() }
        drivesPage = DrivesPage(this) { closePage() }
        lapsPage = LapsPage(this) { closePage() }
        root = FrameLayout(this).apply { setBackgroundColor(Colors.BACKGROUND) }
        safeArea(root)
        setContentView(root)
        if (intent.getBooleanExtra("settings", false)) openPage(settingsPage) else layOut()
        if (!LiveLink.granted(this)) requestPermissions(LiveLink.permissions, 0)
    }

    // Turned, it lays itself out again; the manifest keeps the activity
    // through the turn (configChanges): recreated, it would drop the link.
    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        page?.let { pageView = it.build(barRoom()) }
        layOut()
    }

    override fun onStart() {
        super.onStart()
        LiveLink.start(this)
        Recorder.hold(this)
    }

    override fun onStop() {
        LiveLink.stop()
        Recorder.release()
        super.onStop()
    }

    override fun onResume() {
        super.onResume()
        page?.resume()
    }

    override fun onPause() {
        page?.pause()
        super.onPause()
    }

    override fun onDestroy() {
        drivesPage.destroy()
        super.onDestroy()
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String>, results: IntArray) {
        if (requestCode == Recorder.LOCATION_REQUEST) Recorder.update() else if (page === settingsPage) settingsPage.permissionsAnswered()
    }

    // The folder for the backups, picked.
    @Deprecated("Activity's own: no AndroidX here")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        if (requestCode == RecordingSettings.FOLDER_REQUEST && resultCode == RESULT_OK) settingsPage.folderPicked(data?.data)
    }

    private fun portrait() = resources.configuration.orientation != Configuration.ORIENTATION_LANDSCAPE

    // What the floating bar takes off the bottom in portrait.
    private fun barRoom() = if (portrait()) dp(this, BAR + 2 * BAR_MARGIN) else 0

    // A page in the dash's place.
    private fun openPage(p: Page) {
        if (page === p) return
        val shown = page
        if (shown == null) onBackInvokedDispatcher.registerOnBackInvokedCallback(OnBackInvokedDispatcher.PRIORITY_DEFAULT, back) else shown.pause()
        page = p
        pageView = p.build(barRoom())
        p.resume()
        layOut()
    }

    private fun closePage() {
        val p = page ?: return
        page = null
        pageView = null
        p.pause()
        onBackInvokedDispatcher.unregisterOnBackInvokedCallback(back)
        layOut()
    }

    // Home: back to the gauges, or there to the main view.
    private fun home() {
        if (page != null) closePage() else dash.dash.home()
    }

    private fun update() {
        lit.forEach { (p, v) -> tint(v, if (page === p) Colors.ACCENT else ICON) }
    }

    private fun tint(v: View, color: Int) {
        if (v is ImageView) v.setColorFilter(color) else if (v is TextView) v.setTextColor(color)
    }

    private fun layOut() {
        listOf<View?>(dash, pageView).forEach { v -> (v?.parent as? ViewGroup)?.removeView(v) }
        root.removeAllViews()
        val content = pageView ?: dash
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
        val home = iconButton(this, R.drawable.ic_home, BUTTON) { home() }
        val laps = iconButton(this, R.drawable.ic_laps, BUTTON) { openPage(lapsPage) }
        val map = iconButton(this, R.drawable.ic_map, BUTTON) { openPage(drivesPage) }
        val cog = iconButton(this, R.drawable.ic_settings, BUTTON) { openPage(settingsPage) }
        lit = listOf(null to home, lapsPage to laps, drivesPage to map, settingsPage to cog)
        val top = Gravity.TOP or Gravity.CENTER_HORIZONTAL
        frame.addView(column(home to top, laps to Gravity.CENTER), LinearLayout.LayoutParams(side, MATCH_PARENT))
        frame.addView(content, LinearLayout.LayoutParams(0, MATCH_PARENT, 1f))
        frame.addView(column(cog to top, map to Gravity.CENTER), LinearLayout.LayoutParams(side, MATCH_PARENT))
        return frame
    }

    // Four items across the floating bar, each an icon over its label.
    private fun navigationBar(): View {
        val bar = LinearLayout(this).apply {
            gravity = Gravity.CENTER_VERTICAL
            setPadding(dp(context, 6), 0, dp(context, 6), 0)
        }
        val items = mutableListOf<Pair<Page?, View>>()
        fun item(page: Page?, icon: Int, label: String, onClick: () -> Unit) {
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
            items += listOf(page to image, page to text)
        }
        item(null, R.drawable.ic_home, "Home") { home() }
        item(lapsPage, R.drawable.ic_laps, "Laps") { openPage(lapsPage) }
        item(drivesPage, R.drawable.ic_map, "Map") { openPage(drivesPage) }
        item(settingsPage, R.drawable.ic_settings, "Settings") { openPage(settingsPage) }
        lit = items
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
