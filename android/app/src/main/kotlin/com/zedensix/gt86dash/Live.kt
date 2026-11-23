package com.zedensix.gt86dash

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.content.pm.PackageManager
import android.os.ParcelUuid
import android.os.SystemClock
import java.util.UUID

// The live readings off the dash: a frame (src/app/data/live.h) decoded
// into Readings. The layout must follow live_encode's.
object LiveFrame {
    const val VERSION = 1
    const val SIZE = 17

    fun decode(b: ByteArray): Readings? {
        if (b.size < SIZE || b[0].toInt() != VERSION) return null
        fun u8(i: Int) = b[i].toInt() and 0xFF
        fun u16(i: Int) = u8(i) or (u8(i + 1) shl 8)
        fun i16(i: Int) = u16(i).toShort().toInt()
        val flags = u8(16)
        return Readings(
            oil = i16(1).takeIf { it != Short.MIN_VALUE.toInt() }?.let { it / 10f },
            coolant = i16(3).takeIf { it != Short.MIN_VALUE.toInt() }?.let { it / 10f },
            intake = b[5].toInt().takeIf { it != Byte.MIN_VALUE.toInt() }?.toFloat(),
            afr = u8(6).takeIf { it != 0 }?.let { it / 10f },
            battery = u8(7).takeIf { it != 0 }?.let { it / 10f },
            ethanol = u8(8).takeIf { it != 0xFF }?.toFloat(),
            rpm = u16(9).takeIf { it != 0xFFFF }?.toFloat(),
            gear = u8(11),
            outside = b[12].toInt().takeIf { it != Byte.MIN_VALUE.toInt() }?.toFloat(),
            km = (u8(13) or (u8(14) shl 8) or (u8(15) shl 16)) / 10f,
            trip = flags and 0x10 != 0,
            mode = DriveMode.entries.getOrElse(flags and 0x07) { DriveMode.COMFORT },
            offThrottle = flags and 0x08 != 0,
        )
    }
}

// The link to the dash, "GT86 DASH", one for the car's screen and the
// phone's: found by its service (and its address, once one is chosen in
// the settings), connected, subscribed to its live readings; found again
// whenever it drops (the contact off and on). Up while anything draws or
// the settings are open (start/stop count them).
@SuppressLint("MissingPermission") // start checks them, status says when they're missing
object LiveLink {
    enum class Status { NO_PERMISSION, BLUETOOTH_OFF, SEARCHING, CONNECTING, LIVE }

    val SERVICE: UUID = UUID.fromString("c0ff8600-4b1e-4c7a-9e2d-5a3f86d0a001")
    private val LIVE = UUID.fromString("c0ff8603-4b1e-4c7a-9e2d-5a3f86d0a001")
    private val CCCD = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

    // A link with no frame for this long counts as gone.
    private const val SILENT_MS = 2000L

    private var users = 0
    private var context: Context? = null
    private var gatt: BluetoothGatt? = null
    private var scanning = false

    @Volatile var status = Status.SEARCHING
        private set

    // The address of the dash connecting or connected, null while none.
    @Volatile var device: String? = null
        private set

    @Volatile private var latest: Readings? = null

    @Volatile private var latestAt = 0L

    val permissions = arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)

    fun granted(c: Context) = permissions.all { c.checkSelfPermission(it) == PackageManager.PERMISSION_GRANTED }

    // The readings, if a frame came in lately.
    fun readings(): Readings? =
        latest.takeIf { status == Status.LIVE && SystemClock.uptimeMillis() - latestAt < SILENT_MS }

    @Synchronized
    fun start(c: Context) {
        users++
        context = c.applicationContext
        connect()
    }

    @Synchronized
    fun stop() {
        users = (users - 1).coerceAtLeast(0)
        if (users == 0) close()
    }

    private var retriedAt = 0L

    // Tries again, at most every RETRY_MS (Android blocks an app that
    // starts scanning too often): the permissions granted since, Bluetooth
    // turned on, a scan that failed. Bluetooth turned off ends the scan
    // without a word: it says so.
    @Synchronized
    fun retry() {
        if (users > 0 && status != Status.NO_PERMISSION && adapter()?.isEnabled != true) {
            close()
            status = Status.BLUETOOTH_OFF
        }
        val now = SystemClock.uptimeMillis()
        if (users == 0 || gatt != null || scanning || now - retriedAt < RETRY_MS) return
        retriedAt = now
        connect()
    }

    private const val RETRY_MS = 5000L

    // Connects to that dash from now on (null: the first found), dropping
    // the one connected.
    @Synchronized
    fun use(address: String?) {
        val c = context ?: return
        Settings(c).dash = address
        close()
        status = Status.SEARCHING
        if (users > 0) connect()
    }

    private fun adapter() = context?.getSystemService(BluetoothManager::class.java)?.adapter

    private fun connect() {
        val c = context ?: return
        if (gatt != null || scanning) return
        if (!granted(c)) {
            status = Status.NO_PERMISSION
            return
        }
        val adapter = adapter()
        if (adapter == null || !adapter.isEnabled) {
            status = Status.BLUETOOTH_OFF
            return
        }
        status = Status.SEARCHING
        val filter = ScanFilter.Builder().setServiceUuid(ParcelUuid(SERVICE))
            .apply { Settings(c).dash?.let { setDeviceAddress(it) } }
            .build()
        val settings = ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build()
        adapter.bluetoothLeScanner?.startScan(listOf(filter), settings, scan)
        scanning = true
    }

    private fun close() {
        if (scanning) adapter()?.bluetoothLeScanner?.stopScan(scan)
        scanning = false
        gatt?.close()
        gatt = null
        device = null
        latest = null
    }

    private val scan = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            synchronized(LiveLink) {
                if (!scanning) return
                adapter()?.bluetoothLeScanner?.stopScan(this)
                scanning = false
                status = Status.CONNECTING
                device = result.device.address
                // Its replacement (BluetoothGattConnectionSettings) is
                // Android 17's only.
                @Suppress("DEPRECATION")
                gatt = result.device.connectGatt(context, false, callback, BluetoothDevice.TRANSPORT_LE)
            }
        }

        override fun onScanFailed(errorCode: Int) {
            synchronized(LiveLink) { scanning = false }
        }
    }

    private val callback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(g: BluetoothGatt, state: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                g.discoverServices()
                return
            }
            // Gone: look for it again, while anything still draws.
            synchronized(LiveLink) {
                g.close()
                if (gatt === g) {
                    gatt = null
                    device = null
                }
                latest = null
                status = Status.SEARCHING
                if (users > 0) connect()
            }
        }

        override fun onServicesDiscovered(g: BluetoothGatt, state: Int) {
            val live = g.getService(SERVICE)?.getCharacteristic(LIVE) ?: return
            g.setCharacteristicNotification(live, true)
            val cccd = live.getDescriptor(CCCD) ?: return
            g.writeDescriptor(cccd, byteArrayOf(0x01, 0x00))
        }

        override fun onCharacteristicChanged(g: BluetoothGatt, c: BluetoothGattCharacteristic, value: ByteArray) {
            if (c.uuid != LIVE) return
            latest = LiveFrame.decode(value) ?: return
            latestAt = SystemClock.uptimeMillis()
            status = Status.LIVE
        }
    }
}
