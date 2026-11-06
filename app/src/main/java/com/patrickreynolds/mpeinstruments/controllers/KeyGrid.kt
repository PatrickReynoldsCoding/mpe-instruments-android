package com.patrickreynolds.mpeinstruments.controllers

import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.Arp
import com.patrickreynolds.mpeinstruments.EngineShape
import com.patrickreynolds.mpeinstruments.SCALES
import com.patrickreynolds.mpeinstruments.ui.KeyGridSurface
import com.patrickreynolds.mpeinstruments.ui.keyGridOptions
import org.json.JSONObject

/**
 * A grid of square keys, held upright: 3 across and 7 up to start, the notes climbing row by row
 * from the bottom right (1.1, 1.2, 1.3, then 2.1...; from the left, left-handed), so three fingers sit on three neighbouring
 * notes. Each square a note of a scale, or every semitone with the notes outside the scale
 * darkened. Hold a square and move it: across bends toward the note you drag to, up and down
 * changes the tone (each can be set to loudness, tone, bend or volume); a quick wiggle is vibrato.
 * A square glows while its note sounds, and fades as the sound does.
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

    data class Settings(
        val rows: Int = 7,
        val cols: Int = 3,
        val key: Int = 0,
        val scale: Int = 1,             // index into SCALES (the key's notes); Off is the same as chromatic
        val chromatic: Boolean = false, // every semitone, the notes outside the scale darkened
        val octave: Int = 0,
        val xTarget: Int = BEND,        // Bend: toward the neighbouring note, reached a whole square away
        val yTarget: Int = TONE,
        val vibrato: Int = 2,           // index into PadSettings.VIBRATO
        val labels: Boolean = false,    // the root is marked either way
        val arp: Arp = Arp(),
        val leftHanded: Boolean = false,// the notes climb left to right along each row; right-handed, right to left
        val theme: Int = 0,             // index into GRID_THEMES (ui/GridUi.kt)
        val squareSize: Float = 1f,     // the grid's size, as a share of the largest that fits (SIZES)
        val spacing: Float = 0.09f,     // the gap between squares, as a share of each square's space (SPACINGS)
        val fillRows: Boolean = false,  // as many rows as fit up to the top of the screen, instead of [rows]
        val latch: Boolean = false,     // a lifted finger leaves its note sounding (with or without the arp)
        val latchMode: Int = LATCH_CHORD,
        val sides: List<Int> = DEFAULT_SIDES, // where each of SIDE_CONTROLS sits: OFF, LEFT or RIGHT
    ) {
        fun json(): JSONObject = JSONObject().put("rows", rows).put("cols", cols).put("key", key).put("scale", scale)
            .put("chromatic", chromatic).put("octave", octave).put("xTarget", xTarget).put("yTarget", yTarget)
            .put("vibrato", vibrato).put("labels", labels).put("arp", arp.json()).put("v", 3)
            .put("leftHanded", leftHanded).put("theme", theme)
            .put("squareSize", squareSize.toDouble()).put("spacing", spacing.toDouble())
            .put("fillRows", fillRows).put("latch", latch).put("latchMode", latchMode)
            .put("sides", org.json.JSONArray(sides))
        fun side(control: Int) = sides.getOrElse(control) { OFF }
    }

    /** Latch: a new hand replaces what is held, or each tap holds a square and the next lets it go. */
    const val LATCH_CHORD = 0; const val LATCH_TAP = 1
    val LATCH_MODES = listOf("New hand replaces", "Tap on, tap off")

    /** The controls that can sit beside the grid, in the room it leaves on either side. */
    val SIDE_CONTROLS = listOf("Latch", "Hold", "Arp", "Octave", "Bend", "Tone")
    const val S_LATCH = 0; const val S_HOLD = 1; const val S_ARP = 2; const val S_OCTAVE = 3; const val S_BEND = 4; const val S_TONE = 5
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
    override val shape get() = EngineShape(arps = listOf(settings.arp.copy(latch = false), Arp()), transpose = 0, bendRange = 48f)

    fun update(change: (Settings) -> Settings) {
        val now = change(settings)
        if (now == settings) return
        val engine = now.arp != settings.arp
        settings = now
        AppState.controllerChanged(this, engine)
    }

    override fun save() = settings.json()

    override fun load(o: JSONObject?) {
        val d = Settings()
        // 0.18.0 laid the grid out sideways (3 rows of 7, labelled): from 2 on it stands upright.
        val arp = Arp.from(o?.optJSONObject("arp"))
        settings = if (o == null || o.optInt("v", 1) < 2) d.copy(arp = arp.copy(latch = false), latch = arp.latch) else Settings(
            o.optInt("rows", d.rows).coerceIn(ROWS.first, ROWS.last), o.optInt("cols", d.cols).coerceIn(COLS.first, COLS.last),
            o.optInt("key", d.key).coerceIn(0, 11), o.optInt("scale", d.scale).coerceIn(SCALES.indices),
            o.optBoolean("chromatic", d.chromatic), o.optInt("octave", d.octave).coerceIn(-3, 3),
            o.optInt("xTarget", d.xTarget).coerceIn(TARGETS.indices), o.optInt("yTarget", d.yTarget).coerceIn(TARGETS.indices),
            o.optInt("vibrato", d.vibrato).coerceIn(0, 3), o.optBoolean("labels", d.labels), arp,
            o.optBoolean("leftHanded", d.leftHanded), o.optInt("theme", d.theme).coerceAtLeast(0),
            o.optDouble("squareSize", d.squareSize.toDouble()).toFloat().coerceIn(SIZES),
            o.optDouble("spacing", d.spacing.toDouble()).toFloat().coerceIn(SPACINGS),
            o.optBoolean("fillRows", d.fillRows),
            o.optBoolean("latch", d.latch) || arp.latch,   // the arp's latch (0.18) becomes the grid's
            o.optInt("latchMode", d.latchMode).coerceIn(LATCH_MODES.indices),
            o.optJSONArray("sides")?.let { a -> SIDE_CONTROLS.indices.map { a.optInt(it, OFF).coerceIn(OFF, RIGHT) } } ?: d.sides)
            .let { it.copy(arp = it.arp.copy(latch = false)) }
    }

    override fun clearForPreset() { settings = settings.copy(arp = Arp(), latch = false) }

    @Composable
    override fun options(part: Int) = keyGridOptions()

    @Composable
    override fun Surface(part: Int) = KeyGridSurface()
}
