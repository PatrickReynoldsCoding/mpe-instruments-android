package com.patrickreynolds.mpeinstruments

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.os.Handler
import android.os.Looper
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.setValue
import org.json.JSONObject
import java.util.concurrent.Executors

/**
 * The phone itself as a controller: tilt (the gyro) and breath (the microphone). They change
 * every note of every instrument, played on the pad or on a controller. Not part of a preset,
 * like the pad's layout: it is about how you hold the phone, not about the sound.
 */
data class PhoneMods(
    val tilt: Int = 0,               // 0 off, 1 vibrato, 2 bend, 3 tone
    val vibratoCents: Float = 40f,   // vibrato depth at a firm wobble (3.5 degrees)
    val bendSemis: Float = 2f,       // bend at a 30 degree turn
    val toneRange: Float = 0.4f,     // tone change (slide) at a 30 degree turn
    val breath: Int = 0,             // 0 off, 1 swell, 2 tone
    val sensitivity: Float = 0.5f,   // 0 needs a hard blow .. 1 a soft breath
) {
    /** The engine's amount for the tilt mode, in that mode's unit. */
    val tiltAmount get() = when (tilt) { 1 -> vibratoCents; 2 -> bendSemis; 3 -> toneRange; else -> 0f }

    fun json() = JSONObject().apply {
        put("tilt", tilt); put("vibratoCents", vibratoCents.toDouble()); put("bendSemis", bendSemis.toDouble())
        put("toneRange", toneRange.toDouble()); put("breath", breath); put("sensitivity", sensitivity.toDouble())
    }

    companion object {
        val TILT = listOf("Off", "Vibrato", "Bend", "Tone")
        val BREATH = listOf("Off", "Swell", "Tone")
        val BEND_RANGES = listOf(1f, 2f, 3f, 5f, 7f, 12f)
        fun from(o: JSONObject?, d: PhoneMods = PhoneMods()) = if (o == null) d else PhoneMods(
            o.optInt("tilt", d.tilt).coerceIn(0, 3),
            o.optDouble("vibratoCents", d.vibratoCents.toDouble()).toFloat().coerceIn(10f, 100f),
            o.optDouble("bendSemis", d.bendSemis.toDouble()).toFloat().coerceIn(0.5f, 12f),
            o.optDouble("toneRange", d.toneRange.toDouble()).toFloat().coerceIn(0.1f, 1f),
            o.optInt("breath", d.breath).coerceIn(0, 2),
            o.optDouble("sensitivity", d.sensitivity.toDouble()).toFloat().coerceIn(0f, 1f))
    }
}

/** One reading of what tilt and breath are doing (Native.nativeModifiers; order in Synth::fillModifiers). */
data class ModReading(
    val tilt: Float = 0f,        // degrees the effect sees: the wobble (vibrato) or the turn from the zero (bend, tone)
    val bend: Float = 0f,        // semitones added to every held note
    val slide: Float = 0f,       // added to every note's slide
    val breath: Float = 0f,      // 0..1 as applied
    val held: Boolean = false,   // a finger is down: the tilt zero is fixed
    val micDb: Float = -120f, val floorDb: Float = -80f, val onDb: Float = -65f, val fullDb: Float = -35f,
    val micLevel: Float = 0f,
    val echoDb: Float = -120f,   // the app's own sound at the microphone, at most: breath must beat it
    val micState: Int = 0,       // 0 off, 1 listening, -1 failed, 2 test level
) {
    companion object {
        private val buf = FloatArray(12)
        /** UI thread only (shares one buffer). */
        fun read(): ModReading {
            Native.nativeModifiers(buf)
            val b = buf
            return ModReading(b[0], b[1], b[2], b[3], b[4] > 0.5f, b[5], b[6], b[7], b[8], b[9], b[10], b[11].toInt())
        }
    }
}

/** Sends the settings to the engine, and opens or closes the microphone (off the UI thread). */
object PhoneControl {
    private val worker = Executors.newSingleThreadExecutor { r -> Thread(r, "mic-control") }
    private val main = Handler(Looper.getMainLooper())
    /** 0 off, 1 listening, -1 the microphone could not be opened, -2 breath is on but the permission is missing. */
    var micState by mutableIntStateOf(0)
        private set
    /** Debug builds: run breath without the microphone permission (the level comes from DebugReceiver). */
    var forceBreath = false

    fun hasMicPermission(context: Context) =
        context.checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED

    fun apply(context: Context, m: PhoneMods) {
        Motion.refresh(context)
        val allowed = forceBreath || hasMicPermission(context)
        val mode = if (m.breath != 0 && allowed) m.breath else 0
        val sens = m.sensitivity
        worker.execute {
            val st = Native.nativeSetBreath(mode, sens)
            main.post { micState = if (m.breath != 0 && !allowed) -2 else st }
        }
    }
}
