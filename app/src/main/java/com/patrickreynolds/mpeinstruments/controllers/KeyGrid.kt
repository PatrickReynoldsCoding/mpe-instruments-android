package com.patrickreynolds.mpeinstruments.controllers

import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.Arp
import com.patrickreynolds.mpeinstruments.EngineShape
import com.patrickreynolds.mpeinstruments.PARTS
import com.patrickreynolds.mpeinstruments.SCALES
import com.patrickreynolds.mpeinstruments.ui.KeyGridSurface
import com.patrickreynolds.mpeinstruments.ui.gridPartColors
import com.patrickreynolds.mpeinstruments.ui.keyGridOptions
import org.json.JSONObject

/**
 * A grid of square keys, held upright: 3 across and 7 up to start, the notes climbing row by row
 * from the bottom right (1.1, 1.2, 1.3, then 2.1...; from the left, left-handed), so three fingers sit on three neighbouring
 * notes. Each square a note of a scale, or every semitone with the notes outside the scale
 * darkened. Hold a square and move it: across bends toward the note you drag to, up and down
 * changes the tone (each can be set to loudness, tone, bend or volume); a quick wiggle is vibrato.
 * A square glows while its note sounds, and fades as the sound does.
 *
 * Split, lines across between rows divide it into up to four sections from the bottom up, each
 * its own sound (part), colour and notes. A section is a window on a whole grid of its own: moving
 * a line shows more or less of it, the squares never change size.
 */
object KeyGrid : Controller {
    override val id = "grid"
    override val name = "Grid"
    override val description = "Square keys, upright: the notes climb row by row; drag to bend toward the next, wiggle for vibrato"
    override val landscape = false
    override val upright = true

    /** What moving a held square along an axis changes. */
    val TARGETS = listOf("Nothing", "Loudness", "Tone", "Bend", "Volume")
    const val NONE = 0; const val LOUDNESS = 1; const val TONE = 2; const val BEND = 3; const val VOLUME = 4

    /** One section's notes and arp: the whole grid's when it is not split. */
    data class Section(
        val key: Int = 0,
        val scale: Int = 1,             // index into SCALES (the key's notes); Off is the same as chromatic
        val chromatic: Boolean = false, // every semitone, the notes outside the scale darkened
        val octave: Int = 0,            // on top of the grid's octave
        val arp: Arp = Arp(),
    ) {
        fun json(): JSONObject = JSONObject().put("key", key).put("scale", scale).put("chromatic", chromatic)
            .put("octave", octave).put("arp", arp.json())
        companion object {
            fun from(o: JSONObject?, d: Section = Section()) = if (o == null) d else Section(
                o.optInt("key", d.key).coerceIn(0, 11), o.optInt("scale", d.scale).coerceIn(SCALES.indices),
                o.optBoolean("chromatic", d.chromatic), o.optInt("octave", d.octave).coerceIn(-3, 3),
                Arp.from(o.optJSONObject("arp"), d.arp).copy(latch = false))
        }
    }

