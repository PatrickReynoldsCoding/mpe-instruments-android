package com.patrickreynolds.mpeinstruments.ui

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.border
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.size
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowLeft
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowRight
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.systemGestureExclusion
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Close
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.clipRect
import androidx.compose.ui.graphics.drawscope.rotate
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.controllers.Option
import com.patrickreynolds.mpeinstruments.controllers.TouchPad
import com.patrickreynolds.mpeinstruments.NOTE_NAMES
import com.patrickreynolds.mpeinstruments.Native
import com.patrickreynolds.mpeinstruments.ModReading
import com.patrickreynolds.mpeinstruments.PadSettings
import com.patrickreynolds.mpeinstruments.PhoneControl
import com.patrickreynolds.mpeinstruments.PhoneMods
import com.patrickreynolds.mpeinstruments.SCALES
import com.patrickreynolds.mpeinstruments.ARP_ORDERS
import com.patrickreynolds.mpeinstruments.Chords
import com.patrickreynolds.mpeinstruments.releaseName
import com.patrickreynolds.mpeinstruments.noteName
import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.floor
import kotlin.math.roundToInt
import kotlin.math.sign
import kotlin.math.tanh

/**
 * The touch pad's play surface: columns of notes left to right, each finger its own MPE voice.
 * Slide sideways to glide, up for louder and brighter (the bottom edge is silent), rock a finger
 * for vibrato. The notes go into the engine through the same parser as the Seaboard's
 * (Native.nativeTouch), so the arp, split and effects all apply. A Seaboard played at the same
 * time shows here too. The rail and the loop buttons around it are the app's (PlayHost).
 */
@Composable
fun TouchPadSurface(@Suppress("UNUSED_PARAMETER") part: Int) {
    val pad = remember { Pad() }
    var frame by remember { mutableIntStateOf(0) }
    val split = TouchPad.split
    val theme = padTheme(TouchPad.pad)
    pad.hudStack = AppState.loopDock
    DisposableEffect(Unit) { onDispose { pad.releaseAll() } }
    LaunchedEffect(Unit) {
        var last = 0L
        while (true) {
            val now = withFrameNanos { it }
            pad.tick(if (last == 0L) 0f else (now - last) / 1e9f)
            last = now
            // E-ink: redraw only when something changed, a still screen does not flash.
            if (!pad.theme.flat || pad.takeDirty()) frame++
        }
    }
    androidx.compose.foundation.layout.BoxWithConstraints(Modifier.fillMaxSize().background(theme.bottom)) {
        pad.widthDp = maxWidth.value
        pad.layout(TouchPad.pads, split, TouchPad.splitAt, FloatArray(2) { AppState.instrument(it)?.centerNote ?: 60f },
                   BooleanArray(2) { AppState.instrument(it)?.group == "drums" }, theme)
        Canvas(Modifier.fillMaxSize().systemGestureExclusion()
            .pointerInput(Unit) {
                awaitPointerEventScope {
                    while (true) {
                        val e = awaitPointerEvent()
                        val strip = STRIP.toPx()
                        val playH = (size.height - strip).coerceAtLeast(1f)
                        pad.widthDp = size.width / density
                        for (c in e.changes) {
                            val x = c.position.x / size.width; val y = 1f - c.position.y / playH
                            when {
                                c.pressed && !c.previousPressed ->
                                    if (c.position.y > size.height - strip) pad.stripDown(c.id.value, x, y)
                                    else pad.down(c.id.value, x, y)
                                c.pressed -> pad.move(c.id.value, x, y)
                                c.previousPressed -> pad.up(c.id.value)
                            }
                            c.consume()
                        }
                    }
                }
            }
        ) {
            frame.let { }                                     // redraw every frame
            pad.draw(this, STRIP.toPx())
        }
        // The chord pickers, over the left part of each pad in a chord mode.
        for (k in 0 until (if (split) 2 else 1)) {
            val ps = TouchPad.pads[k]
            val drums = AppState.instrument(k)?.group == "drums"
            val (a, b) = if (!split) 0f to 1f else if (k == 0) 0f to TouchPad.splitAt else TouchPad.splitAt to 1f
            // Its size as on a whole screen; a split only hides what does not fit.
            val f = minOf(pickerFrac(ps, drums, maxWidth.value), b - a)
            if (f > 0f) ChordPicker(k, ps, theme, Modifier.offset(x = maxWidth * a).width(maxWidth * f)
                                                        .height(maxHeight - STRIP))
        }
    }
}

/** The drum kit's notes (drums/kit.dsp, General MIDI order from C), as the pad lays them out: most used first. */
private val DRUM_ORDER = listOf(36, 38, 42, 46, 39, 37, 44, 40, 41, 43, 45, 47)
private val DRUM_NAMES = mapOf(0 to "Kick", 1 to "Rim", 2 to "Snare", 3 to "Clap", 4 to "Crash", 5 to "Tom 1", 6 to "Hat",
                               7 to "Tom 2", 8 to "Pedal", 9 to "Tom 3", 10 to "Open", 11 to "Bell")
private fun drumName(n: Int) = DRUM_NAMES[((n % 12) + 12) % 12] ?: ""

/**
 * How much of a pad (0..1 across it) a chord picker takes: the wheel about 230 dp, the grid about
 * 290 dp, but never more than 60% of the pad; nothing in scale mode or for a drum kit.
 */
fun pickerFrac(s: PadSettings, drums: Boolean, padWidthDp: Float): Float {
    if (drums || s.chordMode == 0 || padWidthDp <= 0f) return 0f
    val want = if (s.chordMode == 1) 230f else 290f
    return (want / padWidthDp).coerceAtMost(0.6f)
}

/** Height of the note strip along the bottom: drag it sideways to move the notes. */
private val STRIP = 64.dp
/** A touch on the note strip this close (dp) to a split's divide drags the divide. */
private const val DIVIDE_GRAB_DP = 28f

/** Vibrato: semitones per dp of wobble at the Normal depth and Medium sensitivity, before it rounds off. */
private const val VIB_SEMIS_PER_DP = 0.08f
private val VIBRATO_NORMAL = PadSettings.VIBRATO_DEPTH[2]
/** Wobble smaller than this (dp) is a still finger's jitter. */
private const val VIB_STILL_DP = 0.4f
/** A finger moving one way this fast (dp/s) is half sliding: its vibrato is let through 1/2. */
private const val VIB_SLIDE_DPS = 40f
/** Snap: a finger has to go this far (columns) past the edge of its column to change note, so a finger on the line does not flicker. */
private const val SNAP_HOLD = 0.08f

