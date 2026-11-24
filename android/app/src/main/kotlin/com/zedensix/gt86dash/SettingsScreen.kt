package com.zedensix.gt86dash

import androidx.car.app.CarContext
import androidx.car.app.CarToast
import androidx.car.app.Screen
import androidx.car.app.model.Action
import androidx.car.app.model.Header
import androidx.car.app.model.ItemList
import androidx.car.app.model.ListTemplate
import androidx.car.app.model.Pane
import androidx.car.app.model.PaneTemplate
import androidx.car.app.model.Row
import androidx.car.app.model.Template
import androidx.car.app.model.Toggle

// The settings on the car's screen, in Android Auto's own templates (a
// navigation app draws only its map itself): the shift light, demo, the
// gauges' min and max set back to the current readings, lap timing and its
// start line, a spirited drive. The GPS's permission is asked on the phone.
class SettingsScreen(carContext: CarContext) : Screen(carContext) {
    private val settings = Settings(carContext)

    private fun needGps() {
        if ((settings.lapTiming || settings.spirited) && !Recorder.locationGranted(carContext)) {
            carContext.requestPermissions(Recorder.locationPermissions.toList()) { _, _ -> Recorder.update() }
        }
        Recorder.update()
    }

    override fun onGetTemplate(): Template {
        val list = ItemList.Builder()
            .addItem(
                Row.Builder()
                    .setTitle("Shift light")
                    .addText(settings.shiftRpmText())
                    .setBrowsable(true)
                    .setOnClickListener { screenManager.push(ShiftLightScreen(carContext)) }
                    .build(),
            )
            .addItem(
                Row.Builder()
                    .setTitle("Demo")
                    .addText("Made-up readings instead of the dash's")
                    .setToggle(Toggle.Builder { settings.demo = it }.setChecked(settings.demo).build())
                    .build(),
            )
            .addItem(
                Row.Builder()
                    .setTitle("Reset min and max")
                    .addText("Every gauge's, from now on")
                    .setOnClickListener {
                        Extremes.reset()
                        CarToast.makeText(carContext, "Min and max reset", CarToast.LENGTH_SHORT).show()
                    }
                    .build(),
            )
            .addItem(
                Row.Builder()
                    .setTitle("Lap timing")
                    .addText("Lap times from the phone's GPS")
                    .setToggle(
                        Toggle.Builder {
                            settings.lapTiming = it
                            needGps()
                        }.setChecked(settings.lapTiming).build(),
                    )
                    .build(),
            )
            .addItem(
                Row.Builder()
                    .setTitle("Set the start line here")
                    .addText("Tap it crossing the line")
                    .setOnClickListener { CarToast.makeText(carContext, Recorder.setStartLine(carContext), CarToast.LENGTH_SHORT).show() }
                    .build(),
            )
            .addItem(
                Row.Builder()
                    .setTitle("Spirited drive")
                    .addText("Records the way on a map, in the phone's Drives")
                    .setToggle(
                        Toggle.Builder {
                            settings.spirited = it
                            needGps()
                        }.setChecked(settings.spirited).build(),
                    )
                    .build(),
            )
            .build()
        return ListTemplate.Builder()
            .setHeader(Header.Builder().setTitle("Settings").setStartHeaderAction(Action.BACK).build())
            .setSingleList(list)
            .build()
    }
}

// The shift light's RPM, a step of 100 down or up per press, as the
// encoder sets it on the Pico.
class ShiftLightScreen(carContext: CarContext) : Screen(carContext) {
    private val settings = Settings(carContext)

    private fun step(steps: Int, title: String) =
        Action.Builder()
            .setTitle(title)
            .setOnClickListener {
                settings.stepShiftRpm(steps)
                invalidate()
            }
            .build()

    override fun onGetTemplate(): Template {
        val pane = Pane.Builder()
            .addRow(
                Row.Builder()
                    .setTitle(settings.shiftRpmText())
                    .addText("The LEDs light over the last 1500 rpm before it, all blue from it")
                    .build(),
            )
            .addAction(step(-1, "− 100"))
            .addAction(step(1, "+ 100"))
            .build()
        return PaneTemplate.Builder(pane)
            .setHeader(Header.Builder().setTitle("Shift light").setStartHeaderAction(Action.BACK).build())
            .build()
    }
}
