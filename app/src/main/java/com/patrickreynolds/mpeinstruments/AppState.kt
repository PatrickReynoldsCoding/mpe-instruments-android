package com.patrickreynolds.mpeinstruments

import android.content.Context
import android.content.SharedPreferences
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import org.json.JSONArray
import org.json.JSONObject

data class Preset(val name: String, val perf: Perf, val builtIn: Boolean = false)

/**
 * What the screen shows and the engine plays. Every change goes to the engine at once and is
 * saved, so the app (and the background service) start where they left off.
 */
object AppState {
    private lateinit var prefs: SharedPreferences
    private var started = false

    var instruments: List<Native.Instrument> = emptyList()
        private set
    var perf by mutableStateOf(Perf())
        private set
    var volume by mutableStateOf(0.8f)
        private set
    var transpose by mutableStateOf(0)
        private set
    var bendRange by mutableStateOf(48f)
        private set
    var presetName by mutableStateOf("")
        private set
    var userPresets by mutableStateOf<List<Preset>>(emptyList())
        private set
    /** Set the split point from the next note played. */
    var learningSplit by mutableStateOf(false)

    fun init(context: Context) {
        if (started) return
        started = true
        prefs = context.applicationContext.getSharedPreferences("mpe", Context.MODE_PRIVATE)
        instruments = Native.instruments()
        perf = runCatching { Perf.from(prefs.getString("perf", null)?.let { JSONObject(it) }) }.getOrDefault(Perf())
        // Before presets existed the instrument was saved as an index.
        if (!prefs.contains("perf") && prefs.contains("instrument"))
            instruments.getOrNull(prefs.getInt("instrument", -1))?.let { inst -> perf = perf.withPart(0) { it.copy(instrument = inst.id) } }
        volume = prefs.getFloat("volume", 0.8f)
        transpose = prefs.getInt("transpose", 0)
        bendRange = prefs.getFloat("bend", 48f)
        presetName = prefs.getString("preset", "") ?: ""
        userPresets = runCatching {
            val a = JSONArray(prefs.getString("presets", "[]"))
            (0 until a.length()).map { a.getJSONObject(it).let { o -> Preset(o.getString("name"), Perf.from(o.getJSONObject("perf"))) } }
        }.getOrDefault(emptyList())
        applyAll()
    }

    /** Send everything to the engine (at start, and after the audio engine was rebuilt). */
    fun applyAll() {
        Native.nativeSetVolume(volume)
        Native.nativeSetTranspose(transpose)
        Native.nativeSetBendRange(bendRange)
        applyInstruments(null)
        Native.nativeSetPerformance(perf.toArray())
    }

    private fun index(id: String) = instruments.indexOfFirst { it.id == id }.takeIf { it >= 0 }
        ?: instruments.indexOfFirst { it.id == "strings/cello" }.coerceAtLeast(0)

    fun instrument(part: Int) = instruments.getOrNull(index(perf.part(part).instrument))

    private fun applyInstruments(old: Perf?) {
        for (k in 0..1) {
            val id = perf.part(k).instrument
            // Part 1 is only built when a split or layer needs it: 15 more copies of an instrument.
            if (k == 1 && perf.layout == 0) continue
            if (old == null || old.part(k).instrument != id || (k == 1 && old.layout == 0))
                Native.nativeSetPartInstrument(k, index(id))
        }
    }

    fun update(change: (Perf) -> Perf) {
        val old = perf
        perf = change(perf)
        if (perf == old) return
        applyInstruments(old)
        Native.nativeSetPerformance(perf.toArray())
        presetName = presetName.takeIf { matchesPreset(it) } ?: ""
        save()
    }

    fun updatePart(i: Int, change: (Part) -> Part) = update { it.withPart(i, change) }

    fun changeVolume(v: Float) { volume = v; Native.nativeSetVolume(v); prefs.edit().putFloat("volume", v).apply() }
    fun changeTranspose(v: Int) {
        transpose = v.coerceIn(-60, 60)
        Native.nativeSetTranspose(transpose)
        prefs.edit().putInt("transpose", transpose).apply()
    }
    fun changeBendRange(v: Float) { bendRange = v; Native.nativeSetBendRange(v); prefs.edit().putFloat("bend", v).apply() }

