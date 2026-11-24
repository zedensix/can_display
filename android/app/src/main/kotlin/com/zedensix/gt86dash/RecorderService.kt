package com.zedensix.gt86dash

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.IBinder

// Keeps the app up while Recorder runs, the phone locked or Android Auto
// gone to another app: a foreground service, its notification saying
// what it does. Its types follow what's granted: the dash's link
// (connected device) always, the GPS (location) while it's on. Android
// may refuse it from the background: Recorder then runs as long as the
// car or the phone's page keeps the app up.
class RecorderService : Service() {
    override fun onBind(intent: Intent?): IBinder? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val gps = intent?.getBooleanExtra(GPS, false) == true && Recorder.locationGranted(this)
        val recording = intent?.getBooleanExtra(RECORDING, false) == true
        val link = ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE
        val note = notification(if (recording) "Recording a drive" else "Pulling the dash's log")
        // The GPS refused (the app in the background): the link alone.
        try {
            startForeground(ID, note, if (gps) link or ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION else link)
        } catch (_: Exception) {
            try {
                startForeground(ID, note, link)
            } catch (_: Exception) {
                stopSelf()
            }
        }
        return START_NOT_STICKY
    }

    private fun notification(text: String): Notification {
        val manager = getSystemService(NotificationManager::class.java)
        manager.createNotificationChannel(NotificationChannel(CHANNEL, "Dash recorder", NotificationManager.IMPORTANCE_LOW))
        val open = PendingIntent.getActivity(this, 0, Intent(this, PreviewActivity::class.java), PendingIntent.FLAG_IMMUTABLE)
        return Notification.Builder(this, CHANNEL)
            .setSmallIcon(R.drawable.ic_launcher_foreground)
            .setContentTitle("GT86 Dash")
            .setContentText(text)
            .setContentIntent(open)
            .setOngoing(true)
            .build()
    }

    companion object {
        private const val ID = 1
        private const val CHANNEL = "recorder"
        private const val GPS = "gps"
        private const val RECORDING = "recording"

        fun start(c: Context, gps: Boolean, recording: Boolean) {
            if (!LiveLink.granted(c)) return
            try {
                c.startForegroundService(Intent(c, RecorderService::class.java).putExtra(GPS, gps).putExtra(RECORDING, recording))
            } catch (_: Exception) {
                // Not allowed from the background (Android 12 on): the car or
                // the phone's page keeps the app up anyway.
            }
        }

        fun stop(c: Context) {
            c.stopService(Intent(c, RecorderService::class.java))
        }
    }
}
