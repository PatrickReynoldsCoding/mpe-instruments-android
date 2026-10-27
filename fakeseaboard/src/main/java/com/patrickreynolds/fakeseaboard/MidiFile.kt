package com.patrickreynolds.fakeseaboard

import java.io.InputStream

/** A channel message and when it happens, in seconds from the start. */
class Timed(val seconds: Double, val bytes: ByteArray)

/** Reads a Standard MIDI File (what `arecordmidi` writes), keeping the real timing. */
object MidiFile {
    fun read(input: InputStream): List<Timed> {
        val d = input.readBytes()
        fun u8(p: Int) = d[p].toInt() and 0xFF
        fun be16(p: Int) = (u8(p) shl 8) or u8(p + 1)
        fun be32(p: Int) = (u8(p) shl 24) or (u8(p + 1) shl 16) or (u8(p + 2) shl 8) or u8(p + 3)
        require(String(d, 0, 4, Charsets.US_ASCII) == "MThd") { "not a MIDI file" }
        val tracks = be16(10)
        val division = be16(12)
        require(division and 0x8000 == 0) { "SMPTE timing not supported" }

        data class Raw(val tick: Long, val bytes: ByteArray)
        val raws = ArrayList<Raw>()
        val tempos = arrayListOf(0L to 500000)
        var p = 8 + be32(4)
        repeat(tracks) {
            if (p + 8 > d.size || String(d, p, 4, Charsets.US_ASCII) != "MTrk") return@repeat
            var q = p + 8
            val end = minOf(d.size, q + be32(p + 4))
            var tick = 0L
            var running = 0
            fun vlq(): Int { var v = 0; while (true) { val b = u8(q++); v = (v shl 7) or (b and 0x7F); if (b and 0x80 == 0 || q >= end) return v } }
            while (q < end) {
                tick += vlq()
                val s = u8(q)
                if (s == 0xFF) {
                    val type = u8(q + 1); q += 2
                    val len = vlq()
                    if (type == 0x51 && len == 3) tempos += tick to ((u8(q) shl 16) or (u8(q + 1) shl 8) or u8(q + 2))
                    q += len
                    continue
                }
                if (s == 0xF0 || s == 0xF7) { q++; q += vlq(); continue }
                if (s and 0x80 != 0) { running = s; q++ }
                if (running == 0) break
                val type = running and 0xF0
                val n = if (type == 0xC0 || type == 0xD0) 1 else 2
                val msg = ByteArray(1 + n)
                msg[0] = running.toByte()
                for (i in 0 until n) msg[1 + i] = d[q++]
                raws += Raw(tick, msg)
            }
            p = end
        }
        raws.sortBy { it.tick }
        tempos.sortBy { it.first }
        val out = ArrayList<Timed>(raws.size)
        var ti = 0
        var secAtTempo = 0.0
        var tickAtTempo = 0L
        var us = tempos[0].second
        for (r in raws) {
            while (ti + 1 < tempos.size && tempos[ti + 1].first <= r.tick) {
                ti++
                secAtTempo += (tempos[ti].first - tickAtTempo).toDouble() * us / 1e6 / division
                tickAtTempo = tempos[ti].first
                us = tempos[ti].second
            }
            out += Timed(secAtTempo + (r.tick - tickAtTempo).toDouble() * us / 1e6 / division, r.bytes)
        }
        return out
    }
}