    // ---- presets ----------------------------------------------------------------------------

    val allPresets get() = BUILT_IN_PRESETS + userPresets

    private fun matchesPreset(name: String) = allPresets.firstOrNull { it.name == name }?.perf == perf

    fun load(p: Preset) {
        update { p.perf }
        presetName = p.name
        prefs.edit().putString("preset", presetName).apply()
    }

    fun saveAs(name: String) {
        val clean = name.trim().ifEmpty { return }
        userPresets = userPresets.filter { it.name != clean } + Preset(clean, perf)
        presetName = clean
        savePresets()
    }

    fun delete(p: Preset) {
        if (p.builtIn) return
        userPresets = userPresets.filter { it.name != p.name }
        if (presetName == p.name) presetName = ""
        savePresets()
    }

    private fun savePresets() {
        val a = JSONArray()
        for (p in userPresets) a.put(JSONObject().put("name", p.name).put("perf", p.perf.json()))
        prefs.edit().putString("presets", a.toString()).putString("preset", presetName).apply()
    }

    // A slider drag changes the performance many times a second: save once it settles.
    private val saver = android.os.Handler(android.os.Looper.getMainLooper())
    private val saveNow = Runnable { prefs.edit().putString("perf", perf.json().toString()).putString("preset", presetName).apply() }
    private fun save() { saver.removeCallbacks(saveNow); saver.postDelayed(saveNow, 400) }
}

private fun preset(name: String, perf: Perf) = Preset(name, perf, builtIn = true)
private fun two(a: Part, b: Part = Part(instrument = "voices/soprano")) = listOf(a, b)

/** A starting point for each kind of playing. */
val BUILT_IN_PRESETS = listOf(
    preset("Solo cello", Perf(parts = two(Part("strings/cello", reverb = 0.25f)), reverbSize = 0.6f)),
    preset("Violin, in tune", Perf(scale = 1, assist = 0.7f, parts = two(Part("strings/violin", reverb = 0.25f)))),
    preset("Breathy flute", Perf(parts = two(Part("winds/flute", reverb = 0.35f, pressureCurve = 0.7f)), reverbSize = 0.75f)),
    preset("Choir layer", Perf(layout = 2, parts = listOf(
        Part("strings/cello", reverb = 0.3f, volume = 0.8f),
        Part("voices/soprano", transpose = 12, reverb = 0.45f, volume = 0.6f)), reverbSize = 0.8f)),
    preset("Bass + lead split", Perf(layout = 1, splitNote = 60, parts = listOf(
        Part("voices/bass", transpose = -12, reverb = 0.1f),
        Part("synths/lead", reverb = 0.2f, delay = 0.25f)))),
    preset("Arp left, cello right", Perf(layout = 1, splitNote = 60, bpm = 100f, parts = listOf(
        Part("struck/marimba", arp = true, order = 2, rate = 4f, gate = 0.5f, octaves = 2, latch = true, reverb = 0.2f, delay = 0.2f),
        Part("strings/cello", reverb = 0.3f)))),
    preset("Pluck arp", Perf(bpm = 118f, delayBeats = 0.75f, delayFeedback = 0.4f, parts = two(
        Part("plucked/steel-guitar", arp = true, order = 0, rate = 4f, gate = 0.4f, octaves = 2, reverb = 0.2f, delay = 0.35f)))),
    preset("Dreamy pad", Perf(scale = 6, assist = 0.5f, reverbSize = 0.95f, parts = two(
        Part("synths/pad", brightness = 0.7f, reverb = 0.6f, delay = 0.15f, pressureCurve = 0.8f)))),
    preset("Gritty lead", Perf(bpm = 124f, parts = two(Part("synths/lead", drive = 0.5f, brightness = 0.85f, reverb = 0.15f, delay = 0.3f)))),
    preset("Vibraphone, latched", Perf(bpm = 90f, parts = two(
        Part("struck/vibraphone", arp = true, order = 3, rate = 3f, gate = 0.8f, latch = true, reverb = 0.4f)))),
)
