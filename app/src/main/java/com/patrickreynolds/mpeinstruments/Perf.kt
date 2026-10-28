package com.patrickreynolds.mpeinstruments

import org.json.JSONObject

/** One part (left or right of a split, or a layer): mirrors mpe::PartConfig. */
data class Part(
    val instrument: String = "strings/cello",
    val arp: Boolean = false,
    val order: Int = 0,            // ArpOrder: 0 up, 1 down, 2 up-down, 3 as played, 4 random, 5 chord
    val rate: Float = 4f,          // steps per beat
    val gate: Float = 0.6f,
    val octaves: Int = 1,
    val latch: Boolean = false,
    val transpose: Int = 0,
    val volume: Float = 1f,
    val brightness: Float = 1f,
    val drive: Float = 0f,
    val pressureCurve: Float = 1f,
    val reverb: Float = 0.15f,
    val delay: Float = 0f,
) {
    fun json() = JSONObject().apply {
        put("instrument", instrument); put("arp", arp); put("order", order); put("rate", rate.toDouble())
        put("gate", gate.toDouble()); put("octaves", octaves); put("latch", latch); put("transpose", transpose)
        put("volume", volume.toDouble()); put("brightness", brightness.toDouble()); put("drive", drive.toDouble())
        put("pressureCurve", pressureCurve.toDouble()); put("reverb", reverb.toDouble()); put("delay", delay.toDouble())
    }

    companion object {
        fun from(o: JSONObject?, d: Part = Part()): Part = if (o == null) d else Part(
            o.optString("instrument", d.instrument), o.optBoolean("arp", d.arp), o.optInt("order", d.order),
            o.optDouble("rate", d.rate.toDouble()).toFloat(), o.optDouble("gate", d.gate.toDouble()).toFloat(),
            o.optInt("octaves", d.octaves), o.optBoolean("latch", d.latch), o.optInt("transpose", d.transpose),
            o.optDouble("volume", d.volume.toDouble()).toFloat(), o.optDouble("brightness", d.brightness.toDouble()).toFloat(),
            o.optDouble("drive", d.drive.toDouble()).toFloat(), o.optDouble("pressureCurve", d.pressureCurve.toDouble()).toFloat(),
            o.optDouble("reverb", d.reverb.toDouble()).toFloat(), o.optDouble("delay", d.delay.toDouble()).toFloat())
    }
}

/** Everything a preset holds: mirrors mpe::Performance, plus each part's instrument. */
data class Perf(
    val layout: Int = 0,           // 0 single, 1 split, 2 layer
    val splitNote: Int = 60,
    val bpm: Float = 110f,
    val scale: Int = 0,            // index into SCALES; 0 = off
    val scaleRoot: Int = 0,
    val assist: Float = 0.6f,
    val reverbSize: Float = 0.5f,
    val reverbDamp: Float = 0.4f,
    val delayBeats: Float = 0.75f,
    val delayFeedback: Float = 0.35f,
    val delayPingPong: Boolean = true,
    val parts: List<Part> = listOf(Part(), Part(instrument = "voices/soprano")),
) {
    fun part(i: Int) = parts[i]
    fun withPart(i: Int, f: (Part) -> Part) = copy(parts = parts.mapIndexed { k, p -> if (k == i) f(p) else p })

    /** The layout nativeSetPerformance (jni.cpp) reads. Keep the two in step. */
    fun toArray(): FloatArray {
        val a = FloatArray(11 + 2 * 13)
        a[0] = layout.toFloat(); a[1] = splitNote.toFloat(); a[2] = bpm
        a[3] = SCALES[scale.coerceIn(SCALES.indices)].mask.toFloat(); a[4] = scaleRoot.toFloat()
        a[5] = if (scale == 0) 0f else assist
        a[6] = reverbSize; a[7] = reverbDamp; a[8] = delayBeats; a[9] = delayFeedback; a[10] = if (delayPingPong) 1f else 0f
        for (k in 0..1) {
            val p = parts[k]
            val b = 11 + 13 * k
            a[b] = if (p.arp) 1f else 0f; a[b + 1] = p.order.toFloat(); a[b + 2] = p.rate; a[b + 3] = p.gate
            a[b + 4] = p.octaves.toFloat(); a[b + 5] = if (p.latch) 1f else 0f; a[b + 6] = p.transpose.toFloat()
            a[b + 7] = p.volume; a[b + 8] = p.brightness; a[b + 9] = p.drive; a[b + 10] = p.pressureCurve
            a[b + 11] = p.reverb; a[b + 12] = p.delay
        }
        return a
    }

    fun json(): JSONObject = JSONObject().apply {
        put("layout", layout); put("splitNote", splitNote); put("bpm", bpm.toDouble()); put("scale", scale)
        put("scaleRoot", scaleRoot); put("assist", assist.toDouble()); put("reverbSize", reverbSize.toDouble())
        put("reverbDamp", reverbDamp.toDouble()); put("delayBeats", delayBeats.toDouble())
        put("delayFeedback", delayFeedback.toDouble()); put("delayPingPong", delayPingPong)
        put("part0", parts[0].json()); put("part1", parts[1].json())
    }

    companion object {
        fun from(o: JSONObject?): Perf {
            val d = Perf()
            if (o == null) return d
            return Perf(
                o.optInt("layout", d.layout), o.optInt("splitNote", d.splitNote), o.optDouble("bpm", d.bpm.toDouble()).toFloat(),
                o.optInt("scale", d.scale), o.optInt("scaleRoot", d.scaleRoot), o.optDouble("assist", d.assist.toDouble()).toFloat(),
                o.optDouble("reverbSize", d.reverbSize.toDouble()).toFloat(), o.optDouble("reverbDamp", d.reverbDamp.toDouble()).toFloat(),
                o.optDouble("delayBeats", d.delayBeats.toDouble()).toFloat(), o.optDouble("delayFeedback", d.delayFeedback.toDouble()).toFloat(),
                o.optBoolean("delayPingPong", d.delayPingPong),
                listOf(Part.from(o.optJSONObject("part0"), d.parts[0]), Part.from(o.optJSONObject("part1"), d.parts[1])))
        }
    }
}