/**
 * The touch pad's options on the rail, for one pad (the left or right of a split): its notes,
 * up/down, glide, vibrato and arp; then, for the screen as a whole, the look, split and lock.
 */
@Composable
fun touchPadOptions(part: Int): List<Option> {
    // Each pad of a split plays its own part; one pad (layered or not) has only the left pad's settings.
    val split = TouchPad.split
    val side = if (split) part.coerceIn(0, 1) else 0
    val s = TouchPad.pads[side]
    val arp = TouchPad.arps[side]
    val accent = if (Eink) Color.Black else PartColors[side]
    fun set(change: (PadSettings) -> PadSettings) = TouchPad.update(side, change)
    val locked = TouchPad.pad.locked
    // "Size · left": only a split's pads are told apart (a layer's sound is not a pad of its own).
    return listOf<Option>(
        Option("Size", "${s.size}") {
            if (AppState.instrument(side)?.group == "drums")
                Stepper("${s.size.coerceIn(4, 12)} drums across") { d -> set { it.copy(size = (it.size.coerceIn(4, 12) + d).coerceIn(4, 12)) } }
            else Stepper("${s.size} notes across") { d -> set { it.copy(size = (it.size + d).coerceIn(3, 24)) } }
        },
        Option("Octave", if (s.octave == 0) "0" else "%+d".format(s.octave)) {
            Stepper(if (s.octave == 0) "Middle of the instrument" else "%+d octaves".format(s.octave)) { d ->
                set { it.copy(octave = (it.octave + d).coerceIn(-4, 4)) } }
            Text(if (locked) "The note strip is locked (Lock on the rail)."
                 else "Or drag the note strip along the bottom sideways.", color = TextDim, fontSize = 12.sp)
            if (s.shift != 0) TextButton(onClick = { set { it.copy(shift = 0) } }) {
                Text("Move the notes back (%+d)".format(s.shift)) }
        },
        Option("Key", NOTE_NAMES[s.key]) { Chips(NOTE_NAMES, s.key, accent) { k -> set { it.copy(key = k) } } },
        Option("Scale", when (s.chordMode) { 1 -> "Wheel"; 2 -> "Grid"; else -> if (s.scale == 0) "All notes" else SCALES[s.scale].name }) {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                Text("The pad's notes", color = TextDim, fontSize = 13.sp)
                Chips(listOf("A scale", "Chord wheel", "Chord grid"), s.chordMode, accent) { k -> set { it.copy(chordMode = k, shift = 0) } }
                if (s.chordMode == 0) {
                    Text("Scale", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
                    Chips(SCALES.map { if (it.mask == 0) "All notes" else it.name }, s.scale, accent) { k -> set { it.copy(scale = k) } }
                } else {
                    Text(if (s.chordMode == 1)
                        "The left of the pad is a circle of fifths: touch a chord and the pad holds only its notes; slide to " +
                        "the next without lifting. The outer ring is major chords, the middle their relative minors, the inner " +
                        "diminished: change them below. The buttons under the wheel add a 7th, 9th, 6th..."
                        else "The left of the pad is a grid of chords: the roots across, a chord type in each row. Touch one and " +
                        "the pad holds only its notes; slide to the next without lifting. Change the rows below.",
                        color = TextDim, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp))
                    ChordLayoutEditor(side, s, accent)
                }
            }
        },
        Option("Up/down", PadSettings.VERTICAL[s.vertical]) {
            Chips(PadSettings.VERTICAL, s.vertical, accent) { k -> set { it.copy(vertical = k) } }
            Text(when (s.vertical) {
                1 -> "Higher up the screen is brighter. Loudness stays the same."
                else -> "The height of a finger sets its loudness: the same height is always the same loudness."
            }, color = TextDim, fontSize = 12.sp)
            if (s.vertical != 1) {
                Text("Fade to silence", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
                Chips(PadSettings.FADE, s.fade, accent) { k -> set { it.copy(fade = k) } }
                Text(if (s.fade == 0) "The bottom edge is quiet but not silent."
                     else "The bottom edge is silent, so sliding down fades a note out. A longer fade spreads " +
                          "it over more of the pad.", color = TextDim, fontSize = 12.sp)
            }
        },
        Option("Glide", PadSettings.glideName(s.glide)) {
            LabeledSlider("Glide", s.glide, 0f..1f, PadSettings.glideName(s.glide), accent = accent, steps = 19) { v ->
                set { it.copy(glide = v) } }
            Text(when {
                s.glide <= 0.005f -> "Snap: each column plays its note, and a finger moving across goes straight " +
                                     "from one note to the next, with no bend in between."
                s.glide >= 0.995f -> "Free: pitch follows the finger exactly, like a fretless string."
                else -> "The middle of each column plays its note; nearer the edges the pitch glides to the next. " +
                        "Higher glides over more of the column."
            }, color = TextDim, fontSize = 12.sp)
        },
        Option("Vibrato", PadSettings.VIBRATO[s.vibrato]) {
            Chips(PadSettings.VIBRATO, s.vibrato, accent) { k -> set { it.copy(vibrato = k) } }
            if (s.vibrato != 0) {
                Text("Sensitivity", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
                Chips(PadSettings.VIB_SENSE, s.vibSense, accent) { k -> set { it.copy(vibSense = k) } }
            }
            Text(if (s.vibrato == 0) "The pitch moves only as far as the finger does."
                 else "Rock a held finger from side to side: the small wobble is made bigger, so a vibrato " +
                      "needs only a roll of the fingertip. Higher sensitivity needs a smaller roll. Slides are left alone.",
                 color = TextDim, fontSize = 12.sp)
            Text("A shake of the phone can add vibrato too, with any controller: Gyro.", color = TextDim, fontSize = 12.sp,
                 modifier = Modifier.padding(top = 6.dp))
        },
        Option("Arp", if (arp.on) ARP_ORDERS[arp.order.coerceIn(ARP_ORDERS.indices)] else "Off") {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                ArpControls(arp, accent) { c -> TouchPad.updateArp(side, c) }
                val bpm = AppState.perf.bpm
                LabeledSlider("Tempo", bpm, 40f..220f, "${bpm.roundToInt()} bpm", accent = accent) { v ->
                    AppState.update { it.copy(bpm = v.roundToInt().toFloat()) } }
            }
        },
        Option("Look", if (Eink) "E-ink" else PAD_THEMES[TouchPad.pad.theme].name, perPart = false) {
            val p0 = TouchPad.pad
            if (Eink) Text("E-ink is on (Settings), so the pad is black on white with no motion.", color = TextDim, fontSize = 12.sp)
            else Chips(PAD_THEMES.map { it.name }, p0.theme) { k -> TouchPad.update(0) { it.copy(theme = k) } }
            Text("Lines", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
            Chips(PadSettings.LINES, p0.lines) { k -> TouchPad.update(0) { it.copy(lines = k) } }
        },
        // Not panels: one tap each. Split makes two pads, each with its own sound and settings.
        Option("Split", if (split) "Two pads" else "One pad", perPart = false, on = split, tap = { TouchPad.changeSplit(!split) }),
        // Lock: the note strip plays instead of moving the notes, so a stray touch cannot move them.
        Option(if (locked) "Locked" else "Lock", if (locked) "Strip plays" else "Strip moves", perPart = false, on = locked,
               tap = { TouchPad.update(0) { it.copy(locked = !it.locked) } }),
    ).map { if (split) it else it.copy(perPart = false) }
}

@Composable
private fun Stepper(text: String, step: (Int) -> Unit) {
    Row(verticalAlignment = Alignment.CenterVertically) {
        TextButton(onClick = { step(-1) }) { Text("−", fontSize = 24.sp) }
        Text(text, fontSize = 15.sp, modifier = Modifier.padding(horizontal = 8.dp))
        TextButton(onClick = { step(1) }) { Text("+", fontSize = 24.sp) }
    }
}

/** How the pad looks. [flat] is the e-ink look: no gradients, glows, trails or ripples. */
class PadTheme(
    val name: String, val top: Color, val mid: Color, val bottom: Color, val line: Color, val label: Color,
    val strip: Color, val rail: Color, val railText: Color, val railDim: Color, val parts: List<Color>,
    val flat: Boolean = false,
)

private fun dark(name: String, top: Long, mid: Long, bottom: Long, a: Long, b: Long, strip: Long = 0xFF07080B) = PadTheme(
    name, Color(top), Color(mid), Color(bottom), Color.White, Color.White, Color(strip),
    Color.Black.copy(alpha = 0.35f), Color.White.copy(alpha = 0.85f), Color(0xFF9AA2B4), listOf(Color(a), Color(b)))

val PAD_THEMES = listOf(
    dark("Night", 0xFF1A1626, 0xFF0E1014, 0xFF0B1418, 0xFFFF8A65, 0xFF4DD0E1),
    dark("Sunset", 0xFF3A1430, 0xFF2A1022, 0xFF1A0C18, 0xFFFFB74D, 0xFFF06292, strip = 0xFF12070F),
    dark("Ocean", 0xFF0A2E3A, 0xFF07202B, 0xFF041318, 0xFF80DEEA, 0xFF69F0AE, strip = 0xFF020B0E),
    dark("Forest", 0xFF1B2A17, 0xFF131E11, 0xFF0C140B, 0xFFD4E157, 0xFFFFB74D, strip = 0xFF060B05),
    dark("Contrast", 0xFF000000, 0xFF000000, 0xFF000000, 0xFFFFEB3B, 0xFF00E5FF, strip = 0xFF1C1C1C),
    PadTheme("Daylight", Color(0xFFF7F3EA), Color(0xFFF1ECE1), Color(0xFFE9E3D6), Color(0xFF1B1B1B), Color(0xFF1B1B1B),
             Color(0xFFDCD4C3), Color(0xFFE2DBCB), Color(0xFF1B1B1B), Color(0xFF5A5A5A), listOf(Color(0xFFD84315), Color(0xFF00838F))),
)

private val EINK_PAD = PadTheme("E-ink", Color.White, Color.White, Color.White, Color.Black, Color.Black,
    Color.White, Color.White, Color.Black, Color(0xFF333333), listOf(Color.Black, Color(0xFF666666)), flat = true)

fun padTheme(s: PadSettings) = if (Eink) EINK_PAD else PAD_THEMES.getOrElse(s.theme) { PAD_THEMES[0] }

/**
 * Touches to MPE. Each finger gets a channel (16 down to 2, so a Seaboard on 2 up rarely
 * collides), a note from its column, and pitch bend for everything after: the glide and the
 * vibrato. A finger keeps the columns it went down on, so dragging the note strip while holding
 * a note does not bend it. With the performance split there are two pads side by side, each
 * playing its own part (the engine is told which with CC 9), whatever notes they cover.
 */
private class Pad {
    /** One pad: its settings, where it sits across the screen (0..1), and its columns. */
    class Zone(val part: Int) {
        var s = PadSettings()
        var drums = false                          // a drum kit: each column is a drum, struck, not pitched
        var start = 0f; var end = 1f               // the part of the screen the pad shows
        // Its notes, from x0 to x1, laid out as on a whole screen (after a chord picker). Split, a pad
        // keeps that size and shows only what fits on its side: the left one from its left end, the
        // right one up to its right end. Moving the divide shows more or fewer columns; none stretch.
        var x0 = 0f; var x1 = 1f
        val chords get() = s.chordMode != 0 && !drums
        var columns = IntArray(8) { 60 + it }      // sounding note of each column
        var shiftRange = 0..0
        val w get() = (x1 - x0).coerceAtLeast(1e-3f)
        fun local(x: Float) = (x - x0) / w
        fun global(lx: Float) = x0 + lx * w

        fun set(s: PadSettings, center: Float, drums: Boolean) {
            this.s = s
            this.drums = drums
            if (drums) {                           // the kit's drums, most used first; Size picks how many
                columns = DRUM_ORDER.take(s.size.coerceIn(4, DRUM_ORDER.size)).toIntArray()
                shiftRange = 0..0
                return
            }
            if (s.chordMode != 0) {                // a chord's notes only; the strip moves them by chord notes
                columns = Chords.columns(s.chordRoot, s.chord, center + 12 * s.octave, s.size, s.shift)
                shiftRange = -24..24
                return
            }
            val mask = SCALES[s.scale].mask.takeIf { it != 0 } ?: 0xFFF
            val inScale = (0..127).filter { (mask shr (((it - s.key) % 12 + 12) % 12)) and 1 == 1 }
            val want = center + 12 * s.octave
            val k = inScale.indices.minByOrNull { abs(inScale[it] - want) } ?: 0
            val base = k - s.size / 2
            val last = (inScale.size - s.size).coerceAtLeast(0)
            shiftRange = -base..(last - base)
            val start = (base + s.shift).coerceIn(0, last)
            columns = IntArray(s.size) { inScale.getOrElse(start + it) { 127 } }
        }
    }

    class Finger(val channel: Int, val note: Int, val zone: Int, var cols: IntArray, var x: Float, var y: Float) {
        // x is across the finger's own pad (0..1), and may go past its edges.
        var base = 0f              // pitch from where the finger is
        var col = -1               // Snap: the column it is playing
        // Vibrato: the finger's wobble around where it has been lately, made bigger.
        var centre = x             // x smoothed over ~0.12 s
        var drift = 0f             // x speed smoothed over ~0.15 s, dp/s: high while sliding
        var lastX = x
        var vib = 0f               // semitones
        var bend = -1; var pressure = -1; var slide = -1; var volume = -1
        val trail = ArrayDeque<Offset>()
        var age = 0f
        val pitch get() = base + vib
    }
    private class Ripple(val x: Float, val y: Float, val color: Color) { var age = 0f }
    /** A finger dragging a pad's note strip: where it started, and the shift then. */
    private class Grab(val zone: Int, val x: Float, val shift: Int)

    private val fingers = HashMap<Long, Finger>()
    private val grabs = HashMap<Long, Grab>()
    private var divider = -1L                          // the finger dragging a split's divide, or -1
    private val ripples = ArrayList<Ripple>()
    private var zones = listOf(Zone(0))
    private var split = false
    var theme = PAD_THEMES[0]; private set
    private val ext = FloatArray(16 * 5)
    private val lastExt = FloatArray(16 * 5)
    private var nExt = 0
    private var dirty = true
    /** Tilt and breath now, and which are on (read every frame while one is). */
    private var mod = ModReading()
    private var modsOn = PhoneMods()
    /** Width of the playing area in dp, so the vibrato is the same size of wiggle on every screen. */
    var widthDp = 400f
    /** The screen as a whole (theme, lines, lock): the left pad's settings. */
    private val main get() = zones[0].s

    fun takeDirty() = dirty.also { dirty = false }

    /** The gyro and breath display stacked at the top left (the loop buttons are at the top right). */
    var hudStack = false

    fun layout(pads: List<PadSettings>, split: Boolean, at: Float, centers: FloatArray, drums: BooleanArray, t: PadTheme) {
        val n = if (split) 2 else 1
        if (n != zones.size) { releaseAll(); zones = List(n) { Zone(it) } }
        val mid = if (split) at else 1f
        for ((k, z) in zones.withIndex()) {
            val (a, b) = if (k == 0) 0f to mid else mid to 1f
            val picker = pickerFrac(pads[k], drums[k], widthDp)
            val notesAt = a + minOf(picker, b - a)
            val notesW = 1f - picker                       // the notes' width on a whole screen
            val (x0, x1) = if (k == 0) notesAt to notesAt + notesW else (1f - notesW) to 1f
            if (z.s != pads[k] || z.x0 != x0 || z.x1 != x1 || z.start != a || z.end != b || z.drums != drums[k]) dirty = true
            z.start = a; z.end = b; z.x0 = x0; z.x1 = x1
            val before = z.columns
            z.set(pads[k], centers[k], drums[k])
            // A new chord: fingers held on this pad move to the same column of it, so the sound
            // changes with the chord. (In scale mode a finger keeps the notes it went down on.)
            if (z.chords && !before.contentEquals(z.columns)) for (f in fingers.values) if (f.zone == k) {
                f.cols = z.columns
                if (f.col >= f.cols.size) f.col = f.cols.lastIndex
                place(f)
                sendExpression(f)
            }
        }
        if (t !== theme || split != this.split) dirty = true
        this.split = split; theme = t
    }

    private fun zoneAt(x: Float) = if (split && x >= zones[1].start) 1 else 0

    /** x (0..1 across a pad) to sounding pitch, for a glide amount g (0 snap .. 1 free). */
    private fun pitchAt(cols: IntArray, x: Float, g: Float): Float {
        val u = x * cols.size - 0.5f
        val i = floor(u).toInt().coerceIn(0, cols.size - 2)
        val t = u - i
        // Free: straight lines between the column notes (and on past the end columns). Less glide:
        // the middle of each column holds its note and the glide is squeezed towards the edges.
        val r = if (g >= 0.995f) t else ((t - (1 - g) / 2) / g).coerceIn(0f, 1f)
        return cols[i] + r * (cols[i + 1] - cols[i])
    }
    private fun columnAt(cols: IntArray, x: Float) = (x * cols.size - 0.5f).roundToInt().coerceIn(0, cols.lastIndex)
    /** Sounding pitch back to x across a pad, for drawing (and for the Seaboard's fingers). */
    private fun xOf(z: Zone, pitch: Float): Float {
        val c = z.columns
        if (c.size < 2) return 0.5f
        var i = 0
        while (i < c.size - 2 && pitch > c[i + 1]) i++
        val span = (c[i + 1] - c[i]).coerceAtLeast(1)
        return (i + (pitch - c[i]) / span + 0.5f) / c.size
    }

    /** Where the finger is to its pitch, by its pad's glide. Snap holds a note until the finger is clearly in the next column. */
    private fun place(f: Finger) {
        val z = zones.getOrNull(f.zone) ?: return
        val g = z.s.glide
        val n = f.cols.size
        if (z.drums) { f.base = f.cols[f.col.coerceIn(0, n - 1)].toFloat(); return }   // a drum keeps its note
        if (g <= 0.005f || n < 2) {
            val u = f.x * n - 0.5f
            if (f.col < 0 || abs(u - f.col) > 0.5f + SNAP_HOLD) f.col = u.roundToInt().coerceIn(0, n - 1)
            f.base = f.cols[f.col].toFloat()
        } else {
            f.col = -1
            f.base = pitchAt(f.cols, f.x, g)
        }
    }

    private fun send(status: Int, d1: Int, d2: Int) = Native.nativeTouch(status, d1, d2)

    fun down(id: Long, x: Float, y: Float) {
        dirty = true
        val used = fingers.values.map { it.channel }.toSet()
        val ch = (15 downTo 1).firstOrNull { it !in used } ?: return
        val zi = zoneAt(x)
        val z = zones[zi]
        val cols = z.columns
        val lx = z.local(x)
        val col = columnAt(cols, lx)
        // The engine adds the app's transpose (set for the Seaboard): take it off so the pad plays what it shows.
        val note = (cols[col] - AppState.shape.transpose).coerceIn(0, 127)
        val y0 = y.coerceIn(0f, 1f)
        val f = Finger(ch, note, zi, cols, lx, y0)
        f.col = col
        place(f)
        fingers[id] = f
        // Order matters. The volume goes first, so a note low on the pad never starts loud, and in a
        // split the part this pad plays. The engine clears bend and pressure at the note on (as for a
        // Seaboard, whose pressure follows it), so they go after it.
        val (p, _, vol) = expression(f)
        f.volume = (vol * 127).roundToInt().coerceIn(0, 127)
        send(0xB0 or ch, 7, f.volume)
        if (split) send(0xB0 or ch, 9, z.part + 1)
        // The strike follows the height too. The engine multiplies velocity by 5 (the Block
        // strikes softly), so 127 / 5 is the hardest strike.
        send(0x90 or ch, note, (p * 127 / 5).roundToInt().coerceIn(1, 127))
        sendExpression(f)
        if (!theme.flat) ripples += Ripple(x, y0, colorOf(zi))
    }

    /**
     * A touch on the note strip. Near a split's divide it drags the divide; elsewhere it drags
     * that pad's notes sideways. Locked, the strip plays notes like the rest of the pad.
     */
    fun stripDown(id: Long, x: Float, y: Float) {
        if (main.locked) { down(id, x, y); return }
        if (split && abs(x - zones[1].start) * widthDp < DIVIDE_GRAB_DP) { divider = id; dirty = true; return }
        val zi = zoneAt(x)
        if (zones[zi].drums) { down(id, x, y); return }       // a kit has nothing to move: the strip plays
        grabs[id] = Grab(zi, x, zones[zi].s.shift); dirty = true
    }

    fun move(id: Long, x: Float, y: Float) {
        if (id == divider) { TouchPad.changeSplitAt(x); return }
        grabs[id]?.let { g ->
            val z = zones.getOrNull(g.zone) ?: return
            val shift = (g.shift - ((x - g.x) / z.w * z.columns.size).roundToInt())
                .coerceIn(z.shiftRange.first.coerceAtMost(g.shift), z.shiftRange.last.coerceAtLeast(g.shift))
            if (shift != z.s.shift) TouchPad.update(z.part) { it.copy(shift = shift) }
            return
        }
        val f = fingers[id] ?: return
        val z = zones.getOrNull(f.zone) ?: return
        f.x = z.local(x); f.y = y.coerceIn(0f, 1f)
        place(f)
        sendExpression(f)
        dirty = true
    }

    fun up(id: Long) {
        if (id == divider) { divider = -1; dirty = true; return }
        if (grabs.remove(id) != null) { dirty = true; return }
        val f = fingers.remove(id) ?: return
        send(0x80 or f.channel, f.note, 64)
        dirty = true
    }

    fun releaseAll() { grabs.clear(); divider = -1; for (id in fingers.keys.toList()) up(id) }

    fun tick(dt: Float) {
        modsOn = AppState.mods
        val r = if (modsOn.gyroOn || modsOn.breath != 0) ModReading.read() else ModReading()
        if (r.coarse() != mod.coarse()) dirty = true          // e-ink: redraw when it changed enough to see
        mod = r
        for (f in fingers.values) {
            f.age += dt
            if (dt > 0) vibrato(f, dt)
            place(f)
            sendExpression(f)
            if (!theme.flat) {
                val z = zones.getOrNull(f.zone) ?: continue
                val tx = fingerX(z, f)
                f.trail.addLast(Offset(z.global(tx), f.y))   // the trail shows the gyro's vibrato too
                if (f.trail.size > 28) f.trail.removeFirst()
            }
        }
        ripples.forEach { it.age += dt }
        ripples.removeAll { it.age > 0.7f }
        nExt = Native.nativeFingers(ext)
        // A controller's fingers: on e-ink only redraw when they moved enough to see.
        for (k in 0 until 5 * nExt) if (abs(ext[k] - lastExt[k]) > 0.02f) { dirty = true; ext.copyInto(lastExt); break }
        if (nExt == 0 && lastExt[0] != -1f) { dirty = true; lastExt[0] = -1f }
    }

    /**
     * A small side-to-side wobble of a resting finger, made bigger. The wobble is the finger's
     * distance from where it has been over the last ~0.12 s; while the finger slides (it keeps
     * moving one way) that distance is a lag, not a wobble, so it is let through less the faster
     * the slide. A tanh keeps a big wobble from going past the setting's depth.
     */
    private fun vibrato(f: Finger, dt: Float) {
        val z = zones.getOrNull(f.zone) ?: return
        val dpPerX = widthDp * z.w
        val depth = if (z.drums) 0f else PadSettings.VIBRATO_DEPTH.getOrElse(z.s.vibrato) { 0f }
        val v = (f.x - f.lastX) * dpPerX / dt
        f.lastX = f.x
        f.drift += (v - f.drift) * (1 - exp(-dt / 0.15f))
        val d = f.drift / VIB_SLIDE_DPS
        val notSliding = 1f / (1f + d * d * d * d)
        // While sliding the centre keeps up with the finger, so when it stops there is no lag
        // left over to be heard as a bend.
        f.centre += (f.x - f.centre) * maxOf(1 - exp(-dt / 0.12f), 1 - notSliding)
        if (depth == 0f) { f.vib = 0f; return }
        val dev = (f.x - f.centre) * dpPerX
        val wobble = sign(dev) * (abs(dev) - VIB_STILL_DP).coerceAtLeast(0f)   // a still finger's jitter is not a vibrato
        val gain = VIB_SEMIS_PER_DP * PadSettings.VIB_SENSE_GAIN.getOrElse(z.s.vibSense) { 1f }
        f.vib = depth * tanh(wobble * notSliding * gain / VIBRATO_NORMAL)
    }

    /** Pressure, slide (CC 74) and volume (CC 7) for a finger's height, by its pad's Up/down setting. */
    private fun expression(f: Finger): Triple<Float, Float, Float> {
        val s = zones.getOrNull(f.zone)?.s ?: PadSettings()
        val y = f.y
        // The bottom of the pad is silent: the volume fades the last stretch, so every instrument
        // goes all the way to nothing (most have a floor at zero pressure, as a Seaboard key does).
        val h = PadSettings.FADE_HEIGHT.getOrElse(s.fade) { 0.25f }
        val fade = if (h == 0f) 1f else (y / h).coerceIn(0f, 1f).let { it * it }
        return when (s.vertical) {
            1 -> Triple(0.7f, y, 1f)
            2 -> Triple(y, 0.5f, fade)
            else -> Triple(y, y, fade)
        }
    }

    private fun sendExpression(f: Finger) {
        val range = AppState.shape.bendRange.coerceAtLeast(1f)
        val semis = f.pitch - (f.note + AppState.shape.transpose)
        val bend = (8192 + semis / range * 8192).roundToInt().coerceIn(0, 16383)
        val (p, sl, vol) = expression(f)
        val pressure = (p * 127).roundToInt().coerceIn(0, 127)
        val slide = (sl * 127).roundToInt().coerceIn(0, 127)
        val volume = (vol * 127).roundToInt().coerceIn(0, 127)
        if (volume != f.volume) { f.volume = volume; send(0xB0 or f.channel, 7, volume) }
        if (bend != f.bend) { f.bend = bend; send(0xE0 or f.channel, bend and 0x7F, bend shr 7) }
        if (slide != f.slide) { f.slide = slide; send(0xB0 or f.channel, 74, slide) }
        if (pressure != f.pressure) { f.pressure = pressure; send(0xD0 or f.channel, pressure, 0) }
    }

    /** A pad's colour: its part's. */
    private fun colorOf(zone: Int) = theme.parts[zones.getOrNull(zone)?.part ?: 0]

    private fun isRoot(z: Zone, n: Int) = when {
        z.drums -> n % 12 == 0
        z.chords -> ((n - z.s.chordRoot) % 12 + 12) % 12 == 0
        else -> ((n - z.s.key) % 12 + 12) % 12 == 0
    }

    /**
     * Where to draw a finger across its pad (0..1): a drum on its column; in a chord mode where the
     * finger is (the notes change under it with the chord); otherwise where its pitch is, so the
     * glide and the gyro's bend show.
     */
    private fun fingerX(z: Zone, f: Finger) = when {
        z.drums -> (f.col + 0.5f) / z.columns.size
        z.chords -> f.x
        else -> xOf(z, f.pitch + mod.bend)
    }

    /** Where a pitch is across a pad (0..1): a kit's drum is wherever its column is. */
    private fun xAt(z: Zone, pitch: Float): Float {
        if (!z.drums) return xOf(z, pitch)
        val pc = ((pitch.roundToInt() % 12) + 12) % 12
        val i = z.columns.indexOfFirst { it % 12 == pc }
        return if (i < 0) -1f else (i + 0.5f) / z.columns.size
    }

    fun draw(d: androidx.compose.ui.graphics.drawscope.DrawScope, strip: Float) = with(d) {
        val t = theme
        val w = size.width; val h = size.height
        val playH = h - strip
        if (t.flat) drawRect(t.top) else drawRect(Brush.verticalGradient(listOf(t.top, t.mid, t.bottom)))
        // Soft, Clear, Bold: (line alpha, root line alpha, line width, root width, band alpha)
        val (la, ra, lw, rw, ba) = when (main.lines) {
            0 -> listOf(0.22f, 0.5f, 2f, 3f, 0f)
            2 -> listOf(0.75f, 1f, 4f, 7f, 0.10f)
            else -> listOf(0.45f, 0.85f, 3f, 5f, 0.05f)
        }
        val held = zones.map { z -> fingers.values.filter { zones.getOrNull(it.zone) === z }
                                         .map { if (z.drums) it.col else columnAt(z.columns, xOf(z, it.pitch)) }.toSet() }
        for ((zi, z) in zones.withIndex()) clipRect(left = z.start * w, right = z.end * w) {
            val left = z.x0 * w
            val cw = z.w * w / z.columns.size
            val c = colorOf(zi)
            for ((i, n) in z.columns.withIndex()) {
                val x = left + (i + 0.5f) * cw
                // Every other column shaded, so it is easy to see which one a finger is in.
                if (i % 2 == 1 && ba > 0f) drawRect(t.line.copy(alpha = ba), Offset(left + i * cw, 0f), Size(cw, playH))
                if (i in held[zi]) {
                    if (t.flat) drawRect(Color(0xFFAAAAAA), Offset(left + i * cw, 0f), Size(cw, playH))
                    else drawRect(Brush.horizontalGradient(listOf(c.copy(alpha = 0f), c.copy(alpha = 0.2f), c.copy(alpha = 0f)),
                                                           startX = x - cw / 2, endX = x + cw / 2),
                                  topLeft = Offset(x - cw / 2, 0f), size = Size(cw, playH))
                }
                val root = isRoot(z, n)
                val lineColor = if (t.flat) Color.Black else if (root) c else t.line
                drawLine(lineColor.copy(alpha = if (t.flat) 1f else if (root) ra else la), Offset(x, 0f), Offset(x, playH),
                         strokeWidth = if (t.flat) (if (root) rw + 2f else lw) else if (root) rw else lw)
            }
            drawStrip(this, z, zi, strip, cw, held[zi])
        }
        if (split) drawDivide(this, zones[1].start * w, strip)
        // Breath: a glow rising from the bottom as you blow (on e-ink a bar along the bottom).
        if (modsOn.breath != 0 && mod.breath > 0.01f) {
            val b = mod.breath
            if (t.flat) drawRect(Color.Black, Offset(0f, playH - 12f), Size(w * b, 12f))
            else {
                val top = playH * (1 - 0.7f * b)
                drawRect(Brush.verticalGradient(listOf(t.parts[1].copy(alpha = 0f), t.parts[1].copy(alpha = 0.32f * b)), startY = top, endY = playH),
                         topLeft = Offset(0f, top), size = Size(w, playH - top))
            }
        }
        for (r in ripples) {
            val k = r.age / 0.7f
            drawCircle(r.color.copy(alpha = 0.5f * (1 - k)), radius = 40f + 220f * k, center = Offset(r.x * w, (1 - r.y) * playH),
                       style = Stroke(width = 4f * (1 - k) + 1f))
        }
        val mine = fingers.values.map { it.channel }.toSet()
        for (i in 0 until nExt) {                           // a controller's fingers (ours are drawn below)
            val b = 5 * i
            if (ext[b].toInt() in mine) continue
            // In a split a controller's note goes by the split point: draw it on that part's pad.
            val zi = if (split && ext[b + 1].toInt() >= AppState.shape.splitNote) 1 else 0
            val z = zones.getOrNull(zi) ?: zones[0]
            val ex = xAt(z, ext[b + 2] + mod.bend)
            if (ex < 0) continue
            orb(Offset(z.global(ex) * w, (1 - ext[b + 4]) * playH), loudness(ext[b + 3]), colorOf(zi))
        }
        for (f in fingers.values) {
            val z = zones.getOrNull(f.zone) ?: continue
            val c = colorOf(f.zone)
            for ((k, p) in f.trail.withIndex()) {
                val a = (k + 1f) / f.trail.size
                drawCircle(c.copy(alpha = 0.22f * a), radius = 8f + 26f * a, center = Offset(p.x * w, (1 - p.y) * playH))
            }
            val fx = fingerX(z, f)
            orb(Offset(z.global(fx) * w, (1 - f.y) * playH), loudness(f.pressure / 127f), c)
        }
        drawHud(this, playH)
    }

    /** A split's divide: a line down the pad and, unlocked, a handle on the strip to drag it by. */
    private fun drawDivide(d: androidx.compose.ui.graphics.drawscope.DrawScope, x: Float, strip: Float) = with(d) {
        val t = theme
        val h = size.height; val top = h - strip
        val ink = if (t.flat) Color.Black else t.label
        drawLine(ink.copy(alpha = if (t.flat) 1f else 0.6f), Offset(x, 0f), Offset(x, h), strokeWidth = if (t.flat) 4f else 3f)
        if (main.locked) return@with
        val hw = 14.dp.toPx(); val hh = strip * 0.7f
        val y0 = top + (strip - hh) / 2
        val dragging = divider != -1L
        drawRoundRect(if (t.flat) Color.White else if (dragging) t.parts[0] else t.strip, Offset(x - hw, y0), Size(2 * hw, hh), CornerRadius(hw))
        drawRoundRect(ink.copy(alpha = if (t.flat) 1f else 0.8f), Offset(x - hw, y0), Size(2 * hw, hh), CornerRadius(hw),
                      style = Stroke(width = if (t.flat) 3f else 2f))
        for (k in -1..1) {                                   // grip: three short lines across
            val gx = x + k * 6.dp.toPx() / 1.5f
            drawLine(ink.copy(alpha = if (t.flat) 1f else 0.7f), Offset(gx, y0 + hh * 0.3f), Offset(gx, y0 + hh * 0.7f), strokeWidth = 2f)
        }
    }

    /** How big an orb is drawn: its pressure, or the breath when breath is the loudness (Swell). */
    private fun loudness(pressure: Float) = if (modsOn.breath == 1) mod.breath else pressure

    /** Tilt and breath along the top of the pad while one is on: what each is doing now. */
    private fun drawHud(d: androidx.compose.ui.graphics.drawscope.DrawScope, playH: Float) = with(d) {
        val items = listOf(modsOn.gyroOn, modsOn.breath != 0).count { it }
        if (items == 0) return@with
        val t = theme
        val iw = 176.dp.toPx(); val ih = 52.dp.toPx(); val gap = 10.dp.toPx(); var top = 8.dp.toPx(); val rr = 14.dp.toPx()
        // Centred along the top; with the loop buttons at the top right, at the bottom left instead,
        // just above the note strip (clear of a chord picker), stacked upwards.
        var x0 = if (hudStack) zones[0].x0 * size.width + 12.dp.toPx() else (size.width - items * iw - (items - 1) * gap) / 2
        if (hudStack) top = playH - ih - 10.dp.toPx()
        val canvas = drawContext.canvas.nativeCanvas
        val text = android.graphics.Paint().apply { isAntiAlias = true; color = t.label.toArgb() }
        val ink = if (t.flat) Color.Black else t.label
        fun box() {
            val bg = when { t.flat -> Color.White; t.name == "Daylight" -> Color.White.copy(alpha = 0.8f); else -> Color.Black.copy(alpha = 0.45f) }
            drawRoundRect(bg, Offset(x0, top), Size(iw, ih), CornerRadius(rr))
            if (t.flat) drawRoundRect(Color.Black, Offset(x0, top), Size(iw, ih), CornerRadius(rr), style = Stroke(width = 3f))
        }
        fun label(s: String, x: Float, y: Float, sizeDp: Float, bold: Boolean, alpha: Float = 1f) {
            text.textSize = sizeDp.dp.toPx(); text.isFakeBoldText = bold; text.alpha = (alpha * 255).toInt()
            canvas.drawText(s, x, y, text)
        }
        if (modsOn.gyroOn) {
            box()
            // A little phone turned as the phone is, by the first source in use (the full swing is the
            // full effect); a shake shakes it. Then the effect.
            val c = Offset(x0 + 32.dp.toPx(), top + ih / 2)
            val src = modsOn.gyro.indexOfFirst { it.target != 0 }.coerceAtLeast(0)
            val v = mod.axis.getOrElse(src) { 0f }
            val a = if (src == 3) v * 25f * kotlin.math.sin(System.nanoTime() / 2.5e7).toFloat()
                    else (v / gyroSpan(src, modsOn.source(src).target)).coerceIn(-1f, 1f) * 60f
            val col = if (t.flat) Color.Black else if (mod.held) t.parts[0] else ink.copy(alpha = 0.55f)
            val half = 20.dp.toPx()
            drawLine(ink.copy(alpha = if (t.flat) 0.4f else 0.25f), Offset(c.x - half - 4f, c.y), Offset(c.x + half + 4f, c.y), strokeWidth = 2f)
            rotate(a, pivot = c) {
                val pw = 2 * half; val ph = 18.dp.toPx()
                drawRoundRect(col, topLeft = Offset(c.x - pw / 2, c.y - ph / 2), size = Size(pw, ph), cornerRadius = CornerRadius(5.dp.toPx()),
                              style = Stroke(width = 3.dp.toPx()))
            }
            val tx = x0 + 64.dp.toPx()
            label("GYRO · " + gyroSummary(modsOn).uppercase(), tx, top + 19.dp.toPx(), 11f, true, 0.7f)
            val effect = gyroEffect(modsOn, mod)
            label(if (mod.held || t.flat) effect.ifEmpty { "move to play" } else "move to play", tx, top + 39.dp.toPx(), 13f, mod.held,
                  if (mod.held) 1f else 0.6f)
            if (hudStack) top -= ih + gap else x0 += iw + gap
        }
        if (modsOn.breath != 0) {
            box()
            val b = mod.breath
            val pad = 12.dp.toPx(); val by = top + ih - 16.dp.toPx(); val bh = 8.dp.toPx()
            val col = if (t.flat) Color.Black else t.parts[1]
            drawRoundRect(ink.copy(alpha = if (t.flat) 1f else 0.18f), Offset(x0 + pad, by), Size(iw - 2 * pad, bh), CornerRadius(bh / 2),
                          style = if (t.flat) Stroke(width = 2f) else androidx.compose.ui.graphics.drawscope.Fill)
            if (b > 0.005f) drawRoundRect(col, Offset(x0 + pad, by), Size((iw - 2 * pad) * b, bh), CornerRadius(bh / 2))
            val name = if (modsOn.breath == 1) "BREATH · SWELL" else "BREATH · TONE"
            label(name, x0 + pad, top + 19.dp.toPx(), 11f, true, 0.7f)
            val hint = when {
                PhoneControl.micState < 0 -> "no microphone"
                b > 0.02f -> "%d%%".format((b * 100).roundToInt())
                modsOn.breath == 1 && mod.held -> "blow to play"
                else -> "blow on the mic"
            }
            text.textAlign = android.graphics.Paint.Align.RIGHT
            label(hint, x0 + iw - pad, top + 19.dp.toPx(), 12f, b > 0.02f || mod.held, if (b > 0.02f) 1f else 0.7f)
            text.textAlign = android.graphics.Paint.Align.LEFT
        }
    }

    /** A pad's note names along the bottom. Unlocked it is a handle: drag sideways to move its notes. */
    private fun drawStrip(d: androidx.compose.ui.graphics.drawscope.DrawScope, z: Zone, zi: Int, strip: Float, cw: Float,
                          held: Set<Int>) = with(d) {
        val t = theme
        val w = size.width; val h = size.height; val top = h - strip
        val left = z.x0 * w; val right = z.x1 * w
        val dragging = grabs.values.any { it.zone == zi }
        drawRect(if (dragging && !t.flat) colorOf(zi).copy(alpha = 0.25f) else t.strip, Offset(left, top), Size(right - left, strip))
        drawLine(t.line.copy(alpha = if (t.flat) 1f else 0.5f), Offset(left, top), Offset(right, top), strokeWidth = if (t.flat) 3f else 2f)
        val paint = android.graphics.Paint().apply {
            isAntiAlias = true; textAlign = android.graphics.Paint.Align.CENTER
            textSize = (strip * 0.36f).coerceAtMost(cw * 0.42f)
        }
        val canvas = drawContext.canvas.nativeCanvas
        for ((i, n) in z.columns.withIndex()) {
            val x = left + (i + 0.5f) * cw
            val root = isRoot(z, n)
            if (i in held && t.flat) drawRect(Color.Black, Offset(left + i * cw, top), Size(cw, strip))
            if (i > 0) drawLine(t.label.copy(alpha = if (t.flat) 0.6f else 0.18f), Offset(left + i * cw, top + strip * 0.2f),
                                Offset(left + i * cw, h - strip * 0.2f), strokeWidth = 2f)
            paint.color = when {
                i in held && t.flat -> android.graphics.Color.WHITE
                t.flat -> android.graphics.Color.BLACK
                root -> colorOf(zi).toArgb()
                else -> t.label.copy(alpha = 0.85f).toArgb()
            }
            paint.isFakeBoldText = root
            canvas.drawText(if (z.drums) drumName(n) else noteName(n), x, top + strip * 0.62f, paint)
        }
        // Grip marks at the pad's outer ends while the strip can be dragged (a split's divide has its own handle;
        // a kit's strip does not move).
        if (!main.locked && !z.drums) {
            val g = t.label.copy(alpha = if (t.flat) 1f else 0.45f)
            for (k in 0..2) {
                val y = top + strip * (0.38f + 0.12f * k)
                if (zi == 0) drawLine(g, Offset(left + 8f, y), Offset(left + 26f, y), strokeWidth = 2f)
                if (zi == zones.lastIndex) drawLine(g, Offset(right - 26f, y), Offset(right - 8f, y), strokeWidth = 2f)
            }
        }
    }

    private fun androidx.compose.ui.graphics.drawscope.DrawScope.orb(at: Offset, pressure: Float, c: Color) {
        val r = 60f + 70f * pressure
        if (theme.flat) {                                    // e-ink: a solid ring and dot, no glow
            drawCircle(Color.White, radius = r, center = at)
            drawCircle(c, radius = r, center = at, style = Stroke(width = 6f))
            drawCircle(c, radius = r * 0.3f, center = at)
            return
        }
        drawCircle(Brush.radialGradient(listOf(c.copy(alpha = 0.55f), c.copy(alpha = 0f)), center = at, radius = r * 1.8f),
                   radius = r * 1.8f, center = at)
        drawCircle(c.copy(alpha = 0.8f), radius = r, center = at, style = Stroke(width = 3f))
        drawCircle(theme.bottom.copy(alpha = 0.55f), radius = r * 0.62f, center = at)
        drawCircle(Color.White.copy(alpha = 0.9f), radius = r * 0.24f, center = at)
        drawCircle(c, radius = r * 0.17f, center = at)
    }
}
