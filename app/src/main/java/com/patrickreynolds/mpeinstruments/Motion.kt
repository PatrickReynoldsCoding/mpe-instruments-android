package com.patrickreynolds.mpeinstruments

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.hardware.display.DisplayManager
import android.os.Handler
import android.os.HandlerThread
import android.util.Log
import android.view.Display
import android.view.Surface
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import kotlin.math.atan2
import kotlin.math.exp
import kotlin.math.sqrt

/**
 * The phone's motion, for the gyro: how far it is turned about each of the screen's three axes,
 * and how hard it is being shaken. Sent to the engine at about 200 Hz, from a thread of its own;
 * the engine does the rest (the zeros, the wobble, the dead zones, what each source drives).
 *
 *   twist  about the axis out of the screen (a steering wheel), clockwise positive
 *   tip    about the screen's left-right axis, the top edge away from you positive
 *   turn   about the screen's up-down axis, the right edge away from you positive
 *   shake  0..1, from the accelerometer
 *
 * The axes are the screen's as it is shown, so they mean the same in either landscape.
 * Only while the app is on screen: with the screen off the phone is in a pocket or on a stand,
 * and a turn then is not playing.
 */
object Motion : SensorEventListener {
    private const val PERIOD_US = 5000          // 200 Hz, the most Android 12+ gives without a special permission
    private const val SHAKE_FROM = 3f           // m/s² of shaking (gravity taken off) where shake starts..
    private const val SHAKE_FULL = 18f          // ..and where it is full

    private var manager: SensorManager? = null
    private var thread: HandlerThread? = null
    private var kind = 0                        // 1 a rotation vector, 2 the raw gyroscope
    private var accel: Sensor? = null
    private var linear = false                  // the accelerometer already has gravity taken off
    private var resumed = false
    private var listening = false
    private var display: Display? = null

    /** Debug builds: a fixed twist from the tests in place of the sensor's. */
    @Volatile var debugAngle: Float? = null
    private var forced = false

    /** Debug builds: run the gyro although the app is not on screen (tests drive it from adb). */
    fun forceOn(context: Context, on: Boolean) { forced = on; refresh(context) }

    /** null until first tried; false: this phone has no gyroscope. */
    var available by mutableStateOf<Boolean?>(null)
        private set

    // sensor thread only
    private val q = FloatArray(4)
    private val prev = FloatArray(4)
    private var havePrev = false
    private var lastNs = 0L
    private val angle = DoubleArray(3)          // radians about the phone's own x, y, z; unwrapped
    private val grav = FloatArray(3)
    private var shakeEnv = 0f
    private var lastAccNs = 0L
    @Volatile private var rotation = Surface.ROTATION_0
    @Volatile private var invert = BooleanArray(3)   // twist, tip, turn reversed

    fun setResumed(context: Context, on: Boolean) { resumed = on; refresh(context) }

    /** Start or stop listening to match the settings, and tell the engine what is in effect. */
    fun refresh(context: Context) {
        val m = AppState.mods
        val want = (resumed || forced) && m.gyroOn
        if (want && !listening) start(context.applicationContext) else if (!want && listening) stop()
        rotation = display(context)?.rotation ?: Surface.ROTATION_0
        invert = BooleanArray(3) { m.source(it).invert }
        // Not listening: every source off, so nothing is left acting on a stale reading.
        val live = listening || (forced && debugAngle != null)
        Native.nativeSetGyro(IntArray(4) { if (live) m.source(it).target else 0 }, FloatArray(4) { m.source(it).amount },
                             m.bendSnap, m.bendReset)
    }

    private fun display(context: Context): Display? {
        display?.let { return it }
        return context.getSystemService(DisplayManager::class.java)?.getDisplay(Display.DEFAULT_DISPLAY).also { display = it }
    }

