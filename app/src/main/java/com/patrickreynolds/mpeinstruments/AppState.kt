package com.patrickreynolds.mpeinstruments

import android.content.Context
import android.content.SharedPreferences
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.patrickreynolds.mpeinstruments.ui.DarkPalette
import com.patrickreynolds.mpeinstruments.ui.EinkPalette
import com.patrickreynolds.mpeinstruments.ui.palette
import org.json.JSONArray
import org.json.JSONObject

/**
 * A preset is a whole touch pad: its sounds and effects (the performance), and the pads' layout
 * (notes, scale, glide, up/down, vibrato, split). How the pad looks and the lock stay as they are.
 * Built-in presets have no layout of their own: loading one keeps the pads as they are.
 */
data class Preset(val name: String, val perf: Perf, val builtIn: Boolean = false, val pad: PadLayout? = null) {
    fun json(): JSONObject = JSONObject().put("name", name).put("perf", perf.json()).apply { pad?.let { put("pad", it.json()) } }
    companion object {
        fun from(o: JSONObject) = Preset(o.getString("name"), Perf.from(o.getJSONObject("perf")),
                                         pad = o.optJSONObject("pad")?.let { PadLayout.from(it) })
    }
}

/** Both pads' settings and where a split divides them. */
data class PadLayout(val pads: List<PadSettings>, val splitAt: Float) {
    fun json(): JSONObject = JSONObject().put("left", pads[0].json()).put("right", pads[1].json()).put("splitAt", splitAt.toDouble())
    companion object {
        fun from(o: JSONObject): PadLayout {
            val l = PadSettings.from(o.optJSONObject("left"))
            return PadLayout(listOf(l, PadSettings.from(o.optJSONObject("right"), l)),
                             o.optDouble("splitAt", 0.5).toFloat().coerceIn(PAD_SPLIT_MIN, 1 - PAD_SPLIT_MIN))
        }
    }
}

/**
 * What the screen shows and the engine plays. Every change goes to the engine at once and is
 * saved, so the app (and the background service) start where they left off.
 */
object AppState {
    private lateinit var prefs: SharedPreferences
    private lateinit var appContext: Context
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
    /** The on-screen pads (left and right when split); not part of a preset. */
    var pads by mutableStateOf(listOf(PadSettings(), PadSettings()))
        private set
    /** The left pad, which also holds the settings of the screen as a whole (theme, lines, lock). */
    val pad get() = pads[0]
    /** Where a split pad divides the screen, 0..1 across the playing area. */
    var padSplitAt by mutableStateOf(0.5f)
        private set
    /** Which pad of a split the rail is changing: 0 left, 1 right. */
    var padSide by mutableStateOf(0)

    /** The built-in preset on view was changed since it was loaded (a built-in is not changed itself). */
    var presetEdited by mutableStateOf(false)
        private set
    /** The loop buttons on the touch pad, and whether a loop's length snaps to beats. */
    var loopDock by mutableStateOf(true)
        private set
    var loopSnap by mutableStateOf(true)
        private set
    /** Each loop's layers' volume and mute (the loops themselves are not kept). */
    var loopLayers by mutableStateOf(List(Looper.LOOPS) { List(Looper.MAX_LAYERS) { 1f to false } })
        private set

    /** Tilt and breath: the phone as a controller; not part of a preset. */
    var mods by mutableStateOf(PhoneMods())
        private set

    /** E-ink look (black on white, no motion); not part of a preset. */
    var eink by mutableStateOf(false)
        private set

    fun init(context: Context) {
        if (started) return
        started = true
        appContext = context.applicationContext
        prefs = appContext.getSharedPreferences("mpe", Context.MODE_PRIVATE)
        instruments = Native.instruments()
        perf = runCatching { Perf.from(prefs.getString("perf", null)?.let { JSONObject(it) }) }.getOrDefault(Perf())
        // Before presets existed the instrument was saved as an index.
        if (!prefs.contains("perf") && prefs.contains("instrument"))
            instruments.getOrNull(prefs.getInt("instrument", -1))?.let { inst -> perf = perf.withPart(0) { it.copy(instrument = inst.id) } }
        volume = prefs.getFloat("volume", 0.8f)
        transpose = prefs.getInt("transpose", 0)
        bendRange = prefs.getFloat("bend", 48f)
        presetName = prefs.getString("preset", "") ?: ""
        eink = prefs.getBoolean("eink", false)
        palette = if (eink) EinkPalette else DarkPalette
        val p0 = runCatching { PadSettings.from(prefs.getString("pad", null)?.let { JSONObject(it) }) }.getOrDefault(PadSettings())
        val p1 = runCatching { PadSettings.from(prefs.getString("pad2", null)?.let { JSONObject(it) }, p0.copy(shift = 0)) }.getOrDefault(p0)
        pads = listOf(p0, p1)
        padSplitAt = prefs.getFloat("padSplitAt", 0.5f).coerceIn(PAD_SPLIT_MIN, 1 - PAD_SPLIT_MIN)
        mods = runCatching { PhoneMods.from(prefs.getString("mods", null)?.let { JSONObject(it) }) }.getOrDefault(PhoneMods())
        loopDock = prefs.getBoolean("loopDock", true)
        loopSnap = prefs.getBoolean("loopSnap", true)
        userPresets = runCatching {
            val a = JSONArray(prefs.getString("presets", "[]"))
            (0 until a.length()).map { Preset.from(a.getJSONObject(it)) }
        }.getOrDefault(emptyList())
        if (!prefs.getBoolean("presetsStarted", false)) startPresets()
        applyAll()
    }

