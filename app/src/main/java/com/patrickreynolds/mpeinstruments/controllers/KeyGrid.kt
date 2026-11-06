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
 * from the bottom left (1.1, 1.2, 1.3, then 2.1...), so three fingers sit on three neighbouring
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
    ) {
        fun json(): JSONObject = JSONObject().put("rows", rows).put("cols", cols).put("key", key).put("scale", scale)
            .put("chromatic", chromatic).put("octave", octave).put("xTarget", xTarget).put("yTarget", yTarget)
            .put("vibrato", vibrato).put("labels", labels).put("arp", arp.json()).put("v", 2)
    }

    val ROWS = 1..10
    val COLS = 2..8

    var settings by mutableStateOf(Settings())
        private set

    override val shape get() = EngineShape(arps = listOf(settings.arp, Arp()), transpose = 0, bendRange = 48f)

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
        settings = if (o == null || o.optInt("v", 1) < 2) d.copy(arp = Arp.from(o?.optJSONObject("arp"))) else Settings(
            o.optInt("rows", d.rows).coerceIn(ROWS.first, ROWS.last), o.optInt("cols", d.cols).coerceIn(COLS.first, COLS.last),
            o.optInt("key", d.key).coerceIn(0, 11), o.optInt("scale", d.scale).coerceIn(SCALES.indices),
            o.optBoolean("chromatic", d.chromatic), o.optInt("octave", d.octave).coerceIn(-3, 3),
            o.optInt("xTarget", d.xTarget).coerceIn(TARGETS.indices), o.optInt("yTarget", d.yTarget).coerceIn(TARGETS.indices),
            o.optInt("vibrato", d.vibrato).coerceIn(0, 3), o.optBoolean("labels", d.labels), Arp.from(o.optJSONObject("arp")))
    }

    override fun clearForPreset() { settings = settings.copy(arp = Arp()) }

    @Composable
    override fun options(part: Int) = keyGridOptions()

    @Composable
    override fun Surface(part: Int) = KeyGridSurface()
}
