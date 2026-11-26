package com.zedensix.gt86dash

import android.content.Context
import android.os.Handler
import android.os.Looper
import android.os.SystemClock

// The dash's firmware updated from the phone (Firmware.kt the protocol):
// its data log pulled first (the dash stages the image over the oldest
// part of its log ring), then asked ('U'); once the driver clicked its
// knob, the image sent a sector at a time; the dash checks it, installs
// it and restarts, the link coming back on its own, the dash then read
// again. The settings page polls its stage; it runs under LiveLink's
// lock, LiveLink's callbacks on Bluetooth's thread, its tick on the main
// one.
object FirmwareUpdate {
    enum class Stage { IDLE, SAVING_LOG, LOG_NOT_SAVED, WAITING_CLICK, SENDING, INSTALLING }

    @Volatile var stage = Stage.IDLE
        private set

    // Why the last one failed, null if it didn't.
    @Volatile var error: String? = null
        private set

    val active: Boolean get() = stage != Stage.IDLE

    // The dash in it: pulls wait.
    val transferring: Boolean get() = stage == Stage.WAITING_CLICK || stage == Stage.SENDING

    private const val TICK_MS = 250L

    // A pull that can't start in this long counts as failed.
    private const val PULL_START_MS = 20_000L

    // Nothing from the dash for this long while it waits for the click or
    // takes the image: given up. Its own limits are 30 s and 10 s.
    private const val QUIET_MS = 45_000L

    // Its copy takes about 10 s, then the link comes back.
    private const val INSTALL_MS = 90_000L

    private val handler = Handler(Looper.getMainLooper())
    private var image: FirmwareImage? = null
    private var transfer: Transfer? = null
    private var stageAt = 0L
    private var heardAt = 0L

    // A write in Android's hands (one at a time), and a cancel waiting
    // for it.
    private var writing = false
    private var cancelling = false

    private var pullStarted = false

    @Volatile private var pulled = false

    // Percent of the image the dash has in flash.
    fun percent(): Int {
        val t = transfer ?: return 0
        val size = image?.size ?: return 0
        return (100L * t.have / size).toInt()
    }

    fun start(c: Context) {
        synchronized(LiveLink) {
            val bundled = FirmwareImage.bundled(c)
            if (active || bundled == null || LiveLink.status != LiveLink.Status.LIVE) return
            image = bundled
            transfer = null
            error = null
            pullStarted = false
            pulled = false
            enter(Stage.SAVING_LOG)
            handler.removeCallbacks(tick)
            handler.post(tick)
        }
    }

    // The data log couldn't be saved and the driver said go on.
    fun anyway() {
        synchronized(LiveLink) {
            if (stage != Stage.LOG_NOT_SAVED) return
            pulled = true
            enter(Stage.SAVING_LOG)
        }
    }

    fun cancel() {
        synchronized(LiveLink) {
            if (!active || stage == Stage.INSTALLING) return
            val told = transferring
            stage = Stage.IDLE
            if (!told) return
            if (writing) cancelling = true else LiveLink.writeControl(UpdateCommand.CANCEL, true)
        }
    }

    // An update characteristic's notification.
    fun notified(value: ByteArray) {
        synchronized(LiveLink) {
            val notice = UpdateNotice.decode(value) ?: return
            if (!transferring) return
            heardAt = SystemClock.uptimeMillis()
            when (notice) {
                is UpdateNotice.Accepted -> {
                    val bytes = image?.bytes ?: return
                    if (stage == Stage.WAITING_CLICK) {
                        transfer = Transfer(bytes, (LiveLink.mtu - 3 - 5).coerceAtLeast(1))
                        enter(Stage.SENDING)
                    }
                    transfer?.accepted(notice.have)
                    send()
                }
                UpdateNotice.Installing -> enter(Stage.INSTALLING)
                is UpdateNotice.Failed -> fail(reason(notice.code))
            }
        }
    }

    // The control characteristic's write done (or not).
    fun written(ok: Boolean) {
        synchronized(LiveLink) {
            if (!writing) return
            writing = false
            if (cancelling) {
                cancelling = false
                LiveLink.writeControl(UpdateCommand.CANCEL, true)
                return
            }
            heardAt = SystemClock.uptimeMillis()
            // A lost write: the dash asks for it again ('A').
            if (stage == Stage.SENDING) send()
            if (stage == Stage.WAITING_CLICK && !ok) fail("The dash didn't take the request")
        }
    }

    fun linkLost() {
        synchronized(LiveLink) {
            writing = false
            cancelling = false
            when (stage) {
                Stage.IDLE -> {}
                // It restarts: the wait counts from now.
                Stage.INSTALLING -> heardAt = SystemClock.uptimeMillis()
                else -> fail("The link to the dash dropped: nothing installed")
            }
        }
    }

    // The dash's firmware read, once connected.
    fun dashRead(dash: DashFirmware) {
        synchronized(LiveLink) {
            if (stage != Stage.INSTALLING) return
            if (image?.runsOn(dash) == true) stage = Stage.IDLE else fail("The dash restarted on its old firmware")
        }
    }

    private val tick = object : Runnable {
        override fun run() {
            synchronized(LiveLink) { step(SystemClock.uptimeMillis()) }
            if (active) handler.postDelayed(this, TICK_MS)
        }
    }

    private fun step(now: Long) {
        when (stage) {
            Stage.SAVING_LOG -> saveLog(now)
            Stage.WAITING_CLICK, Stage.SENDING -> {
                if (now - heardAt > QUIET_MS) {
                    fail("The dash went quiet: nothing installed")
                    LiveLink.writeControl(UpdateCommand.CANCEL, true)
                } else if (stage == Stage.SENDING) {
                    send()
                }
            }
            Stage.INSTALLING -> if (now - heardAt > INSTALL_MS) fail("The dash didn't come back after installing: is it on?")
            else -> {}
        }
    }

    // The pull (Recorder's, kept as its own are), then the request, once
    // no pull is under way.
    private fun saveLog(now: Long) {
        val i = image ?: return
        when {
            pulled -> {
                if (!LiveLink.pulling() && LiveLink.writeControl(UpdateCommand.request(i.size, i.crc), true)) {
                    writing = true
                    enter(Stage.WAITING_CLICK)
                }
            }
            !pullStarted -> {
                pullStarted = Recorder.pullNow { pulled = true }
                if (!pullStarted && now - stageAt > PULL_START_MS) enter(Stage.LOG_NOT_SAVED)
            }
            // Over without its bytes: given up.
            !LiveLink.pulling() -> enter(Stage.LOG_NOT_SAVED)
        }
    }

    private fun send() {
        val t = transfer ?: return
        if (writing || cancelling) return
        val from = t.position
        val command = t.next() ?: return
        if (LiveLink.writeControl(command, false)) writing = true else t.rewind(from)
    }

    private fun enter(s: Stage) {
        stage = s
        stageAt = SystemClock.uptimeMillis()
        heardAt = stageAt
    }

    private fun fail(why: String) {
        error = why
        stage = Stage.IDLE
        transfer = null
    }

    private fun reason(code: Int) =
        when (code) {
            1 -> "The dash refused it: no click within 30 s, a click cancelled it, or it was busy with its log"
            2 -> "Too big for the dash's room for it"
            3 -> "The image arrived damaged: nothing installed"
            4 -> "The dash heard nothing from the phone for 10 s and gave up"
            else -> "The dash gave up (error $code)"
        }
}
