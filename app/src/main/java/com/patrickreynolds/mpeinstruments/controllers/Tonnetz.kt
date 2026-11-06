package com.patrickreynolds.mpeinstruments.controllers

import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.Arp
import com.patrickreynolds.mpeinstruments.EngineShape
import com.patrickreynolds.mpeinstruments.ui.TonnetzSurface
import com.patrickreynolds.mpeinstruments.ui.tonnetzOptions
import org.json.JSONObject

/**
 * The Tonnetz: Euler's net of notes. Across is a fifth, up and to the right a major third, up
 * and to the left a minor third, so every small triangle is a triad (pointing up major, down
 * minor) and neighbouring triangles share two notes. Touch a node for its note, the middle of a
 * triangle for its chord; slide to the next triangle and the notes the two share keep sounding.
 */
object Tonnetz : Controller {
    override val id = "tonnetz"
    override val name = "Tonnetz"
    override val description = "A net of notes: a node plays a note, a triangle a chord; slide for smooth chord changes"
    override val landscape = true

    data class Settings(
        val key: Int = 0,             // the note in the middle; the key's chords are lit
        val octave: Int = 0,          // from the middle of the instrument
        val size: Int = 7,            // nodes across the screen
        val chords: Int = 0,          // 0 triads, 1 sevenths (maj7, m7)
        val bass: Boolean = false,    // the root an octave below as well
        val strum: Int = 0,           // index into STRUMS: a chord's notes one after another
        val labels: Int = 0,          // 0 notes and chords, 1 notes, 2 none
        val litKey: Boolean = true,   // light the key's chords
        val trails: Boolean = true,   // the path a finger took across the net
    ) {
        fun json(): JSONObject = JSONObject().put("key", key).put("octave", octave).put("size", size).put("chords", chords)
            .put("bass", bass).put("strum", strum).put("labels", labels).put("litKey", litKey).put("trails", trails)
    }

    /** Strum delays between a chord's notes, ms. */
    val STRUMS = listOf("Off" to 0L, "Fast" to 25L, "Slow" to 70L, "Harp" to 140L)
    val SIZES = 4..12

    var settings by mutableStateOf(Settings())
        private set

    override val shape get() = EngineShape(transpose = 0, bendRange = 48f)

    fun update(change: (Settings) -> Settings) {
        val now = change(settings)
        if (now == settings) return
        settings = now
        AppState.controllerChanged(this, engine = false)
    }

    override fun save() = settings.json()

    override fun load(o: JSONObject?) {
        val d = Settings()
        settings = if (o == null) d else Settings(
            o.optInt("key", d.key).coerceIn(0, 11), o.optInt("octave", d.octave).coerceIn(-3, 3),
            o.optInt("size", d.size).coerceIn(SIZES.first, SIZES.last), o.optInt("chords", d.chords).coerceIn(0, 1),
            o.optBoolean("bass", d.bass), o.optInt("strum", d.strum).coerceIn(STRUMS.indices), o.optInt("labels", d.labels).coerceIn(0, 2),
            o.optBoolean("litKey", d.litKey), o.optBoolean("trails", d.trails))
    }

    /** Until 0.21 the Tonnetz had its own arp, for its one part. */
    override fun legacyArps(o: JSONObject?) = o?.optJSONObject("arp")?.let { listOf(Arp.from(it)) }

    @Composable
    override fun options(part: Int) = tonnetzOptions()

    @Composable
    override fun Surface(part: Int) = TonnetzSurface()
}