    private fun start(context: Context) {
        val sm = context.getSystemService(SensorManager::class.java) ?: return
        // The game rotation vector is the gyroscope and the accelerometer together: smooth, no
        // drift when upright, and no compass jumps. The raw gyroscope otherwise.
        val sensor = sm.getDefaultSensor(Sensor.TYPE_GAME_ROTATION_VECTOR)?.also { kind = 1 }
            ?: sm.getDefaultSensor(Sensor.TYPE_GYROSCOPE)?.also { kind = 2 }
            ?: sm.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR)?.also { kind = 1 }
        available = sensor != null
        if (sensor == null) { Log.w("mpe", "gyro: no rotation sensor on this phone"); return }
        accel = sm.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION)?.also { linear = true }
            ?: sm.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)?.also { linear = false }
        val t = HandlerThread("gyro").apply { start() }
        val h = Handler(t.looper)
        h.post { havePrev = false; lastNs = 0L; angle.fill(0.0); shakeEnv = 0f; lastAccNs = 0L }
        if (!sm.registerListener(this, sensor, PERIOD_US, h)) { t.quitSafely(); available = false; return }
        accel?.let { sm.registerListener(this, it, PERIOD_US, h) }
        Log.i("mpe", "gyro: listening to ${sensor.name}" + (accel?.let { " and ${it.name}" } ?: ""))
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
        Native.nativeSetMotion(0f, 0f, 0f, 0f)
    }

    override fun onSensorChanged(e: SensorEvent) {
        when (e.sensor.type) {
            Sensor.TYPE_LINEAR_ACCELERATION, Sensor.TYPE_ACCELEROMETER -> { shake(e); return }
        }
        if (kind == 1) {
            SensorManager.getQuaternionFromVector(q, e.values)     // w, x, y, z: device to world
            if (havePrev) {
                // The turn since the last reading, in the phone's own frame: conj(prev) * q.
                // A small turn's angle about each axis is 2 atan2(that axis's part, w).
                var w = prev[0] * q[0] + prev[1] * q[1] + prev[2] * q[2] + prev[3] * q[3]
                var x = prev[0] * q[1] - prev[1] * q[0] - prev[2] * q[3] + prev[3] * q[2]
                var y = prev[0] * q[2] + prev[1] * q[3] - prev[2] * q[0] - prev[3] * q[1]
                var z = prev[0] * q[3] - prev[1] * q[2] + prev[2] * q[1] - prev[3] * q[0]
                if (w < 0) { w = -w; x = -x; y = -y; z = -z }
                angle[0] += 2.0 * atan2(x.toDouble(), w.toDouble())
                angle[1] += 2.0 * atan2(y.toDouble(), w.toDouble())
                angle[2] += 2.0 * atan2(z.toDouble(), w.toDouble())
            }
            q.copyInto(prev)
            havePrev = true
        } else {
            if (lastNs != 0L) for (i in 0..2) angle[i] += e.values[i] * (e.timestamp - lastNs) / 1e9
            lastNs = e.timestamp
        }
        send()
    }

    /** Shaking: the acceleration without gravity, as a level that jumps up and falls over ~0.25 s. */
    private fun shake(e: SensorEvent) {
        val dt = if (lastAccNs == 0L) 0f else ((e.timestamp - lastAccNs) / 1e9f).coerceIn(0f, 0.1f)
        lastAccNs = e.timestamp
        var ax = e.values[0]; var ay = e.values[1]; var az = e.values[2]
        if (!linear) {                                             // take gravity off: it is the slow part
            val k = 1 - exp(-dt / 0.3f)
            grav[0] += (ax - grav[0]) * k; grav[1] += (ay - grav[1]) * k; grav[2] += (az - grav[2]) * k
            ax -= grav[0]; ay -= grav[1]; az -= grav[2]
        }
        val mag = sqrt(ax * ax + ay * ay + az * az)
        shakeEnv = if (mag > shakeEnv) mag else shakeEnv * exp(-dt / 0.25f)
    }

    private var sends = 0

    private fun send() {
        // From the phone's axes to the screen's, as it is shown now (looked up a few times a second:
        // the pad turns to landscape after the gyro may have started).
        if (++sends % 50 == 0) display?.let { rotation = it.rotation }
        val dx = angle[0]; val dy = angle[1]
        val (sx, sy) = when (rotation) {
            Surface.ROTATION_90 -> dy to -dx
            Surface.ROTATION_180 -> -dx to -dy
            Surface.ROTATION_270 -> -dy to dx
            else -> dx to dy
        }
        val twist = debugAngle ?: -Math.toDegrees(angle[2]).toFloat()       // clockwise positive
        val tip = -Math.toDegrees(sx).toFloat()                             // top edge away from you positive
        val turn = Math.toDegrees(sy).toFloat()                             // right edge away from you positive
        val shake = ((shakeEnv - SHAKE_FROM) / (SHAKE_FULL - SHAKE_FROM)).coerceIn(0f, 1f)
        val inv = invert
        Native.nativeSetMotion(if (inv[0]) -twist else twist, if (inv[1]) -tip else tip, if (inv[2]) -turn else turn, shake)
    }

    override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) {}
}
