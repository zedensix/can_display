package com.zedensix.gt86dash

import android.annotation.SuppressLint
import android.bluetooth.BluetoothManager
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.ParcelUuid
import android.os.SystemClock
import android.view.ViewGroup.LayoutParams.MATCH_PARENT
import android.view.ViewGroup.LayoutParams.WRAP_CONTENT
import android.widget.LinearLayout
import android.widget.TextView

// The settings page's dashes, by their names: any (the first found), then
// the one picked, the one connected and those the page's own scan sees
// nearby; the one in use outlined. A tap on a dash opens its details (its
// address, signal strength, when it was last seen) and a button to use it
// or forget it; a tap on Any dash uses the first found.
@SuppressLint("MissingPermission") // refresh scans only once they're granted
class DashPicker(private val context: Context, private val settings: Settings) {
    val view = LinearLayout(context).apply { orientation = LinearLayout.VERTICAL }

    private class Seen(val name: String, val rssi: Int, val at: Long)

    // What the page's scan saw, by address.
    private val seen = linkedMapOf<String, Seen>()
    private var scanning = false
    private var on = false
    private var shown = ""

    // The dash whose details are open, and the text they're in.
    private var open: String? = null
    private var details: TextView? = null

    private val scan = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val name = result.scanRecord?.deviceName ?: seen[result.device.address]?.name ?: DASH_NAME
            seen[result.device.address] = Seen(name, result.rssi, SystemClock.uptimeMillis())
        }

        override fun onScanFailed(errorCode: Int) {
            scanning = false
        }
    }

    private fun scanner() = context.getSystemService(BluetoothManager::class.java)?.adapter?.bluetoothLeScanner

    // A dash's name, as it advertises it, else as it was last.
    fun name(address: String): String =
        seen[address]?.name
            ?: LiveLink.deviceName.takeIf { address == LiveLink.device }
            ?: settings.dashName.takeIf { address == settings.dash }
            ?: DASH_NAME

    fun stop() {
        if (scanning) scanner()?.stopScan(scan)
        scanning = false
    }

    // on: Bluetooth allowed and on, for the scan. The rows are built again
    // only when something they show changed, so a tap isn't lost to a row
    // rebuilt under it; the open details follow the signal in place.
    fun refresh(on: Boolean) {
        this.on = on
        if (!on) scanning = false // Bluetooth off ended it
        if (on && !scanning) {
            val filter = ScanFilter.Builder().setServiceUuid(ParcelUuid(LiveLink.SERVICE)).build()
            val mode = ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build()
            scanner()?.startScan(listOf(filter), mode, scan)
            scanning = true
        }
        val addresses = (listOfNotNull(settings.dash, LiveLink.device) + seen.keys).distinct()
        val key = "${settings.dash} $open " + addresses.joinToString { "$it ${name(it)} ${state(it)}" }
        if (key != shown) {
            shown = key
            build(addresses)
        }
        open?.let {
            if (it == LiveLink.device) LiveLink.readRssi()
            details?.text = detailsText(it)
        }
    }

    private fun nearby(address: String) = seen[address]?.let { SystemClock.uptimeMillis() - it.at < GONE_MS } == true

    private fun state(address: String) =
        when {
            address == LiveLink.device && LiveLink.status == LiveLink.Status.LIVE -> "connected"
            address == LiveLink.device -> "connecting"
            nearby(address) -> "nearby"
            else -> "not in range"
        }

    private fun detailsText(address: String): String {
        val connected = address == LiveLink.device
        val rssi = if (connected) LiveLink.rssi else seen[address]?.rssi
        val signal = rssi?.let {
            val word = when {
                it >= -60 -> "strong"
                it >= -75 -> "fair"
                else -> "weak"
            }
            "$it dBm, $word"
        } ?: "--"
        val last = seen[address]?.at
        val when_ = when {
            connected -> "connected now"
            last != null -> "${(SystemClock.uptimeMillis() - last) / 1000} s ago"
            else -> "not since the app started"
        }
        return "Address: $address\nSignal: $signal\nSeen: $when_"
    }

    private fun build(addresses: List<String>) {
        view.removeAllViews()
        details = null
        row(null, "Any dash", "the first one found")
        addresses.forEach { row(it, name(it), state(it)) }
    }

    private fun row(address: String?, name: String, line: String) {
        val picked = address == settings.dash
        val row = LinearLayout(context).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundResource(if (picked) R.drawable.pill_selected else R.drawable.pill_button)
            setPadding(dp(context, 20), dp(context, 8), dp(context, 20), dp(context, 10))
        }
        row.addView(pageText(context, name, 20f, R.font.barlow_condensed_semibold, if (picked) Colors.ACCENT else Colors.LABEL))
        row.addView(pageText(context, line, 15f, R.font.barlow_condensed_medium, Colors.DIM))
        row.setOnClickListener {
            if (address == null) {
                if (!picked) LiveLink.use(null, null)
            } else {
                open = if (open == address) null else address
            }
            refresh(on)
        }
        if (address != null && address == open) {
            details = pageText(context, detailsText(address), 16f, R.font.barlow_condensed_medium, Colors.LABEL).apply {
                setPadding(0, dp(context, 8), 0, dp(context, 8))
                setLineSpacing(0f, 1.3f)
            }
            row.addView(details)
            val action = pagePill(context, if (picked) "Forget this dash" else "Use this dash") {
                if (picked) LiveLink.use(null, null) else LiveLink.use(address, name)
                open = null
                refresh(on)
            }
            row.addView(action, LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT))
        }
        view.addView(row, LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT).apply { bottomMargin = dp(context, 8) })
    }

    private companion object {
        const val DASH_NAME = "GT86 DASH"

        // A dash its scan hasn't heard for this long isn't nearby.
        const val GONE_MS = 10_000L
    }
}
