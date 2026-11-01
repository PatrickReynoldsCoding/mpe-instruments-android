package com.patrickreynolds.mpeinstruments

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.os.Handler
import android.os.HandlerThread
import android.util.Log
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import kotlin.math.atan2

/**
 * The phone's twist, for tilt: how far it is turned like a steering wheel (about the axis out
 * of the screen), whichever way it is held. Sent to the engine at about 200 Hz, from a thread of
 * its own; the engine does the rest (the zero, the wobble, the dead zones).
 *
 * Only while the app is on screen: with the screen off the phone is in a pocket or on a stand,
 * and a turn then is not playing.
 */
object Motion : SensorEventListener {
    private const val PERIOD_US = 5000          // 200 Hz, the most Android 12+ gives without a special permission

    private var manager: SensorManager? = null
    private var thread: HandlerThread? = null
    private var kind = 0                        // 1 a rotation vector, 2 the raw gyroscope
    private var resumed = false
    private var listening = false

    /** Debug builds: a fixed angle from the tests in place of the sensor's. */
    @Volatile var debugAngle: Float? = null
    private var forced = false

    /** Debug builds: run tilt although the app is not on screen (tests drive it from adb). */
    fun forceOn(context: Context, on: Boolean) { forced = on; refresh(context) }

    /** null until first tried; false: this phone has no gyroscope. */
    var available by mutableStateOf<Boolean?>(null)
        private set

    // sensor thread only
    private val q = FloatArray(4)
    private val prev = FloatArray(4)
    private var havePrev = false
    private var lastNs = 0L
    private var angle = 0.0                     // radians, counter-clockwise positive, unwrapped

    fun setResumed(context: Context, on: Boolean) { resumed = on; refresh(context) }

    /** Start or stop listening to match the setting, and tell the engine the mode in effect. */
    fun refresh(context: Context) {
        val m = AppState.mods
        val want = (resumed || forced) && m.tilt != 0
        if (want && !listening) start(context.applicationContext) else if (!want && listening) stop()
        Native.nativeSetTiltMode(if (listening || (forced && debugAngle != null)) m.tilt else 0, m.tiltAmount)
    }

    private fun start(context: Context) {
        val sm = context.getSystemService(SensorManager::class.java) ?: return
        // The game rotation vector is the gyroscope and the accelerometer together: smooth, no
        // drift when upright, and no compass jumps. The raw gyroscope otherwise.
        val sensor = sm.getDefaultSensor(Sensor.TYPE_GAME_ROTATION_VECTOR)?.also { kind = 1 }
            ?: sm.getDefaultSensor(Sensor.TYPE_GYROSCOPE)?.also { kind = 2 }
            ?: sm.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR)?.also { kind = 1 }
        available = sensor != null
        if (sensor == null) { Log.w("mpe", "tilt: no rotation sensor on this phone"); return }
        val t = HandlerThread("tilt").apply { start() }
        Handler(t.looper).post { havePrev = false; lastNs = 0L }
        if (!sm.registerListener(this, sensor, PERIOD_US, Handler(t.looper))) { t.quitSafely(); available = false; return }
        Log.i("mpe", "tilt: listening to ${sensor.name}")
        manager = sm
        thread = t
        listening = true
    }

    private fun stop() {
        manager?.unregisterListener(this)
        thread?.quitSafely()
        manager = null
        thread = null
        listening = false
    }

    override fun onSensorChanged(e: SensorEvent) {
        if (kind == 1) {
            SensorManager.getQuaternionFromVector(q, e.values)     // w, x, y, z: device to world
            if (havePrev) {
                // The turn since the last reading, in the phone's own frame: conj(prev) * q.
                // Its twist about the screen's axis (z) is 2 atan2(z, w).
                var w = prev[0] * q[0] + prev[1] * q[1] + prev[2] * q[2] + prev[3] * q[3]
                var z = prev[0] * q[3] - prev[1] * q[2] + prev[2] * q[1] - prev[3] * q[0]
                if (w < 0) { w = -w; z = -z }
                angle += 2.0 * atan2(z.toDouble(), w.toDouble())
            }
            q.copyInto(prev)
            havePrev = true
        } else {
            if (lastNs != 0L) angle += e.values[2] * (e.timestamp - lastNs) / 1e9
            lastNs = e.timestamp
        }
        if (debugAngle == null) Native.nativeSetTilt(-Math.toDegrees(angle).toFloat())     // clockwise positive
    }

    override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) {}
}
