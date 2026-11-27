package com.zedensix.gt86dash

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp

// The settings' dash firmware: whether the dash runs the one the app
// carries (a dot its color), Update the dash when it doesn't, then the
// update's stage (FirmwareUpdate; its own page, UpdatePage, opens with
// it), Cancel until the image is all sent.
@Composable
fun FirmwareSection() {
    val context = LocalContext.current
    ticking(TICK_MS)
    val stage = FirmwareUpdate.stage
    val image = FirmwareImage.bundled(context)
    val dash = LiveLink.dashFirmware
    val live = LiveLink.status == LiveLink.Status.LIVE
    val available = live && LiveLink.updatable && image != null && dash != null && !image.runsOn(dash)
    val (text, color) = when (stage) {
        FirmwareUpdate.Stage.SAVING_LOG ->
            (LiveLink.pullKb()?.let { (got, total) -> "Saving the data log… $got of $total KB" } ?: "Saving the data log…") to Colors.ACCENT
        FirmwareUpdate.Stage.LOG_NOT_SAVED -> "Couldn't save the data log" to Colors.WARNING
        FirmwareUpdate.Stage.WAITING_CLICK -> "Click the knob on the dash to allow it" to Colors.ACCENT
        FirmwareUpdate.Stage.SENDING -> "Sending… ${FirmwareUpdate.percent()}%" to Colors.ACCENT
        FirmwareUpdate.Stage.INSTALLING -> "Installing: don't turn the car off" to Colors.WARNING
        FirmwareUpdate.Stage.IDLE -> when {
            image == null -> "No firmware in this app" to Colors.DIM
            !live -> "Dash not connected" to Colors.DIM
            !LiveLink.updatable -> "Can't be updated from the phone" to Colors.DIM
            dash == null -> "Reading the dash's firmware" to Colors.DIM
            available -> "Update available (${image.size / 1024} KB)" to Colors.WARNING
            else -> "Up to date" to Colors.GOOD
        }
    }
    val error = FirmwareUpdate.error.takeIf { stage == FirmwareUpdate.Stage.IDLE }
    val detail = when {
        error != null -> error
        stage == FirmwareUpdate.Stage.LOG_NOT_SAVED -> "${FirmwareUpdate.error}: the update takes the place of the dash's oldest records"
        stage == FirmwareUpdate.Stage.IDLE && live && !LiveLink.updatable -> "Its firmware is older than that: flash it over USB once"
        else -> ""
    }
    val update = stage == FirmwareUpdate.Stage.IDLE && available
    val anyway = stage == FirmwareUpdate.Stage.LOG_NOT_SAVED
    val cancel = FirmwareUpdate.active && stage != FirmwareUpdate.Stage.INSTALLING
    SectionTitle("Dash firmware")
    SectionCard {
        Item(text, detail, supportingColor = if (error != null) Color(Colors.DANGER) else Color.Unspecified, trailing = { Dot(color) })
        if (update || anyway || cancel) {
            Row(Modifier.padding(start = 16.dp, end = 16.dp, bottom = 16.dp), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                if (update) Button(onClick = { FirmwareUpdate.start(context) }) { Text("Update the dash") }
                if (anyway) Button(onClick = { FirmwareUpdate.anyway() }) { Text("Update anyway") }
                if (cancel) OutlinedButton(onClick = { FirmwareUpdate.cancel() }) { Text("Cancel") }
            }
        }
    }
}

private const val TICK_MS = 250L
