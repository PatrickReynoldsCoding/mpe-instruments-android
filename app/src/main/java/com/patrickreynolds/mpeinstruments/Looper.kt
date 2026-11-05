package com.patrickreynolds.mpeinstruments

/**
 * The looper, in the engine (mpe/looper.h): one button records the first loop (from the first
 * note), then each press layers one more pass over it, in time. Up to 8 layers.
 */
object Looper {
    const val EMPTY = 0; const val ARMED = 1; const val RECORDING = 2; const val PLAYING = 3; const val OVERDUBBING = 4
    const val MAX_LAYERS = 8

    /** One reading of the looper (Synth::fillLoop). */
    data class View(
        val state: Int = EMPTY, val length: Int = 0, val pos: Int = 0, val layers: Int = 0, val paused: Boolean = false,
        val rate: Int = 48000, val level: List<Float> = List(MAX_LAYERS) { 0f },
    ) {
        val seconds get() = length.toFloat() / rate.coerceAtLeast(1)
        /** How far round the loop is, 0..1 (while recording the first: the seconds so far). */
        val progress get() = if (length > 0) pos.toFloat() / length else 0f
        val recordedSeconds get() = pos.toFloat() / rate.coerceAtLeast(1)
    }

    private val buf = FloatArray(14)
    /** UI thread only (shares one buffer). */
    fun read(): View {
        Native.nativeLoopView(buf)
        val b = buf
        return View(b[0].toInt(), b[1].toInt(), b[2].toInt(), b[3].toInt(), b[4] > 0.5f, b[5].toInt(), List(MAX_LAYERS) { b[6 + it] })
    }

    fun press() = Native.nativeLoop(0, 0, 0f, false)
    fun undo() = Native.nativeLoop(1, 0, 0f, false)
    fun clear() = Native.nativeLoop(2, 0, 0f, false)
    fun pause(on: Boolean) = Native.nativeLoop(3, 0, 0f, on)
    fun layer(i: Int, volume: Float, muted: Boolean) = Native.nativeLoop(4, i, volume, muted)
    fun snap(on: Boolean) = Native.nativeLoop(5, 0, 0f, on)
}
