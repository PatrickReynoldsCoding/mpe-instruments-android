package com.patrickreynolds.mpeinstruments.packs

import android.content.Context
import com.patrickreynolds.mpeinstruments.Native
import com.patrickreynolds.mpeinstruments.Part
import org.json.JSONObject

/**
 * An instrument pack: instruments, the groups they are shown in, and the effects each is meant
 * to be heard with. Described by a manifest (assets/packs/<id>.json); the sounds themselves are
 * the engine's (built into it from faust-mpe-instruments for the built-in pack). A downloaded
 * pack will bring its manifest and its sounds the same way.
 */
data class InstrumentPack(
    val id: String,
    val name: String,
    val description: String,
    val source: String,
    val defaults: SoundDefaults,
    val groups: List<Group>,
    val overrides: Map<String, SoundDefaults> = emptyMap(),
) {
    data class Group(val id: String, val name: String, val instruments: List<Native.Instrument>)

    val instruments get() = groups.flatMap { it.instruments }

    /** The effects instrument [id] is meant to be heard with. */
    fun defaultsFor(id: String) = overrides[id] ?: defaults
}

/** A pack's effects for an instrument: its part's sends, tone and touch, and the room it plays in. */
data class SoundDefaults(
    val reverb: Float = 0.15f,
    val reverbSize: Float = 0.5f,
    val reverbDamp: Float = 0.4f,
    val delay: Float = 0f,
    val brightness: Float = 1f,
    val drive: Float = 0f,
    val pressureCurve: Float = 1f,
    val release: Float = 0f,
) {
    /** A part playing [instrument], with these effects. */
    fun applyTo(p: Part, instrument: String = p.instrument) = p.copy(instrument = instrument, reverb = reverb, delay = delay,
        brightness = brightness, drive = drive, pressureCurve = pressureCurve, release = release)

    companion object {
        fun from(o: JSONObject?, d: SoundDefaults = SoundDefaults()) = if (o == null) d else SoundDefaults(
            f(o, "reverb", d.reverb), f(o, "reverbSize", d.reverbSize), f(o, "reverbDamp", d.reverbDamp), f(o, "delay", d.delay),
            f(o, "brightness", d.brightness), f(o, "drive", d.drive), f(o, "pressureCurve", d.pressureCurve), f(o, "release", d.release))
        private fun f(o: JSONObject, k: String, d: Float) = o.optDouble(k, d.toDouble()).toFloat()
    }
}

/** The instrument packs there are: the built-in one for now. */
object Packs {
    const val BUILT_IN = "built-in-instruments"

    var all: List<InstrumentPack> = emptyList()
        private set

    /** Read the manifests and match them to the engine's instruments. */
    fun load(context: Context, instruments: List<Native.Instrument>) {
        val text = runCatching { context.assets.open("packs/$BUILT_IN.json").bufferedReader().use { it.readText() } }.getOrNull()
        all = listOf(parse(text?.let { runCatching { JSONObject(it) }.getOrNull() }, instruments))
    }

    fun packOf(id: String) = all.firstOrNull { p -> p.instruments.any { it.id == id } }
    fun groupOf(id: String) = all.firstNotNullOfOrNull { p -> p.groups.firstOrNull { g -> g.instruments.any { it.id == id } } }
    fun defaultsFor(id: String) = packOf(id)?.defaultsFor(id) ?: SoundDefaults()

    /**
     * The built-in pack: its groups as the manifest lists them; an instrument the engine has but
     * the manifest does not (a new one in faust-mpe-instruments) joins the group of its folder,
     * or a new group, so none is ever left out.
     */
    private fun parse(o: JSONObject?, instruments: List<Native.Instrument>): InstrumentPack {
        val byId = instruments.associateBy { it.id }
        val listed = mutableSetOf<String>()
        val groups = mutableListOf<InstrumentPack.Group>()
        val ga = o?.optJSONArray("groups")
        for (k in 0 until (ga?.length() ?: 0)) {
            val g = ga!!.getJSONObject(k)
            val ids = g.optJSONArray("instruments")
            val list = (0 until (ids?.length() ?: 0)).mapNotNull { byId[ids!!.getString(it)] }.filter { listed.add(it.id) }
            groups += InstrumentPack.Group(g.getString("id"), g.optString("name", g.getString("id")), list)
        }
        for (i in instruments) if (i.id !in listed) {
            val at = groups.indexOfFirst { it.id == i.group }
            if (at >= 0) groups[at] = groups[at].copy(instruments = groups[at].instruments + i)
            else groups += InstrumentPack.Group(i.group, i.group.replaceFirstChar { it.uppercase() }, listOf(i))
        }
        val defaults = SoundDefaults.from(o?.optJSONObject("defaults"))
        val ov = o?.optJSONObject("overrides")
        val overrides = ov?.keys()?.asSequence()?.associateWith { SoundDefaults.from(ov.getJSONObject(it), defaults) } ?: emptyMap()
        return InstrumentPack(o?.optString("id", BUILT_IN) ?: BUILT_IN, o?.optString("name", "Built-in instruments") ?: "Built-in instruments",
                              o?.optString("description", "") ?: "", o?.optString("source", "") ?: "", defaults,
                              groups.filter { it.instruments.isNotEmpty() }, overrides)
    }
}
