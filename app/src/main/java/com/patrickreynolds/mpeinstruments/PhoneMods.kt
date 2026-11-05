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
 * What one gyro source does: a target (GyroSettings.TARGETS index), its amount in the target's
 * unit, and for a turn whether its direction is reversed (anticlockwise, top towards you, left away).
 */
data class GyroSource(val target: Int = 0, val amount: Float = 0f, val invert: Boolean = false)

/**
 * The phone itself as a controller: the gyro (its motion) and breath (the microphone). They change
 * every note of every instrument, played on the pad or on a controller. Not part of a preset,
 * like the pad's layout: it is about how you hold the phone, not about the sound.
 *
 * The gyro has four sources, each driving one target: twist (turned like a steering wheel), tip
 * (the top edge away from you), turn (the right edge away from you) and shake.
 */
data class PhoneMods(
    val gyro: List<GyroSource> = List(4) { GyroSource() },
    val bendSnap: Float = 0f,        // gyro bends pulled onto semitones: 0 not at all .. 1 at once
    val bendReset: Boolean = true,   // a new note (or a finger gliding onto another) zeroes the gyro's bend
    val breath: Int = 0,             // 0 off, 1 swell, 2 tone
    val sensitivity: Float = 0.5f,   // 0 needs a hard blow .. 1 a soft breath
) {
    val gyroOn get() = gyro.any { it.target != 0 }
    fun source(a: Int) = gyro.getOrElse(a) { GyroSource() }
    fun withSource(a: Int, f: (GyroSource) -> GyroSource) = copy(gyro = gyro.mapIndexed { k, g -> if (k == a) f(g) else g })

    fun json() = JSONObject().apply {
        put("gyro", org.json.JSONArray().apply {
            for (g in gyro) put(JSONObject().put("target", g.target).put("amount", g.amount.toDouble()).put("invert", g.invert))
        })
        put("bendSnap", bendSnap.toDouble()); put("bendReset", bendReset)
        put("breath", breath); put("sensitivity", sensitivity.toDouble())
    }

    companion object {
        val SOURCES = listOf("Twist", "Tip", "Turn", "Shake")
        val BREATH = listOf("Off", "Swell", "Tone")

        fun from(o: JSONObject?, d: PhoneMods = PhoneMods()): PhoneMods {
            if (o == null) return d
            val arr = o.optJSONArray("gyro")
            val gyro = if (arr != null) List(4) { k ->
                arr.optJSONObject(k)?.let { g ->
                    val t = g.optInt("target", 0).coerceIn(GyroSettings.TARGETS.indices)
                    GyroSource(t, GyroSettings.clampAmount(t, g.optDouble("amount", GyroSettings.defaultAmount(t).toDouble()).toFloat()),
                               g.optBoolean("invert", false))
                } ?: GyroSource()
            } else {
                // Before the gyro: tilt was the twist, as vibrato (1), bend (2) or tone (3).
                val twist = when (o.optInt("tilt", 0)) {
                    1 -> GyroSource(GyroSettings.VIBRATO, o.optDouble("vibratoCents", 40.0).toFloat().coerceIn(10f, 100f))
                    2 -> GyroSource(GyroSettings.BEND, o.optDouble("bendSemis", 2.0).toFloat().coerceIn(0.5f, 12f))
                    3 -> GyroSource(GyroSettings.TONE, o.optDouble("toneRange", 0.4).toFloat().coerceIn(0.1f, 1f))
                    else -> GyroSource()
                }
                listOf(twist, GyroSource(), GyroSource(), GyroSource())
            }
            return PhoneMods(gyro, o.optDouble("bendSnap", d.bendSnap.toDouble()).toFloat().coerceIn(0f, 1f),
                             o.optBoolean("bendReset", d.bendReset),
                             o.optInt("breath", d.breath).coerceIn(0, 2),
                             o.optDouble("sensitivity", d.sensitivity.toDouble()).toFloat().coerceIn(0f, 1f))
        }
    }
}

/** The gyro's targets, in the engine's order (mpe::GyroTarget), and each one's amount. */
object GyroSettings {
    const val OFF = 0; const val BEND = 1; const val VIBRATO = 2; const val VOLUME = 3; const val TONE = 4
    const val BRIGHTNESS = 5; const val RELEASE = 6; const val REVERB = 7; const val DELAY = 8
    val TARGETS = listOf("Off", "Bend", "Vibrato", "Volume", "Tone", "Brightness", "Release", "Reverb", "Delay")
    val BEND_RANGES = listOf(1f, 2f, 3f, 5f, 7f, 12f)

    fun defaultAmount(t: Int) = when (t) {
        BEND -> 2f; VIBRATO -> 40f; VOLUME -> 0.6f; TONE -> 0.4f; BRIGHTNESS -> 0.7f
        RELEASE -> 2f; REVERB -> 0.5f; DELAY -> 0.5f; else -> 0f
    }
    fun range(t: Int) = when (t) {
        BEND -> 0.5f..12f; VIBRATO -> 10f..100f; RELEASE -> 0.2f..8f; OFF -> 0f..0f; else -> 0.05f..1f
    }
    fun clampAmount(t: Int, a: Float) = range(t).let { r -> a.coerceIn(r.start, r.endInclusive) }
    /** The amount in words: "±2 semitones", "40 cents", "2.0 s", "60%". */
    fun amountName(t: Int, a: Float) = when (t) {
        BEND -> "±%s semitone%s".format(if (a % 1f == 0f) "%d".format(a.toInt()) else "%.1f".format(a), if (a == 1f) "" else "s")
        VIBRATO -> "${a.toInt()} cents"
        RELEASE -> "%.1f s".format(a)
        OFF -> ""
        else -> "${(a * 100).toInt()}%"
    }
}

/** One reading of what the gyro and breath are doing (Native.nativeModifiers; order in Synth::fillModifiers). */
data class ModReading(
    val tilt: Float = 0f,        // the twist's reading: degrees from the zero, or the wobble for vibrato
    val bend: Float = 0f,        // semitones added to every held note
    val slide: Float = 0f,       // added to every note's slide
    val breath: Float = 0f,      // 0..1 as applied
    val held: Boolean = false,   // a finger is down: the gyro's zero is fixed
    val micDb: Float = -120f, val floorDb: Float = -80f, val onDb: Float = -65f, val fullDb: Float = -35f,
    val micLevel: Float = 0f,
    val echoDb: Float = -120f,   // the app's own sound at the microphone, at most: breath must beat it
    val micState: Int = 0,       // 0 off, 1 listening, -1 failed, 2 test level
    val axis: List<Float> = listOf(0f, 0f, 0f, 0f),   // what each gyro source's effect sees (degrees, wobble, shake 0..1)
    val gain: Float = 1f, val bright: Float = 1f,     // the gyro's volume and brightness, as multipliers
    val release: Float = 0f, val reverb: Float = 0f, val delay: Float = 0f,   // added by the gyro
) {
    companion object {
        private val buf = FloatArray(21)
        /** UI thread only (shares one buffer). */
        fun read(): ModReading {
            Native.nativeModifiers(buf)
            val b = buf
            return ModReading(b[0], b[1], b[2], b[3], b[4] > 0.5f, b[5], b[6], b[7], b[8], b[9], b[10], b[11].toInt(),
                              listOf(b[12], b[13], b[14], b[15]), b[16], b[17], b[18], b[19], b[20])
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
