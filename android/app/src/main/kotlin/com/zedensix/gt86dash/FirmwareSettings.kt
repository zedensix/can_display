package com.zedensix.gt86dash

import android.app.Activity
import android.view.View
import android.view.ViewGroup.LayoutParams.MATCH_PARENT
import android.view.ViewGroup.LayoutParams.WRAP_CONTENT
import android.view.WindowManager
import android.widget.LinearLayout
import android.widget.TextView

// The settings page's dash firmware: whether the dash runs the one the app
// carries, a button to update it when it doesn't, then the update's stage
// (FirmwareUpdate), Cancel until the image is all sent; the screen kept on
// meanwhile.
class FirmwareSettings(private val activity: Activity, private val label: (String) -> View) {
    private lateinit var title: TextView
    private lateinit var detail: TextView
    private lateinit var update: View
    private lateinit var anyway: View
    private lateinit var cancel: View

    fun build(): View {
        val a = activity
        val c = LinearLayout(a).apply { orientation = LinearLayout.VERTICAL }
        c.addView(label("DASH FIRMWARE"))
        title = pageText(a, "", 22f, R.font.barlow_condensed_bold, Colors.LABEL)
        detail = pageText(a, "", 16f, R.font.barlow_condensed_medium, Colors.DIM)
        c.addView(title)
        c.addView(detail)
        update = button(c, "Update the dash") { FirmwareUpdate.start(a) }
        anyway = button(c, "Update anyway") { FirmwareUpdate.anyway() }
        cancel = button(c, "Cancel") { FirmwareUpdate.cancel() }
        refresh()
        return c
    }

    fun pause() {
        activity.window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
    }

    fun refresh() {
        val stage = FirmwareUpdate.stage
        val image = FirmwareImage.bundled(activity)
        val dash = LiveLink.dashFirmware
        val live = LiveLink.status == LiveLink.Status.LIVE
        val available = live && LiveLink.updatable && image != null && dash != null && !image.runsOn(dash)
        val (text, color) = when (stage) {
            FirmwareUpdate.Stage.SAVING_LOG -> "Saving the data log…" to Colors.LABEL
            FirmwareUpdate.Stage.LOG_NOT_SAVED -> "Couldn't save the data log" to Colors.WARNING
            FirmwareUpdate.Stage.WAITING_CLICK -> "Click the knob on the dash to allow it" to Colors.ACCENT
            FirmwareUpdate.Stage.SENDING -> "Sending… ${FirmwareUpdate.percent()}%" to Colors.LABEL
            FirmwareUpdate.Stage.INSTALLING -> "Installing: don't turn the car off, the dash restarts" to Colors.WARNING
            FirmwareUpdate.Stage.IDLE -> when {
                image == null -> "No firmware in this app" to Colors.DIM
                !live -> "Dash not connected" to Colors.DIM
                !LiveLink.updatable -> "Can't be updated from the phone" to Colors.DIM
                dash == null -> "Reading the dash's firmware" to Colors.DIM
                available -> "Update available (${image.size / 1024} KB)" to Colors.WARNING
                else -> "Up to date" to Colors.GOOD
            }
        }
        title.text = text
        title.setTextColor(color)
        val error = FirmwareUpdate.error.takeIf { stage == FirmwareUpdate.Stage.IDLE }
        detail.text = when {
            error != null -> error
            stage == FirmwareUpdate.Stage.LOG_NOT_SAVED -> "The update takes the place of the dash's oldest records"
            stage == FirmwareUpdate.Stage.IDLE && live && !LiveLink.updatable -> "Its firmware is older than that: flash it over USB once"
            else -> ""
        }
        detail.setTextColor(if (error != null) Colors.DANGER else Colors.DIM)
        detail.visibility = if (detail.text.isEmpty()) View.GONE else View.VISIBLE
        update.visibility = if (stage == FirmwareUpdate.Stage.IDLE && available) View.VISIBLE else View.GONE
        anyway.visibility = if (stage == FirmwareUpdate.Stage.LOG_NOT_SAVED) View.VISIBLE else View.GONE
        cancel.visibility = if (FirmwareUpdate.active && stage != FirmwareUpdate.Stage.INSTALLING) View.VISIBLE else View.GONE
        if (FirmwareUpdate.active) {
            activity.window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        } else {
            pause()
        }
    }

    private fun button(column: LinearLayout, s: String, onClick: () -> Unit): View {
        val pill = pagePill(activity, s) {
            onClick()
            refresh()
        }
        column.addView(pill, LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT).apply { topMargin = dp(activity, 12) })
        return pill
    }
}