/** The on-screen pad: columns of notes left to right, up and down for loudness and tone. */
data class PadSettings(
    val size: Int = 8,             // columns (notes) across the screen
    val octave: Int = 0,           // 0 = the middle of the instrument's range
    val key: Int = 0,              // 0 = C
    val scale: Int = 1,            // index into SCALES; 0 (Off) plays every semitone
    val vertical: Int = 0,         // up/down: 0 loudness and tone, 1 tone, 2 loudness
    val snap: Boolean = true,      // start on the column's note and settle back to it when resting
) {
    fun json() = JSONObject().apply {
        put("size", size); put("octave", octave); put("key", key); put("scale", scale)
        put("vertical", vertical); put("snap", snap)
    }

    companion object {
        val VERTICAL = listOf("Loudness + tone", "Tone", "Loudness")
        fun from(o: JSONObject?, d: PadSettings = PadSettings()) = if (o == null) d else PadSettings(
            o.optInt("size", d.size).coerceIn(3, 24), o.optInt("octave", d.octave).coerceIn(-4, 4),
            o.optInt("key", d.key).coerceIn(0, 11), o.optInt("scale", d.scale).coerceIn(SCALES.indices),
            o.optInt("vertical", d.vertical).coerceIn(0, 2), o.optBoolean("snap", d.snap))
    }
}

data class Scale(val name: String, val mask: Int)

/** Bit n = the note n semitones above the root is in the scale. */
val SCALES = listOf(
    Scale("Off", 0),
    Scale("Major", 0b101010110101),
    Scale("Minor", 0b010110101101),
    Scale("Dorian", 0b011010101101),
    Scale("Mixolydian", 0b011010110101),
    Scale("Harmonic minor", 0b100110101101),
    Scale("Pentatonic major", 0b001010010101),
    Scale("Pentatonic minor", 0b010010101001),
    Scale("Blues", 0b010011101001),
    Scale("Chromatic", 0b111111111111),
)

val NOTE_NAMES = listOf("C", "C♯", "D", "E♭", "E", "F", "F♯", "G", "A♭", "A", "B♭", "B")
fun noteName(n: Int) = NOTE_NAMES[((n % 12) + 12) % 12] + (n / 12 - 1)

val ARP_ORDERS = listOf("Up", "Down", "Up-down", "As played", "Random", "Chord")
/** Arp rates as (label, steps per beat). */
val ARP_RATES = listOf("1/4" to 1f, "1/8" to 2f, "1/8T" to 3f, "1/16" to 4f, "1/16T" to 6f, "1/32" to 8f)
/** Delay times as (label, beats). */
val DELAY_TIMES = listOf("1/16" to 0.25f, "1/8" to 0.5f, "1/8." to 0.75f, "1/4" to 1f, "1/4." to 1.5f, "1/2" to 2f)
