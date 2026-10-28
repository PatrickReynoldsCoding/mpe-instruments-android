package com.patrickreynolds.mpeinstruments

import android.media.midi.MidiDevice

/** The C++ side (Synth.cpp). One synth for the whole process. */
object Native {
    init { System.loadLibrary("mpeandroid") }

    external fun nativeStart(): Boolean
    external fun nativeStop()

    external fun nativeAttachMidi(device: MidiDevice, port: Int): Boolean
    external fun nativeDetachMidi()
    /** One MIDI message from the on-screen pad (UI thread only). */
    external fun nativeTouch(status: Int, d1: Int, d2: Int)

    external fun nativeInstrumentCount(): Int
    /** field: 0 id, 1 group, 2 name, 3 description, 4 range ("low high" in Hz, or "") */
    external fun nativeInstrumentField(index: Int, field: Int): String

    external fun nativeSetInstrument(index: Int)
    external fun nativeSetPartInstrument(part: Int, index: Int)
    /** Perf.toArray() */
    external fun nativeSetPerformance(perf: FloatArray)
    /** Sounding voices, 8 floats each (part, finger, note, gate, pitch, pressure, slide, level); returns the count. */
    external fun nativeVoices(out: FloatArray): Int
    /** Held fingers, 5 floats each (channel, note, pitch, pressure, slide); returns the count. */
    external fun nativeFingers(out: FloatArray): Int
    external fun nativeSetVolume(v: Float)
    external fun nativeSetTranspose(semitones: Int)
    external fun nativeSetBendRange(semitones: Float)
    external fun nativeSetWatchdogMs(ms: Int)
    external fun nativePanic()
    external fun nativeResetLatencyTuner()
    external fun nativeStats(): DoubleArray

    // Debug: record the audio output (used by the emulator tests).
    external fun nativeCaptureStart(seconds: Float): Boolean
    external fun nativeCaptureProgress(): Double
    external fun nativeCaptureSave(path: String): Boolean

    data class Instrument(val index: Int, val id: String, val group: String, val name: String, val description: String,
                          val lowHz: Float, val highHz: Float) {
        /** The middle of the instrument's range as a MIDI note, for the octave check. */
        val centerNote: Float get() = if (lowHz > 0 && highHz > 0)
            69f + 12f * (kotlin.math.ln(kotlin.math.sqrt(lowHz * highHz) / 440f) / kotlin.math.ln(2f)) else 60f
    }

    fun instruments(): List<Instrument> = (0 until nativeInstrumentCount()).map {
        val range = nativeInstrumentField(it, 4).trim().split(Regex("\\s+")).mapNotNull { v -> v.toFloatOrNull() }
        Instrument(it, nativeInstrumentField(it, 0), nativeInstrumentField(it, 1),
                   nativeInstrumentField(it, 2), nativeInstrumentField(it, 3),
                   range.getOrElse(0) { 0f }, range.getOrElse(1) { 0f })
    }
}
