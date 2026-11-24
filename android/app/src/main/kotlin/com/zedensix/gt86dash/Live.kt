package com.zedensix.gt86dash

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
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

// The live readings off the dash: a frame (pico/src/app/data/live.h) decoded
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
// the settings are open (start/stop count them). It also carries the
// pulls of the dash's data log (Pull), on the data characteristic.
@SuppressLint("MissingPermission") // start checks them, status says when they're missing
object LiveLink {
    enum class Status { NO_PERMISSION, BLUETOOTH_OFF, SEARCHING, CONNECTING, LIVE }

    val SERVICE: UUID = UUID.fromString("c0ff8600-4b1e-4c7a-9e2d-5a3f86d0a001")
    private val LIVE = UUID.fromString("c0ff8603-4b1e-4c7a-9e2d-5a3f86d0a001")
    private val DATA = UUID.fromString("c0ff8602-4b1e-4c7a-9e2d-5a3f86d0a001")
    private val CONTROL = UUID.fromString("c0ff8601-4b1e-4c7a-9e2d-5a3f86d0a001")

    // The biggest the dash takes: a pull's notifications as big as can be.
    private const val MTU = 247
    private val CCCD = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

    // A link with no frame for this long counts as gone.
    private const val SILENT_MS = 2000L

    private var users = 0
    private var context: Context? = null
    private var gatt: BluetoothGatt? = null
    private var scanning = false

    @Volatile var status = Status.SEARCHING
        private set

    // The address of the dash connecting or connected, null while none,
    // its name and its signal strength (dBm: readRssi asks for it again).
    @Volatile var device: String? = null
        private set

    @Volatile var deviceName: String? = null
        private set

    @Volatile var rssi: Int? = null
        private set

    @Volatile private var latest: Readings? = null

    @Volatile private var latestAt = 0L

    private var pull: Pull? = null

    // The data characteristic's notifications on: a pull can be asked.
    private var dataReady = false

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

    @Synchronized
    fun readRssi() {
        gatt?.readRemoteRssi()
    }

    // Connects to that dash from now on (null: the first found), dropping
    // the one connected; its name kept to show it while it's off.
    @Synchronized
    fun use(address: String?, name: String?) {
        val c = context ?: return
        Settings(c).dash = address
        Settings(c).dashName = name
        close()
        status = Status.SEARCHING
        if (users > 0) connect()
    }

    // Asks the dash for what it logged since from (Pull); done gets it,
    // on Bluetooth's thread. False while not live, or while a pull is
    // under way (one gone quiet past Pull.QUIET_MS is given up).
    @Synchronized
    fun pull(from: Long, done: (PullHeader, ByteArray) -> Unit): Boolean {
        val g = gatt ?: return false
        if (status != Status.LIVE || !dataReady) return false
        pull?.let { if (SystemClock.uptimeMillis() - it.heardAt < Pull.QUIET_MS) return false }
        val control = g.getService(SERVICE)?.getCharacteristic(CONTROL) ?: return false
        val p = Pull(SystemClock.uptimeMillis(), done)
        if (g.writeCharacteristic(control, Pull.request(from), BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT) != BluetoothGatt.GATT_SUCCESS) return false
        pull = p
        return true
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
        deviceName = null
        rssi = null
        latest = null
        pull = null
        dataReady = false
    }

    private val scan = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            synchronized(LiveLink) {
                if (!scanning) return
                adapter()?.bluetoothLeScanner?.stopScan(this)
                scanning = false
                status = Status.CONNECTING
                device = result.device.address
                deviceName = result.scanRecord?.deviceName
                rssi = result.rssi
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
                    deviceName = null
                    rssi = null
                }
                latest = null
                pull = null
                dataReady = false
                status = Status.SEARCHING
                if (users > 0) connect()
            }
        }

        // One request at a time: the MTU, then the live readings, then the
        // data log's notifications.
        override fun onServicesDiscovered(g: BluetoothGatt, state: Int) {
            g.requestMtu(MTU)
        }

        override fun onMtuChanged(g: BluetoothGatt, mtu: Int, state: Int) {
            subscribe(g, LIVE)
        }

        override fun onDescriptorWrite(g: BluetoothGatt, d: BluetoothGattDescriptor, state: Int) {
            if (d.characteristic.uuid == LIVE) subscribe(g, DATA)
            if (d.characteristic.uuid == DATA) dataReady = state == BluetoothGatt.GATT_SUCCESS
        }

        override fun onReadRemoteRssi(g: BluetoothGatt, value: Int, state: Int) {
            if (state == BluetoothGatt.GATT_SUCCESS && gatt === g) rssi = value
        }

        override fun onCharacteristicChanged(g: BluetoothGatt, c: BluetoothGattCharacteristic, value: ByteArray) {
            if (c.uuid == DATA) {
                synchronized(LiveLink) {
                    if (pull?.take(value, SystemClock.uptimeMillis()) == true) pull = null
                }
                return
            }
            if (c.uuid != LIVE) return
            latest = LiveFrame.decode(value) ?: return
            latestAt = SystemClock.uptimeMillis()
            status = Status.LIVE
        }
    }

    private fun subscribe(g: BluetoothGatt, uuid: UUID) {
        val c = g.getService(SERVICE)?.getCharacteristic(uuid) ?: return
        g.setCharacteristicNotification(c, true)
        val cccd = c.getDescriptor(CCCD) ?: return
        g.writeDescriptor(cccd, byteArrayOf(0x01, 0x00))
    }
}
