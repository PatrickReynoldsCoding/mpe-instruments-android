package com.patrickreynolds.mpeinstruments

import android.media.midi.MidiDevice

/** The C++ side (Synth.cpp). One synth for the whole process. */
object Native {
    init { System.loadLibrary("mpeandroid") }

    external fun nativeStart(): Boolean
    external fun nativeStop()

    external fun nativeAttachMidi(device: MidiDevice, port: Int): Boolean
    external fun nativeDetachMidi()

    external fun nativeInstrumentCount(): Int
    /** field: 0 id, 1 group, 2 name, 3 description */
    external fun nativeInstrumentField(index: Int, field: Int): String

    external fun nativeSetInstrument(index: Int)
    external fun nativeSetVolume(v: Float)
    external fun nativeSetTranspose(semitones: Int)
    external fun nativeSetBendRange(semitones: Float)
    external fun nativeSetWatchdogMs(ms: Int)
    external fun nativePanic()
    external fun nativeResetLatencyTuner()
    external fun nativeStats(): DoubleArray

    data class Instrument(val index: Int, val id: String, val group: String, val name: String, val description: String)

    fun instruments(): List<Instrument> = (0 until nativeInstrumentCount()).map {
        Instrument(it, nativeInstrumentField(it, 0), nativeInstrumentField(it, 1),
                   nativeInstrumentField(it, 2), nativeInstrumentField(it, 3))
    }
}