    /** Send everything to the engine (at start, and after the audio engine was rebuilt). */
    fun applyAll() {
        Native.nativeSetVolume(volume)
        Native.nativeSetTranspose(transpose)
        Native.nativeSetBendRange(bendRange)
        applyInstruments(null)
        Native.nativeSetPerformance(perf.toArray())
        PhoneControl.apply(appContext, mods)
        Looper.snap(loopSnap)
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
        touched()
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

    fun changeEink(on: Boolean) {
        eink = on
        palette = if (on) EinkPalette else DarkPalette
        prefs.edit().putBoolean("eink", on).apply()
    }

    fun updateMods(change: (PhoneMods) -> PhoneMods) {
        val old = mods
        mods = change(mods)
        if (mods == old) return
        PhoneControl.apply(appContext, mods)
        // Breath with the screen off: the service has to say it uses the microphone.
        if ((mods.breath != 0) != (old.breath != 0)) SynthService.refresh(appContext)
        saver.removeCallbacks(saveMods)
        saver.postDelayed(saveMods, 400)
    }
    private val saveMods = Runnable { prefs.edit().putString("mods", mods.json().toString()).apply() }

    /** Change one pad: 0 left (or the only one), 1 right. */
    fun updatePad(side: Int = 0, change: (PadSettings) -> PadSettings) {
        val now = change(pads[side])
        if (now == pads[side]) return
        pads = pads.mapIndexed { k, p -> if (k == side) now else p }
        prefs.edit().putString(if (side == 0) "pad" else "pad2", now.json().toString()).apply()
        touched()
    }

    fun changePadSplit(at: Float) {
        padSplitAt = at.coerceIn(PAD_SPLIT_MIN, 1 - PAD_SPLIT_MIN)
        touched()
        saver.removeCallbacks(savePadSplit)
        saver.postDelayed(savePadSplit, 400)
    }
    private val savePadSplit = Runnable { prefs.edit().putFloat("padSplitAt", padSplitAt).apply() }

    // ---- the loops ---------------------------------------------------------------------------

    fun changeLoopDock(on: Boolean) { loopDock = on; prefs.edit().putBoolean("loopDock", on).apply() }
    fun changeLoopSnap(on: Boolean) { loopSnap = on; Looper.snap(on); prefs.edit().putBoolean("loopSnap", on).apply() }
    fun changeLoopLayer(t: Int, i: Int, volume: Float, muted: Boolean) {
        loopLayers = loopLayers.mapIndexed { k, ls -> if (k == t) ls.mapIndexed { j, l -> if (j == i) volume to muted else l } else ls }
        Looper.layer(t, i, volume, muted)
    }
    /** After a clear or an undo a loop's layers start again at full volume, unmuted. */
    fun resetLoopLayers(t: Int, from: Int = 0) {
        loopLayers = loopLayers.mapIndexed { k, ls -> if (k == t) ls.mapIndexed { j, l -> if (j >= from) 1f to false else l } else ls }
        for (j in from until Looper.MAX_LAYERS) Looper.layer(t, j, 1f, false)
    }

    // ---- presets ----------------------------------------------------------------------------

    val allPresets get() = BUILT_IN_PRESETS + userPresets
    /** The preset on view, if it is one of yours: what you change is kept in it. */
    val activePreset get() = userPresets.firstOrNull { it.name == presetName }
    private var loading = false

    private fun currentLayout() = PadLayout(pads, padSplitAt)

    /** Something changed: keep it in your preset on view, or note that a built-in one was changed. */
    private fun touched() {
        if (loading || presetName.isEmpty()) return
        val p = activePreset
        if (p == null) { presetEdited = true; return }
        val now = p.copy(perf = perf, pad = currentLayout())
        if (now == p) return
        userPresets = userPresets.map { if (it.name == p.name) now else it }
        saver.removeCallbacks(savePresetsNow)
        saver.postDelayed(savePresetsNow, 400)
    }

    fun load(p: Preset) {
        loading = true
        update { p.perf }
        p.pad?.let { l ->
            // The layout comes from the preset; how the pad looks and the lock stay as they are.
            val look = pads[0]
            pads = listOf(l.pads[0].copy(theme = look.theme, lines = look.lines, locked = look.locked, hideRail = look.hideRail), l.pads[1])
            prefs.edit().putString("pad", pads[0].json().toString()).putString("pad2", pads[1].json().toString()).apply()
            padSplitAt = l.splitAt
            prefs.edit().putFloat("padSplitAt", padSplitAt).apply()
        }
        loading = false
        presetName = p.name
        presetEdited = false
        prefs.edit().putString("preset", presetName).apply()
    }

    /** A name not yet taken: "Cello", then "Cello 2", "Cello 3"... */
    fun freeName(base: String, except: String? = null): String {
        val b = base.trim().ifEmpty { "Preset" }
        val taken = allPresets.map { it.name }.filter { it != except }.toSet()
        if (b !in taken) return b
        val stem = b.replace(Regex(" \\d+$"), "")
        var n = 2
        while ("$stem $n" in taken) n++
        return "$stem $n"
    }

    /** What is playing now, as a name: the instrument, or both of a split or layer. */
    fun soundName(p: Perf = perf): String {
        fun n(i: Int) = instruments.firstOrNull { it.id == p.part(i).instrument }?.name ?: "Preset"
        return if (p.layout == 0) n(0) else "${n(0)} + ${n(1)}"
    }

    /** Keep what is playing now as a new preset of yours (after the one on view), and switch to it. */
    fun newPreset(name: String = soundName()): String {
        val clean = freeName(name)
        val fresh = Preset(clean, perf, pad = currentLayout())
        val at = userPresets.indexOfFirst { it.name == presetName }
        userPresets = if (at < 0) userPresets + fresh else userPresets.take(at + 1) + fresh + userPresets.drop(at + 1)
        presetName = clean
        presetEdited = false
        savePresets()
        return clean
    }

    fun saveAs(name: String) {
        val clean = name.trim().ifEmpty { return }
        val fresh = Preset(clean, perf, pad = currentLayout())
        userPresets = if (userPresets.any { it.name == clean }) userPresets.map { if (it.name == clean) fresh else it } else userPresets + fresh
        presetName = clean
        presetEdited = false
        savePresets()
    }

    fun rename(p: Preset, name: String) {
        if (p.builtIn) return
        val clean = freeName(name, except = p.name)
        userPresets = userPresets.map { if (it.name == p.name) it.copy(name = clean) else it }
        if (presetName == p.name) presetName = clean
        savePresets()
    }

    fun duplicate(p: Preset) {
        val copy = p.copy(name = freeName(p.name), builtIn = false, pad = p.pad ?: currentLayout())
        val at = userPresets.indexOfFirst { it.name == p.name }
        userPresets = if (at < 0) userPresets + copy else userPresets.take(at + 1) + copy + userPresets.drop(at + 1)
        savePresets()
    }

    /** Move one of your presets one place earlier (-1) or later (+1). */
    fun move(p: Preset, by: Int) {
        val i = userPresets.indexOfFirst { it.name == p.name }
        val j = i + by
        if (i < 0 || j !in userPresets.indices) return
        userPresets = userPresets.toMutableList().apply { add(j, removeAt(i)) }
        savePresets()
    }

    fun delete(p: Preset) {
        if (p.builtIn) return
        userPresets = userPresets.filter { it.name != p.name }
        if (presetName == p.name) presetName = ""
        savePresets()
    }

    /**
     * Your first presets: a spread of sounds to loop together (and the touch pad's sound slots of
     * 0.13, which become presets of their own).
     */
    private fun startPresets() {
        val layout = currentLayout()
        val fromSlots = runCatching {
            val a = JSONArray(prefs.getString("slots", null) ?: return@runCatching null)
            (0 until a.length()).mapNotNull { k -> a.optJSONObject(k)?.let { Part.from(it) } }
        }.getOrNull()
        val parts = fromSlots ?: STARTING_SOUNDS
        val made = mutableListOf<Preset>()
        for (part in parts) {
            val p = Perf(parts = listOf(part, Part(instrument = "voices/soprano")))
            val base = soundName(p)
            val taken = (BUILT_IN_PRESETS + userPresets + made).map { it.name }.toSet()
            var name = base; var n = 2
            while (name in taken) name = "$base ${n++}"
            val drums = instruments.firstOrNull { it.id == part.instrument }?.group == "drums"
            made += Preset(name, p, pad = if (drums) PadLayout(layout.pads.map { it.copy(vertical = 2) }, layout.splitAt) else layout)
        }
        userPresets = userPresets + made
        prefs.edit().putBoolean("presetsStarted", true).remove("slots").apply()
        savePresets()
    }

    private val savePresetsNow = Runnable { savePresets() }
    private fun savePresets() {
        val a = JSONArray()
        for (p in userPresets) a.put(p.json())
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

/** The narrowest a pad of a split can be, as a fraction of the screen. */
const val PAD_SPLIT_MIN = 0.2f

/** Your first presets: a spread of sounds to loop together. */
val STARTING_SOUNDS = listOf(
    Part(instrument = "drums/kit", reverb = 0.08f),
    Part(instrument = "plucked/nylon-guitar", reverb = 0.15f),
    Part(instrument = "strings/cello", reverb = 0.2f),
    Part(instrument = "synths/pad", reverb = 0.3f),
    Part(instrument = "synths/lead", reverb = 0.12f, delay = 0.15f),
)