    data class Settings(
        val rows: Int = 7,
        val cols: Int = 3,
        val sections: List<Section> = List(PARTS) { Section() },
        val splits: List<Int> = emptyList(), // the rows (from the bottom, 1..) where the next section starts
        val octave: Int = 0,            // the whole grid's, every section's
        val scroll: Int = 0,            // rows the notes are slid by: up shows higher notes (SCROLL)
        val xTarget: Int = BEND,        // Bend: toward the neighbouring note, reached a whole square away
        val yTarget: Int = TONE,
        val vibrato: Int = 2,           // index into PadSettings.VIBRATO
        val labels: Boolean = false,    // the root is marked either way
        val leftHanded: Boolean = false,// the notes climb left to right along each row; right-handed, right to left
        val theme: Int = 0,             // index into GRID_THEMES (ui/GridUi.kt)
        val squareSize: Float = 1f,     // the grid's size, as a share of the largest that fits (SIZES)
        val spacing: Float = 0.09f,     // the gap between squares, as a share of each square's space (SPACINGS)
        val fillRows: Boolean = true,   // as many rows as fit up to the top of the screen, instead of [rows]
        val latch: Boolean = false,     // a lifted finger leaves its note sounding (with or without the arp)
        val latchMode: Int = LATCH_CHORD,
        val sides: List<Int> = DEFAULT_SIDES, // where each of SIDE_CONTROLS sits: OFF, LEFT or RIGHT
    ) {
        fun json(): JSONObject = JSONObject().put("rows", rows).put("cols", cols)
            .put("sections", org.json.JSONArray(sections.map { it.json() })).put("splits", org.json.JSONArray(splits))
            .put("octave", octave).put("xTarget", xTarget).put("yTarget", yTarget)
            .put("vibrato", vibrato).put("labels", labels).put("v", 5).put("scroll", scroll)
            .put("leftHanded", leftHanded).put("theme", theme)
            .put("squareSize", squareSize.toDouble()).put("spacing", spacing.toDouble())
            .put("fillRows", fillRows).put("latch", latch).put("latchMode", latchMode)
            .put("sides", org.json.JSONArray(sides))
        fun side(control: Int) = sides.getOrElse(control) { OFF }
        /** How many sections: one, or one more than the lines. */
        val count get() = splits.size + 1
        fun section(k: Int) = sections.getOrElse(k) { Section() }
        fun withSection(k: Int, f: (Section) -> Section) = copy(sections = List(PARTS) { if (it == k) f(section(it)) else section(it) })
        /** The section of row [r] (from the bottom). */
        fun sectionOf(r: Int) = splits.count { it <= r }
    }

    /** The sections' names, bottom up, for the rail. */
    fun sectionNames(n: Int) = when (n) {
        2 -> listOf("Bottom", "Top"); 3 -> listOf("Bottom", "Middle", "Top")
        else -> listOf("Bottom", "Low mid", "High mid", "Top").take(n)
    }

    /** The rows the grid shows now (with Fill it depends on the screen): set by the surface, for placing new lines. */
    var shownRows by mutableStateOf(7)

    /** Latch: a new hand replaces what is held, or each tap holds a square and the next lets it go. */
    const val LATCH_CHORD = 0; const val LATCH_TAP = 1
    val LATCH_MODES = listOf("New hand replaces", "Tap on, tap off")

    /** The controls that can sit beside the grid, in the room it leaves on either side. */
    val SIDE_CONTROLS = listOf("Latch", "Hold", "Arp", "Scroll", "Bend", "Tone")
    const val S_LATCH = 0; const val S_HOLD = 1; const val S_ARP = 2; const val S_SCROLL = 3; const val S_BEND = 4; const val S_TONE = 5
    /** How far the rows can be slid either way: enough to reach every note an instrument has. */
    val SCROLL = -30..30
    const val OFF = 0; const val LEFT = 1; const val RIGHT = 2
    val DEFAULT_SIDES = listOf(LEFT, OFF, LEFT, RIGHT, OFF, OFF)
    /** The most rows Fill makes, however tall the screen. */
    const val FILL_MAX = 24

    /**
     * The limits keep the grid whole: never bigger than fits the screen (so no square leaves it),
     * and a gap that never reaches a square's whole space (so squares never overlap or vanish).
     */
    val SIZES = 0.5f..1f
    val SPACINGS = 0.02f..0.3f
    val ROWS = 1..10
    val COLS = 2..8

    var settings by mutableStateOf(Settings())
        private set

    // The grid latches its own notes (it simply does not let them go), so the engine's arp latch stays off.
    override val shape get() = settings.let { s ->
        EngineShape(split = s.count > 1, parts = s.count, arps = List(PARTS) { s.section(it).arp.copy(latch = false) },
                    transpose = 0, bendRange = 48f)
    }
    override val splitNames get() = if (settings.count > 1) sectionNames(settings.count) else null
    override val partColors get() = gridPartColors(settings)

    /**
     * Split into [n] sections (1 = not split), the lines spread evenly over the rows on screen.
     * Sections already there keep their notes; the rail goes back to the bottom one if its section went.
     */
    fun splitInto(n: Int) {
        val rows = shownRows.coerceAtLeast(1)
        val c = n.coerceIn(1, minOf(PARTS, rows))
        update { it.copy(splits = (1 until c).map { k -> (rows * k + c / 2) / c }.distinct().filter { r -> r in 1 until rows }) }
        if (AppState.side >= settings.count) AppState.side = 0
    }

