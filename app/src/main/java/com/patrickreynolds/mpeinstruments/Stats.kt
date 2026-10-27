package com.patrickreynolds.mpeinstruments

/** Mirrors Synth::fillStats(). Keep the order in step with Synth.cpp. */
data class Stats(
    val eventAgeAvgMs: Double, val eventAgeMaxMs: Double, val eventAgeLastMs: Double,
    val loadAvg: Double, val loadPeak: Double,
    val voices: Int, val events: Long, val dropped: Long, val panics: Long,
    val sampleRate: Int,
    val outputLatencyMs: Double,          // buffer + hardware, -1 if the stream cannot say
    val bufferFrames: Int, val burstFrames: Int,
    val xruns: Int,
    val exclusive: Boolean,
    val path: Int,                        // 0 OpenSL ES, 1 AAudio, 2 AAudio with MMAP
    val midiBytes: Long,
    val midiState: Int,                   // 0 none, 1 attached, -1 broken
) {
    val pathName get() = when (path) { 2 -> "AAudio MMAP"; 1 -> "AAudio"; else -> "OpenSL ES" }

    /** Time from the message reaching Android to it leaving the speaker's buffer pipeline. */
    val totalMs get() = if (outputLatencyMs >= 0) eventAgeAvgMs + outputLatencyMs else Double.NaN

    companion object {
        fun from(v: DoubleArray) = Stats(
            v[0], v[1], v[2], v[3], v[4], v[5].toInt(), v[6].toLong(), v[7].toLong(), v[8].toLong(),
            v[9].toInt(), v[10], v[11].toInt(), v[12].toInt(), v[13].toInt(), v[14] > 0, v[15].toInt(),
            v[16].toLong(), v[17].toInt())
    }
}
