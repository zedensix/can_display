package com.zedensix.gt86dash

import android.os.Bundle
import android.view.ViewGroup
import androidx.activity.ComponentActivity
import androidx.activity.compose.BackHandler
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts.RequestMultiplePermissions
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Icon
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationRail
import androidx.compose.material3.NavigationRailItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalView
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.viewinterop.AndroidView
import kotlinx.coroutines.delay

// The dash on the phone, full screen, turned as the phone is (portrait
// unless it's turned; the manifest's configChanges keep the activity, and
// the link, through the turn), a swipe across it turning the views as the
// Pico's encoder does. Material 3 around it (Theme.kt): a navigation bar,
// a rail in landscape, Home (back to the gauges; there, the main view),
// Laps, Map and Settings, each page in the dash's place, Back closing it.
// A firmware update starting opens its page (UpdatePage) over whatever
// showed, the screen kept on while it runs. It asks for Bluetooth's
// permissions, for the dash's readings. Started with a scene (adb shell am
// start -n com.zedensix.gt86dash/.PreviewActivity --es scene track --ei
// view 1), it holds that scene's fixed readings, for screenshots; --ez
// settings true opens the settings.
class PreviewActivity : ComponentActivity() {
    private lateinit var dash: DashView
    private lateinit var picker: DashPicker

    private val askBluetooth = registerForActivityResult(RequestMultiplePermissions()) {}

    enum class Tab(val label: String, val icon: Int) {
        HOME("Home", R.drawable.ic_home),
        LAPS("Laps", R.drawable.ic_laps),
        MAP("Map", R.drawable.ic_map),
        SETTINGS("Settings", R.drawable.ic_settings),
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        dash = DashView(this, intent.getStringExtra("scene"))
        repeat(intent.getIntExtra("view", 0)) { dash.dash.nextView() }
        picker = DashPicker(this, Settings(this))
        val start = if (intent.getBooleanExtra("settings", false)) Tab.SETTINGS else Tab.HOME
        setContent { DashTheme { App(start) } }
        if (!LiveLink.granted(this)) askBluetooth.launch(LiveLink.permissions)
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

    @Composable
    private fun App(start: Tab) {
        var tab by rememberSaveable { mutableStateOf(start) }
        var updating by remember { mutableStateOf(false) }
        val context = LocalContext.current
        val askGps = rememberLauncherForActivityResult(RequestMultiplePermissions()) { Recorder.update() }
        val wantGps = { context.wantGps { askGps.launch(it) } }

        // An update starting opens its page; the screen stays on through it.
        val view = LocalView.current
        LaunchedEffect(Unit) {
            var was = false
            while (true) {
                val active = FirmwareUpdate.active
                if (active && !was) updating = true
                was = active
                view.keepScreenOn = active
                delay(WATCH_MS)
            }
        }
        BackHandler(enabled = updating || tab != Tab.HOME) {
            if (updating) updating = false else tab = Tab.HOME
        }
        fun select(t: Tab) {
            if (t == Tab.HOME && tab == Tab.HOME && !updating) dash.dash.home()
            updating = false
            tab = t
        }

        val wide = landscape()
        Scaffold(
            bottomBar = {
                if (!wide) {
                    NavigationBar {
                        Tab.entries.forEach { t ->
                            NavigationBarItem(
                                selected = !updating && tab == t,
                                onClick = { select(t) },
                                icon = { Icon(painterResource(t.icon), null) },
                                label = { Text(t.label) },
                            )
                        }
                    }
                }
            },
        ) { padding ->
            Row(Modifier.padding(padding).fillMaxSize()) {
                if (wide) {
                    NavigationRail {
                        Spacer(Modifier.weight(1f))
                        Tab.entries.forEach { t ->
                            NavigationRailItem(
                                selected = !updating && tab == t,
                                onClick = { select(t) },
                                icon = { Icon(painterResource(t.icon), null) },
                                label = { Text(t.label) },
                            )
                        }
                        Spacer(Modifier.weight(1f))
                    }
                }
                Box(Modifier.weight(1f).fillMaxHeight()) {
                    when {
                        updating -> UpdatePage { updating = false }
                        tab == Tab.HOME -> Gauges()
                        tab == Tab.LAPS -> LapsPage(wantGps)
                        tab == Tab.MAP -> DrivesPage(wantGps)
                        else -> SettingsPage(picker)
                    }
                }
            }
        }
    }

    // The gauges' own View (Dash draws them as on the car), kept across
    // pages so its view stays as it was turned.
    @Composable
    private fun Gauges() =
        AndroidView(
            factory = {
                (dash.parent as? ViewGroup)?.removeView(dash)
                dash
            },
            modifier = Modifier.fillMaxSize(),
        )

    private companion object {
        const val WATCH_MS = 250L
    }
}
