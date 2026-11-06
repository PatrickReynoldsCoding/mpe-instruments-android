package com.patrickreynolds.mpeinstruments.controllers

import androidx.compose.runtime.Composable
import com.patrickreynolds.mpeinstruments.EngineShape
import org.json.JSONObject

/**
 * A controller: a way of playing the app's instruments. It sends MPE to the engine (through
 * Native.nativeTouch, the same parser as a Seaboard's), and brings its own settings, its own
 * options on the rail and its own play surface. What every controller shares (the sounds,
 * effects, release, loops, gyro and breath, presets) is the app's, around it.
 *
 * The touch pad and an external MIDI controller are built in. This is the seam a downloaded
 * controller, a plug-in, will implement.
 */
interface Controller {
    /** Stable, saved in presets: "touchpad", "midi". */
    val id: String
    val name: String
    val description: String

    /** The play surface wants the screen on its side (the touch pad), or either way up. */
    val landscape: Boolean

    /** What it sets in the engine beside the sound: a split, the arps, pitch assist, transpose. */
    val shape: EngineShape

    /** The parts (sounds) it plays, by name, when it splits the notes between two: "Left", "Right". */
    val splitNames: List<String>? get() = null

    /** Its settings, kept between sessions and in each preset. */
    fun save(): JSONObject

    /** Settings saved by [save], or null for its defaults. */
    fun load(o: JSONObject?)

    /** A preset's settings: like [load], but what belongs to the screen rather than the preset stays (the pad's look). */
    fun loadPreset(o: JSONObject) = load(o)

    /** A preset that says nothing of this controller: what is per preset goes back to off (the pad's split and arps). */
    fun clearForPreset() {}

    /** Its own options on the rail, in order, for the part the rail is changing. */
    @Composable
    fun options(part: Int): List<Option>

    /** What you play on; it fills the screen beside the rail. */
    @Composable
    fun Surface(part: Int)

    /** Let go of every note it holds (switching to another controller). */
    fun stop() {}
}

/**
 * One option on the rail: a label, a word or two for its value, and either a panel that opens
 * beside the rail or, for a switch, a tap.
 */
data class Option(
    val label: String,
    val value: String,
    /** Changes the part the rail is on (named in the panel's title when the notes are split). */
    val perPart: Boolean = true,
    /** A switch: on (highlighted) or off; [tap] flips it. */
    val on: Boolean = false,
    val tap: (() -> Unit)? = null,
    val panel: (@Composable () -> Unit)? = null,
)

/** The controllers there are: built in for now. */
object Controllers {
    val all: List<Controller> get() = listOf(TouchPad, ExternalMidi)
    fun byId(id: String?): Controller = all.firstOrNull { it.id == id } ?: TouchPad
}
