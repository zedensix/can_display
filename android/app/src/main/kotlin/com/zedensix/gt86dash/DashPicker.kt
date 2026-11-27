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

// The settings page's dashes, by their names (DashSection draws them):
// any (the first found), then the one picked, the one connected and those
// its own scan sees nearby, each with its state and, opened, its details
// (its address, signal strength, when it was last seen).
@SuppressLint("MissingPermission") // scan runs only once they're granted
class DashPicker(private val context: Context, private val settings: Settings) {
    private class Seen(val name: String, val rssi: Int, val at: Long)

    // What the scan saw, by address.
    private val seen = linkedMapOf<String, Seen>()
    private var scanning = false

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

    // on: Bluetooth allowed and on, for the scan, started again after
    // Bluetooth turned off ended it.
    fun scan(on: Boolean) {
        if (!on) scanning = false
        if (on && !scanning) {
            val filter = ScanFilter.Builder().setServiceUuid(ParcelUuid(LiveLink.SERVICE)).build()
            val mode = ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build()
            scanner()?.startScan(listOf(filter), mode, scan)
            scanning = true
        }
    }

    // The dashes to list after Any dash: the one picked, the one connected,
    // those seen.
    fun addresses(): List<String> = (listOfNotNull(settings.dash, LiveLink.device) + seen.keys).distinct()

    private fun nearby(address: String) = seen[address]?.let { SystemClock.uptimeMillis() - it.at < GONE_MS } == true

    fun state(address: String) =
        when {
            address == LiveLink.device && LiveLink.status == LiveLink.Status.LIVE -> "connected"
            address == LiveLink.device -> "connecting"
            nearby(address) -> "nearby"
            else -> "not in range"
        }

    fun details(address: String): String {
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

    private companion object {
        const val DASH_NAME = "GT86 DASH"

        // A dash its scan hasn't heard for this long isn't nearby.
        const val GONE_MS = 10_000L
    }
}
