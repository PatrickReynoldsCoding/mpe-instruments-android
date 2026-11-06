package com.patrickreynolds.mpeinstruments.controllers

import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.EngineShape
import com.patrickreynolds.mpeinstruments.SCALES
import com.patrickreynolds.mpeinstruments.ui.ExternalMidiSurface
import com.patrickreynolds.mpeinstruments.ui.externalMidiOptions
import org.json.JSONObject

/**
 * The built-in external controller: an MPE keyboard (a Seaboard, a LinnStrument) or any MIDI
 * keyboard, over USB or Bluetooth. The device itself is connected in Settings, as the app's:
 * here are what its notes need, the bend range it sends, a transpose and pitch assist.
 */
object ExternalMidi : Controller {
    override val id = "midi"
    override val name = "External MIDI"
    override val description = "An MPE or MIDI keyboard, over USB or Bluetooth"
    override val landscape = false

    data class Settings(
        val bendRange: Float = 48f,  // the Seaboard sends ±48
        val transpose: Int = 0,
        val scale: Int = 0,          // pitch assist: index into SCALES; 0 = off
        val scaleRoot: Int = 0,
        val assist: Float = 0.6f,
    )

    var settings by mutableStateOf(Settings())
        private set

    override val shape get() = settings.let {
        EngineShape(scale = it.scale, scaleRoot = it.scaleRoot, assist = it.assist, transpose = it.transpose, bendRange = it.bendRange)
    }

    fun update(change: (Settings) -> Settings) {
        val now = change(settings)
        if (now == settings) return
        settings = now.copy(transpose = now.transpose.coerceIn(-60, 60))
        AppState.controllerChanged(this)
    }

    override fun save(): JSONObject = settings.let {
        JSONObject().put("bendRange", it.bendRange.toDouble()).put("transpose", it.transpose).put("scale", it.scale)
            .put("scaleRoot", it.scaleRoot).put("assist", it.assist.toDouble())
    }

    override fun load(o: JSONObject?) {
        val d = Settings()
        settings = if (o == null) d else Settings(
            o.optDouble("bendRange", d.bendRange.toDouble()).toFloat(), o.optInt("transpose", d.transpose).coerceIn(-60, 60),
            o.optInt("scale", d.scale).coerceIn(SCALES.indices), o.optInt("scaleRoot", d.scaleRoot).coerceIn(0, 11),
            o.optDouble("assist", d.assist.toDouble()).toFloat().coerceIn(0f, 1f))
    }

    /** Before 0.16 these were the app's: the bend range and transpose, and pitch assist in the performance. */
    fun legacy(bendRange: Float, transpose: Int, perf: JSONObject?): JSONObject = JSONObject()
        .put("bendRange", bendRange.toDouble()).put("transpose", transpose)
        .put("scale", perf?.optInt("scale", 0) ?: 0).put("scaleRoot", perf?.optInt("scaleRoot", 0) ?: 0)
        .put("assist", perf?.optDouble("assist", 0.6) ?: 0.6)

    @Composable
    override fun options(part: Int) = externalMidiOptions(part)

    @Composable
    override fun Surface(part: Int) = ExternalMidiSurface(part)
}
