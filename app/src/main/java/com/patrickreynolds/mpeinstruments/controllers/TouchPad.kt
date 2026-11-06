package com.patrickreynolds.mpeinstruments.controllers

import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.Arp
import com.patrickreynolds.mpeinstruments.EngineShape
import com.patrickreynolds.mpeinstruments.PadSettings
import com.patrickreynolds.mpeinstruments.ui.TouchPadSurface
import com.patrickreynolds.mpeinstruments.ui.touchPadOptions
import org.json.JSONObject

/** The narrowest a pad of a split can be, as a fraction of the screen. */
const val PAD_SPLIT_MIN = 0.2f

/**
 * The built-in touch pad: the screen as an instrument, columns of notes, each finger its own
 * voice. Its settings: per pad the notes (size, octave, key, scale or chords), up/down, glide
 * and vibrato; split into two pads; the lock and the look. Split, each pad plays its own part
 * (and the app's arp of that part): left part 0, right part 1.
 */
object TouchPad : Controller {
    override val id = "touchpad"
    override val name = "Touch pad"
    override val description = "Play with your fingers on the screen"
    override val landscape = true

    /** Each pad's settings (left and right when split); the left one also holds the look and the lock. */
    var pads by mutableStateOf(listOf(PadSettings(), PadSettings()))
        private set
    /** The left pad, which also holds the settings of the screen as a whole (theme, lines, lock). */
    val pad get() = pads[0]
    var split by mutableStateOf(false)
        private set
    /** Where a split divides the screen, 0..1 across the playing area. */
    var splitAt by mutableStateOf(0.5f)
        private set
    override val shape get() = EngineShape(split = split, transpose = 0, bendRange = 48f)
    override val splitNames get() = if (split) listOf("Left", "Right") else null

    /** Change one pad: 0 left (or the only one), 1 right. */
    fun update(side: Int = 0, change: (PadSettings) -> PadSettings) {
        val now = change(pads[side])
        if (now == pads[side]) return
        pads = pads.mapIndexed { k, p -> if (k == side) now else p }
        AppState.controllerChanged(this, engine = false)
    }

    fun changeSplit(on: Boolean) {
        if (on == split) return
        split = on
        if (!on) AppState.side = 0
        AppState.controllerChanged(this)
    }

    fun changeSplitAt(at: Float) {
        splitAt = at.coerceIn(PAD_SPLIT_MIN, 1 - PAD_SPLIT_MIN)
        AppState.controllerChanged(this, engine = false)
    }

    override fun save(): JSONObject = JSONObject().put("left", pads[0].json()).put("right", pads[1].json())
        .put("splitAt", splitAt.toDouble()).put("split", split)

    override fun load(o: JSONObject?) {
        val l = PadSettings.from(o?.optJSONObject("left"))
        pads = listOf(l, PadSettings.from(o?.optJSONObject("right"), l.copy(shift = 0)))
        splitAt = (o?.optDouble("splitAt", 0.5) ?: 0.5).toFloat().coerceIn(PAD_SPLIT_MIN, 1 - PAD_SPLIT_MIN)
        split = o?.optBoolean("split", false) ?: false
    }

    /** Until 0.21 each pad had its arp: "arp0" and "arp1". */
    override fun legacyArps(o: JSONObject?) = o?.let { listOf(Arp.from(it.optJSONObject("arp0")), Arp.from(it.optJSONObject("arp1"))) }

    /**
     * A preset: the split always comes from it; its pads, if it has them (a built-in
     * preset may not: then the pads stay as they are). The look and the lock stay as they are.
     */
    override fun loadPreset(o: JSONObject) {
        val look = pads[0]
        val left = o.optJSONObject("left")
        if (left != null) {
            val l = PadSettings.from(left)
            pads = listOf(l.copy(theme = look.theme, lines = look.lines, locked = look.locked),
                          PadSettings.from(o.optJSONObject("right"), l.copy(shift = 0)))
            splitAt = o.optDouble("splitAt", 0.5).toFloat().coerceIn(PAD_SPLIT_MIN, 1 - PAD_SPLIT_MIN)
        }
        split = o.optBoolean("split", false)
    }

    override fun clearForPreset() { split = false }

    /** A preset's settings for the pad: the split given (off unless said), pads only if given. */
    fun preset(split: Boolean = false, pads: List<PadSettings>? = null, splitAt: Float = 0.5f): JSONObject = JSONObject().apply {
        put("split", split)
        if (pads != null) { put("left", pads[0].json()); put("right", pads[1].json()); put("splitAt", splitAt.toDouble()) }
    }

    /**
     * Before 0.16 the pad's settings were the app's: the pads (their own, or a preset's), and in
     * the performance the split (layout 1) and each part's arp.
     */
    fun legacy(left: JSONObject?, right: JSONObject?, splitAt: Float?, perf: JSONObject?): JSONObject = JSONObject().apply {
        if (left != null) {
            put("left", left); right?.let { put("right", it) }
            put("splitAt", (splitAt ?: 0.5f).toDouble())
        }
        put("split", perf?.optInt("layout", 0) == 1)
        put("arp0", Arp.from(perf?.optJSONObject("part0")).json())
        put("arp1", Arp.from(perf?.optJSONObject("part1")).json())
    }

    @Composable
    override fun options(part: Int) = touchPadOptions(part)

    @Composable
    override fun Surface(part: Int) = TouchPadSurface(part)
}
