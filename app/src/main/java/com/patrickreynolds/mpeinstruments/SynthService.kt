package com.patrickreynolds.mpeinstruments

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.IBinder

/** Keeps the process, and so the audio stream and the MIDI port, alive with the screen off. */
class SynthService : Service() {
    override fun onBind(intent: Intent?): IBinder? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (intent?.action == ACTION_STOP) { stop(this); return START_NOT_STICKY }
        val nm = getSystemService(NotificationManager::class.java)
        nm.createNotificationChannel(NotificationChannel(CHANNEL, "Playing", NotificationManager.IMPORTANCE_LOW))
        val open = PendingIntent.getActivity(this, 0, Intent(this, MainActivity::class.java),
                                             PendingIntent.FLAG_IMMUTABLE)
        val notification = Notification.Builder(this, CHANNEL)
            .setContentTitle("MPE Instruments")
            .setContentText("Ready to play")
            .setSmallIcon(R.drawable.ic_notification)
            .setContentIntent(open)
            .addAction(Notification.Action.Builder(null, "Stop",
                PendingIntent.getService(this, 1, Intent(this, SynthService::class.java).setAction(ACTION_STOP),
                                         PendingIntent.FLAG_IMMUTABLE)).build())
            .setOngoing(true)
            .build()
        startForeground(1, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK)
        Native.nativeStart()
        AppState.init(applicationContext)            // the saved instruments and settings, also when started without the screen
        MidiConnector.init(applicationContext)
        return START_NOT_STICKY
    }

    override fun onDestroy() {
        MidiConnector.shutdown()
        Native.nativeStop()
        super.onDestroy()
    }

    companion object {
        private const val CHANNEL = "playing"
        private const val ACTION_STOP = "com.patrickreynolds.mpeinstruments.STOP"
        /** Called when the user stops the app; the screen listens to close itself. */
        var onStopped: (() -> Unit)? = null

        /** Everything off: no sound engine, no controller, no Bluetooth search, until the app is opened again. */
        fun stop(service: Service) {
            service.stopForeground(STOP_FOREGROUND_REMOVE)
            service.stopSelf()
            onStopped?.invoke()
        }
    }
}
