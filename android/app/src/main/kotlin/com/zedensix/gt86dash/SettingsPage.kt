package com.zedensix.gt86dash

import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.content.Intent
import android.net.Uri
import android.provider.Settings.ACTION_APPLICATION_DETAILS_SETTINGS
import android.widget.Toast
import androidx.activity.compose.LocalActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts.RequestMultiplePermissions
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.FilledTonalIconButton
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.RadioButton
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.SideEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp

// The phone's settings, a page in place of the dash: the connection (the
// link's state, what it needs, and which dash: the first found, or one
// picked from those nearby or used before, its firmware updated from the
// phone's), then the gauges' (the shift light, demo, the min and max),
// then what the phone records (RecordingSection); three columns in
// landscape. Bluetooth's permissions refused for good, the system asks no
// more: Allow Bluetooth opens the app's page in the system's settings.
@Composable
fun SettingsPage(picker: DashPicker) {
    val context = LocalContext.current
    ticking(TICK_MS)
    LiveLink.retry()
    val bluetoothOn = LiveLink.granted(context) && context.getSystemService(BluetoothManager::class.java)?.adapter?.isEnabled == true
    SideEffect { picker.scan(bluetoothOn) }
    DisposableEffect(Unit) { onDispose { picker.stop() } }

    val column = Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(horizontal = 16.dp)
    if (landscape()) {
        Column(column) {
            PageTitle("Settings")
            Row(horizontalArrangement = Arrangement.spacedBy(24.dp)) {
                Column(Modifier.weight(1f)) { Connection(picker) }
                Column(Modifier.weight(1f)) { Gauges() }
                Column(Modifier.weight(1f)) { RecordingSection() }
            }
            Spacer(Modifier.height(24.dp))
        }
    } else {
        Column(column) {
            PageTitle("Settings")
            Connection(picker)
            Gauges()
            RecordingSection()
            Spacer(Modifier.height(24.dp))
        }
    }
}

@Composable
private fun Connection(picker: DashPicker) {
    val context = LocalContext.current
    val activity = LocalActivity.current
    val ask = rememberLauncherForActivityResult(RequestMultiplePermissions()) {
        if (!LiveLink.granted(context) && activity?.shouldShowRequestPermissionRationale(LiveLink.permissions[0]) == false) {
            context.startActivity(Intent(ACTION_APPLICATION_DETAILS_SETTINGS, Uri.fromParts("package", context.packageName, null)))
        }
    }
    val device = LiveLink.device
    val noData = LiveLink.readings() == null
    val (title, line) = when (LiveLink.status) {
        LiveLink.Status.NO_PERMISSION -> "Bluetooth not allowed" to "The app needs it to find the dash"
        LiveLink.Status.BLUETOOTH_OFF -> "Bluetooth is off" to "The dash's readings come over it"
        LiveLink.Status.SEARCHING -> "Looking for the dash" to (Settings(context).dash?.let { picker.name(it) } ?: "The first one found")
        LiveLink.Status.CONNECTING -> "Connecting" to (device?.let { picker.name(it) } ?: "")
        LiveLink.Status.LIVE ->
            if (noData) "Connected" to "No data coming in from the car" else "Live" to (device?.let { picker.name(it) } ?: "")
    }
    val color = when (LiveLink.status) {
        LiveLink.Status.LIVE -> if (noData) Colors.WARNING else Colors.GOOD
        LiveLink.Status.SEARCHING, LiveLink.Status.CONNECTING -> Colors.WARNING
        else -> Colors.DANGER
    }
    SectionTitle("Connection")
    SectionCard {
        Item(title, line, trailing = { Dot(color) })
        when (LiveLink.status) {
            LiveLink.Status.NO_PERMISSION ->
                Button(onClick = { ask.launch(LiveLink.permissions) }, Modifier.padding(start = 16.dp, bottom = 16.dp)) { Text("Allow Bluetooth") }
            LiveLink.Status.BLUETOOTH_OFF ->
                Button(onClick = { context.startActivity(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)) }, Modifier.padding(start = 16.dp, bottom = 16.dp)) {
                    Text("Turn Bluetooth on")
                }
            else -> {}
        }
    }
    SectionTitle("Dash")
    SectionCard { Dashes(picker) }
    Footnote("A dash shows here while it's on and no other phone holds its link")
    FirmwareSection()
}

// Any dash, then each dash: a tap opens its details and Use or Forget,
// the radio uses it.
@Composable
private fun Dashes(picker: DashPicker) {
    val settings = Settings(LocalContext.current)
    var open by remember { mutableStateOf<String?>(null) }
    val picked = settings.dash
    Item(
        "Any dash",
        "The first one found",
        trailing = { RadioButton(selected = picked == null, onClick = { LiveLink.use(null, null) }) },
        onClick = { LiveLink.use(null, null) },
    )
    for (address in picker.addresses()) {
        val name = picker.name(address)
        Divider()
        Item(
            name,
            picker.state(address).replaceFirstChar { it.uppercase() },
            trailing = { RadioButton(selected = picked == address, onClick = { LiveLink.use(address, name) }) },
            onClick = { open = if (open == address) null else address },
        )
        if (open == address) {
            if (address == LiveLink.device) SideEffect { LiveLink.readRssi() }
            Text(
                picker.details(address),
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
                modifier = Modifier.padding(horizontal = 16.dp),
            )
            TextButton(
                onClick = {
                    if (picked == address) LiveLink.use(null, null) else LiveLink.use(address, name)
                    open = null
                },
                modifier = Modifier.padding(start = 4.dp, bottom = 8.dp),
            ) { Text(if (picked == address) "Forget this dash" else "Use this dash") }
        }
    }
}

@Composable
private fun Gauges() {
    val context = LocalContext.current
    val settings = remember { Settings(context) }
    var shift by remember { mutableStateOf(settings.shiftRpmText()) }
    var demo by remember { mutableStateOf(settings.demo) }
    fun step(n: Int) {
        settings.stepShiftRpm(n)
        shift = settings.shiftRpmText()
    }
    SectionTitle("Gauges")
    SectionCard {
        Item("Shift light", "The LEDs light over the last 1500 rpm before it")
        Row(Modifier.fillMaxWidth().padding(start = 16.dp, end = 16.dp, bottom = 16.dp), verticalAlignment = Alignment.CenterVertically) {
            FilledTonalIconButton(onClick = { step(-1) }) { Icon(painterResource(R.drawable.ic_remove), "Lower") }
            Text(shift, style = MaterialTheme.typography.titleLarge, textAlign = TextAlign.Center, modifier = Modifier.widthIn(min = 112.dp))
            FilledTonalIconButton(onClick = { step(1) }) { Icon(painterResource(R.drawable.ic_add), "Higher") }
        }
        Divider()
        Item(
            "Demo",
            "Made-up readings instead of the dash's",
            trailing = {
                Switch(checked = demo, onCheckedChange = {
                    demo = it
                    settings.demo = it
                })
            },
            onClick = {
                demo = !demo
                settings.demo = demo
            },
        )
        Divider()
        Item("Reset min and max", "Every gauge's, on the phone and the car", onClick = {
            Extremes.reset()
            Toast.makeText(context, "Min and max reset", Toast.LENGTH_SHORT).show()
        })
    }
}

private const val TICK_MS = 500L
