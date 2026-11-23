package com.zedensix.gt86dash

import android.content.Intent
import android.content.pm.ApplicationInfo
import androidx.car.app.CarAppService
import androidx.car.app.CarContext
import androidx.car.app.Screen
import androidx.car.app.Session
import androidx.car.app.model.Action
import androidx.car.app.model.ActionStrip
import androidx.car.app.model.CarIcon
import androidx.car.app.model.Template
import androidx.car.app.navigation.model.NavigationTemplate
import androidx.car.app.validation.HostValidator
import androidx.core.graphics.drawable.IconCompat

// What Android Auto starts: a navigation app whose "map" is the gauges.
class DashService : CarAppService() {
    // A debug build answers any host (the desktop head unit); a release
    // one only Google's (Android Auto).
    override fun createHostValidator(): HostValidator =
        if (applicationInfo.flags and ApplicationInfo.FLAG_DEBUGGABLE != 0) {
            HostValidator.ALLOW_ALL_HOSTS_VALIDATOR
        } else {
            HostValidator.Builder(applicationContext)
                .addAllowedHosts(androidx.car.app.R.array.hosts_allowlist_sample)
                .build()
        }

    override fun onCreateSession(): Session = DashSession()
}

class DashSession : Session() {
    override fun onCreateScreen(intent: Intent): Screen = GaugeScreen(carContext)
}

// The gauges drawn on the surface, buttons over them: the previous and the
// next view, as the Pico's encoder turns them, and the settings.
class GaugeScreen(carContext: CarContext) : Screen(carContext) {
    private val renderer = SurfaceRenderer(carContext, lifecycle)

    init {
        // Bluetooth's permissions, asked on the phone (the car's screen
        // says to look there meanwhile).
        if (!LiveLink.granted(carContext)) {
            carContext.requestPermissions(LiveLink.permissions.toList()) { _, _ -> LiveLink.retry() }
        }
    }

    override fun onGetTemplate(): Template {
        val strip = ActionStrip.Builder()
            .addAction(iconAction(carContext, R.drawable.ic_previous) { renderer.dash.previousView() })
            .addAction(iconAction(carContext, R.drawable.ic_next) { renderer.dash.nextView() })
            .addAction(iconAction(carContext, R.drawable.ic_settings) { screenManager.push(SettingsScreen(carContext)) })
            .build()
        return NavigationTemplate.Builder().setActionStrip(strip).build()
    }
}

fun iconAction(context: CarContext, icon: Int, onClick: () -> Unit): Action =
    Action.Builder()
        .setIcon(CarIcon.Builder(IconCompat.createWithResource(context, icon)).build())
        .setOnClickListener(onClick)
        .build()