    /** Move line [i] to row [r], keeping it between its neighbours. */
    fun moveSplit(i: Int, r: Int) = update { s ->
        val lo = (s.splits.getOrNull(i - 1) ?: 0) + 1
        val hi = (s.splits.getOrNull(i + 1) ?: shownRows) - 1
        if (lo > hi) s else s.copy(splits = s.splits.mapIndexed { k, v -> if (k == i) r.coerceIn(lo, hi) else v })
    }

    fun update(change: (Settings) -> Settings) {
        val now = change(settings)
        if (now == settings) return
        val engine = now.sections.map { it.arp } != settings.sections.map { it.arp } || now.splits.size != settings.splits.size
        settings = now
        AppState.controllerChanged(this, engine)
    }

    override fun save() = settings.json()

    override fun load(o: JSONObject?) {
        val d = Settings()
        // 0.18.0 laid the grid out sideways (3 rows of 7, labelled): from 2 on it stands upright.
        val arp = Arp.from(o?.optJSONObject("arp"))
        // Before 4 there was one section, its notes and arp in the settings themselves.
        val first = Section(o?.optInt("key", 0)?.coerceIn(0, 11) ?: 0, o?.optInt("scale", 1)?.coerceIn(SCALES.indices) ?: 1,
                            o?.optBoolean("chromatic", false) ?: false, 0, arp.copy(latch = false))
        val sections = o?.optJSONArray("sections")?.let { a -> List(PARTS) { Section.from(a.optJSONObject(it)) } }
            ?: listOf(first) + List(PARTS - 1) { Section() }
        settings = if (o == null || o.optInt("v", 1) < 2) d.copy(sections = listOf(Section(arp = arp.copy(latch = false))) +
                                                                     List(PARTS - 1) { Section() }, latch = arp.latch) else Settings(
            o.optInt("rows", d.rows).coerceIn(ROWS.first, ROWS.last), o.optInt("cols", d.cols).coerceIn(COLS.first, COLS.last),
            sections,
            o.optJSONArray("splits")?.let { a -> List(a.length()) { a.optInt(it) } }?.filter { it >= 1 }?.distinct()?.sorted()
                ?.take(PARTS - 1) ?: emptyList(),
            o.optInt("octave", d.octave).coerceIn(-3, 3), o.optInt("scroll", 0).coerceIn(SCROLL),
            o.optInt("xTarget", d.xTarget).coerceIn(TARGETS.indices), o.optInt("yTarget", d.yTarget).coerceIn(TARGETS.indices),
            o.optInt("vibrato", d.vibrato).coerceIn(0, 3), o.optBoolean("labels", d.labels),
            o.optBoolean("leftHanded", d.leftHanded), o.optInt("theme", d.theme).coerceAtLeast(0),
            o.optDouble("squareSize", d.squareSize.toDouble()).toFloat().coerceIn(SIZES),
            o.optDouble("spacing", d.spacing.toDouble()).toFloat().coerceIn(SPACINGS),
            // Fill is the way to play from 5 on: an older grid with set rows fills too.
            o.optBoolean("fillRows", d.fillRows) || o.optInt("v", 1) < 5,
            o.optBoolean("latch", d.latch) || arp.latch,   // the arp's latch (0.18) becomes the grid's
            o.optInt("latchMode", d.latchMode).coerceIn(LATCH_MODES.indices),
            o.optJSONArray("sides")?.let { a -> SIDE_CONTROLS.indices.map { a.optInt(it, OFF).coerceIn(OFF, RIGHT) } } ?: d.sides)
    }

    override fun clearForPreset() { settings = settings.copy(sections = settings.sections.map { it.copy(arp = Arp()) }, latch = false) }

    @Composable
    override fun options(part: Int) = keyGridOptions(part)

    @Composable
    override fun Surface(part: Int) = KeyGridSurface()
}
