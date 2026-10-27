package com.patrickreynolds.fakeseaboard

import android.util.Log
import kotlin.math.PI
import kotlin.math.sin

/** Plays a timeline of MIDI messages on its own thread, at real time (or faster). */
object Player {
    private const val TAG = FakeSeaboardService.TAG
    @Volatile private var thread: Thread? = null
    @Volatile private var stopRequested = false
    @Volatile private var dropOffs = false
    private val held = IntArray(16) { -1 }           // note held per channel, so stop() can end them

    fun play(timeline: List<Timed>, speed: Double = 1.0, label: String) {
        stop(releaseNotes = true)
        stopRequested = false
        dropOffs = false
        thread = Thread({
            val svc = FakeSeaboardService.instance
            if (svc == null) { Log.w(TAG, "no app has the device open: nothing to play to"); return@Thread }
            Log.i(TAG, "PLAY start $label: ${timeline.size} messages, ${"%.1f".format(timeline.lastOrNull()?.seconds ?: 0.0)} s, speed $speed")
            val t0 = System.nanoTime()
            var sent = 0
            for (m in timeline) {
                if (stopRequested) break
                val due = t0 + (m.seconds / speed * 1e9).toLong()
                while (true) {
                    val wait = due - System.nanoTime()
                    if (wait <= 0 || stopRequested) break
                    if (wait > 2_000_000) Thread.sleep(1) else Thread.yield()
                }
                if (stopRequested) break
                track(m.bytes)
                svc.send(m.bytes)
                sent++
            }
            Log.i(TAG, "PLAY end $label: sent $sent messages in ${"%.2f".format((System.nanoTime() - t0) / 1e9)} s${if (stopRequested) " (stopped)" else ""}")
        }, "fake-seaboard-player").apply { priority = Thread.MAX_PRIORITY; start() }
    }

    private fun track(b: ByteArray) {
        val type = b[0].toInt() and 0xF0
        val ch = b[0].toInt() and 0x0F
        val vel = if (b.size > 2) b[2].toInt() else 0
        if (type == 0x90 && vel > 0) held[ch] = b[1].toInt()
        else if (type == 0x80 || (type == 0x90 && vel == 0)) held[ch] = -1
    }

    /**
     * releaseNotes = true: end held notes like a player lifting their fingers.
     * releaseNotes = false: just go silent mid-note, like a Bluetooth link that drops.
     */
    fun stop(releaseNotes: Boolean) {
        stopRequested = true
        thread?.join(500)
        thread = null
        val svc = FakeSeaboardService.instance ?: return
        for (ch in 0 until 16) {
            if (held[ch] >= 0 && releaseNotes) svc.send(byteArrayOf((0x80 or ch).toByte(), held[ch].toByte(), 0))
            held[ch] = -1
        }
    }

    /**
     * A synthetic gesture with known pitch, for checking the app's audio:
     * note on with velocity, a pressure swell, vibrato with pitch bend (MPE: +/-48 semitones
     * = full scale), then note off, unless holdForever.
     */
    fun gesture(channel: Int, note: Int, seconds: Double, vibratoCents: Double, holdForever: Boolean): List<Timed> {
        val out = ArrayList<Timed>()
        val ch = channel and 0x0F
        out += Timed(0.0, byteArrayOf((0xB0 or ch).toByte(), 74, 64))
        out += Timed(0.0, byteArrayOf((0x90 or ch).toByte(), note.toByte(), 40))
        var t = 0.0
        while (t < seconds) {
            val pressure = (30 + 80 * minOf(1.0, t / 0.3)).toInt().coerceIn(0, 127)
            out += Timed(t, byteArrayOf((0xD0 or ch).toByte(), pressure.toByte()))
            val cents = if (t > 0.5) vibratoCents * sin(2 * PI * 5.5 * t) else 0.0
            val v = (8192 + cents / 100.0 / 48.0 * 8192).toInt().coerceIn(0, 16383)
            out += Timed(t, byteArrayOf((0xE0 or ch).toByte(), (v and 0x7F).toByte(), ((v shr 7) and 0x7F).toByte()))
            t += 0.005                                   // 200 updates per second, like the Block
        }
        if (!holdForever) out += Timed(seconds, byteArrayOf((0x80 or ch).toByte(), note.toByte(), 0))
        return out
    }
}
