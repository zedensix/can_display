package com.zedensix.gt86dash

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp

// The dash's firmware update, a page in place of the dash, opened by the
// activity when one starts: a ring with the step's progress (turning while
// it waits on the knob or the install), its steps (the data log saved,
// allowed on the dash, sent, installed) marked done, under way, failed or
// to come, what the step under way says, and its buttons: Cancel until the
// install, Update anyway when the log couldn't be saved, Done once over
// (Try again too when it failed). Two columns in landscape.
@Composable
fun UpdatePage(onClose: () -> Unit) {
    val context = LocalContext.current
    ticking(TICK_MS)
    // The furthest step it got to, for a failure to show where.
    val reached = remember { intArrayOf(0) }
    val stage = FirmwareUpdate.stage
    val image = FirmwareImage.bundled(context)
    val step = when (stage) {
        FirmwareUpdate.Stage.SAVING_LOG, FirmwareUpdate.Stage.LOG_NOT_SAVED -> 0
        FirmwareUpdate.Stage.WAITING_CLICK -> 1
        FirmwareUpdate.Stage.SENDING -> 2
        FirmwareUpdate.Stage.INSTALLING -> 3
        FirmwareUpdate.Stage.IDLE -> reached[0]
    }
    reached[0] = step
    val error = FirmwareUpdate.error
    val idle = stage == FirmwareUpdate.Stage.IDLE
    val installed = idle && error == null && image != null && LiveLink.dashFirmware?.let { image.runsOn(it) } == true
    val pull = LiveLink.pullKb()
    val imageKb = (image?.size ?: 0) / 1024
    val (title, detail, color) = when (stage) {
        FirmwareUpdate.Stage.SAVING_LOG ->
            Triple("Saving the data log", pull?.let { (got, total) -> "$got of $total KB, onto the phone" } ?: "Onto the phone, before the update takes its place", Colors.LABEL)
        FirmwareUpdate.Stage.LOG_NOT_SAVED -> Triple("Couldn't save the data log", "$error: the update takes the place of the dash's oldest records", Colors.WARNING)
        FirmwareUpdate.Stage.WAITING_CLICK -> Triple("Click the knob on the dash", "To allow the update: it waits 30 s", Colors.ACCENT)
        FirmwareUpdate.Stage.SENDING -> Triple("Sending the firmware", "${imageKb * FirmwareUpdate.percent() / 100} of $imageKb KB", Colors.LABEL)
        FirmwareUpdate.Stage.INSTALLING -> Triple("Installing", "Don't turn the car off: the dash restarts", Colors.WARNING)
        FirmwareUpdate.Stage.IDLE -> when {
            installed -> Triple("Updated", "The dash runs the app's firmware", Colors.GOOD)
            error != null -> Triple("Not updated", error, Colors.DANGER)
            else -> Triple("Not updated", "Cancelled", Colors.LABEL)
        }
    }
    val fraction = when {
        installed -> 1f
        stage == FirmwareUpdate.Stage.SAVING_LOG -> pull?.let { (got, total) -> if (total > 0) got.toFloat() / total else 1f } ?: 0f
        stage == FirmwareUpdate.Stage.SENDING -> FirmwareUpdate.percent() / 100f
        stage == FirmwareUpdate.Stage.WAITING_CLICK || stage == FirmwareUpdate.Stage.INSTALLING -> null
        else -> 0f
    }
    val ringColor = when {
        installed -> Colors.GOOD
        idle -> Colors.DANGER
        stage == FirmwareUpdate.Stage.LOG_NOT_SAVED -> Colors.WARNING
        else -> Colors.ACCENT
    }
    val center = when {
        installed -> "✓"
        idle -> "✕"
        stage == FirmwareUpdate.Stage.LOG_NOT_SAVED -> "!"
        fraction == null -> "…"
        else -> "${(fraction * 100).toInt()}%"
    }

    val ring = @Composable {
        Box(Modifier.size(220.dp), contentAlignment = Alignment.Center) {
            val track = MaterialTheme.colorScheme.surfaceContainerHighest
            if (fraction == null) {
                CircularProgressIndicator(Modifier.fillMaxSize(), color = Color(ringColor), strokeWidth = 14.dp, trackColor = track)
            } else {
                CircularProgressIndicator(progress = { fraction }, modifier = Modifier.fillMaxSize(), color = Color(ringColor), strokeWidth = 14.dp, trackColor = track)
            }
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Text(center, style = MaterialTheme.typography.displayMedium, color = if (center.length == 1) Color(ringColor) else Color.Unspecified)
                if (!idle) Text("Step ${step + 1} of ${STEPS.size}", style = MaterialTheme.typography.labelLarge, color = MaterialTheme.colorScheme.onSurfaceVariant)
            }
        }
    }
    val info = @Composable { column: ColumnScope ->
        with(column) {
            Text(title, style = MaterialTheme.typography.headlineSmall, color = Color(color), fontWeight = FontWeight.Medium)
            Text(detail, style = MaterialTheme.typography.bodyLarge, color = MaterialTheme.colorScheme.onSurfaceVariant)
            Spacer(Modifier.height(16.dp))
            SectionCard {
                STEPS.forEachIndexed { i, label ->
                    val (mark, markColor) = when {
                        installed || i < step -> "✓" to Colors.GOOD
                        i > step -> "○" to Colors.DIM
                        stage == FirmwareUpdate.Stage.LOG_NOT_SAVED -> "!" to Colors.WARNING
                        idle -> "✕" to Colors.DANGER
                        else -> "●" to Colors.ACCENT
                    }
                    Item(label, leading = { Text(mark, color = Color(markColor), style = MaterialTheme.typography.titleLarge) })
                }
            }
            Spacer(Modifier.height(16.dp))
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                if (stage == FirmwareUpdate.Stage.LOG_NOT_SAVED) Button(onClick = { FirmwareUpdate.anyway() }) { Text("Update anyway") }
                if (FirmwareUpdate.active && stage != FirmwareUpdate.Stage.INSTALLING) {
                    OutlinedButton(onClick = {
                        FirmwareUpdate.cancel()
                        onClose()
                    }) { Text("Cancel") }
                }
                if (idle && !installed && image != null && LiveLink.status == LiveLink.Status.LIVE) {
                    OutlinedButton(onClick = {
                        reached[0] = 0
                        FirmwareUpdate.start(context)
                    }) { Text("Try again") }
                }
                if (idle) Button(onClick = onClose) { Text("Done") }
            }
        }
    }

    val page = Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(horizontal = 16.dp)
    Column(page) {
        PageTitle("Dash update", onBack = onClose)
        if (landscape()) {
            Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(32.dp)) {
                Box(Modifier.weight(1f), contentAlignment = Alignment.Center) { ring() }
                Column(Modifier.weight(1f)) { info(this) }
            }
        } else {
            Box(Modifier.fillMaxWidth().padding(vertical = 24.dp), contentAlignment = Alignment.Center) { ring() }
            info(this)
        }
        Spacer(Modifier.height(24.dp))
    }
}

private const val TICK_MS = 250L
private val STEPS = listOf("Save the data log", "Allow it on the dash", "Send the firmware", "Install it")
