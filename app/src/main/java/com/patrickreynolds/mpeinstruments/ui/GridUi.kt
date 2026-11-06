package com.patrickreynolds.mpeinstruments.ui

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.systemGestureExclusion
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.rotate
import androidx.compose.ui.draw.clipToBounds
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.PathEffect
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.patrickreynolds.mpeinstruments.ARP_ORDERS
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.NOTE_NAMES
import com.patrickreynolds.mpeinstruments.Native
import com.patrickreynolds.mpeinstruments.PadSettings
import com.patrickreynolds.mpeinstruments.SCALES
import com.patrickreynolds.mpeinstruments.controllers.KeyGrid
import com.patrickreynolds.mpeinstruments.controllers.Option
import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.floor
import kotlin.math.log10
import kotlin.math.roundToInt

/** The grid's options on the rail. */
@Composable
fun keyGridOptions(): List<Option> {
    val s = KeyGrid.settings
    val accent = if (Eink) Color.Black else gridTheme(s).light
    fun set(change: (KeyGrid.Settings) -> KeyGrid.Settings) = KeyGrid.update(change)
    return listOf(
        Option("Grid", "${s.cols} × ${if (s.fillRows) "fill" else s.rows}", perPart = false) {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                Text("Squares across", color = TextDim, fontSize = 13.sp)
                Chips(KeyGrid.COLS.map { "$it" }, s.cols - KeyGrid.COLS.first, accent) { k -> set { it.copy(cols = KeyGrid.COLS.first + k) } }
                Text("Rows up", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
                Chips(listOf("Fill") + KeyGrid.ROWS.map { "$it" }, if (s.fillRows) 0 else s.rows - KeyGrid.ROWS.first + 1, accent) { k ->
                    set { if (k == 0) it.copy(fillRows = true) else it.copy(fillRows = false, rows = KeyGrid.ROWS.first + k - 1) } }
                if (s.fillRows) Text("Fill: as many rows as fit up to the top of the screen, at the square size below.",
                                     color = TextDim, fontSize = 12.sp)
                LabeledSlider("Square size", s.squareSize, KeyGrid.SIZES, "${(s.squareSize * 100).roundToInt()}%", accent = accent) { v ->
                    set { it.copy(squareSize = v.coerceIn(KeyGrid.SIZES)) } }
                LabeledSlider("Spacing", s.spacing, KeyGrid.SPACINGS, "${(s.spacing * 100).roundToInt()}%", accent = accent) { v ->
                    set { it.copy(spacing = v.coerceIn(KeyGrid.SPACINGS)) } }
                Text("Size is a share of the biggest grid that fits the screen; spacing a share of each square's room.",
                     color = TextDim, fontSize = 12.sp)
                Text("Hand", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
                Segmented(listOf("Right-handed", "Left-handed"), if (s.leftHanded) 1 else 0, accent) { k -> set { it.copy(leftHanded = k == 1) } }
                Text("The notes climb row by row from the bottom " + (if (s.leftHanded) "left, left to right" else "right, right to left") +
                     ": three fingers on a row of three play three neighbouring notes, and the next row carries on.",
                     color = TextDim, fontSize = 12.sp)
            }
        },
        Option("Notes", if (s.chromatic) "Chromatic" else SCALES[s.scale].name, perPart = false) {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                Segmented(listOf("The scale's notes", "Chromatic"), if (s.chromatic) 1 else 0, accent) { k -> set { it.copy(chromatic = k == 1) } }
                Text(if (s.chromatic) "Every semitone; the notes outside the scale are dimmed, their outlines broken."
                     else "Only the scale's notes, so every square fits.", color = TextDim, fontSize = 12.sp)
                Text("Scale", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
                Chips(SCALES.drop(1).map { it.name }, s.scale - 1, accent) { k -> set { it.copy(scale = k + 1) } }
                Text("Key", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
                Chips(NOTE_NAMES, s.key, accent) { k -> set { it.copy(key = k) } }
                SwitchRow("Note names", s.labels, "The root's square is lit either way", accent = accent) { v -> set { it.copy(labels = v) } }
            }
        },
        Option("Look", if (Eink) "E-ink" else gridTheme(s).name, perPart = false) {
            if (Eink) Text("E-ink is on (Settings), so the grid is black on white.", color = TextDim, fontSize = 12.sp)
            else Chips(GRID_THEMES.map { it.name }, s.theme.coerceIn(GRID_THEMES.indices), accent) { k -> set { it.copy(theme = k) } }
        },
        Option("Octave", if (s.octave == 0) "0" else "%+d".format(s.octave), perPart = false) {
            Chips(listOf("−3", "−2", "−1", "0", "+1", "+2", "+3"), s.octave + 3, accent) { k -> set { it.copy(octave = k - 3) } }
        },
        Option("Across", KeyGrid.TARGETS[s.xTarget], perPart = false) {
            Chips(KeyGrid.TARGETS, s.xTarget, accent) { k -> set { it.copy(xTarget = k) } }
            AxisHelp()
        },
        Option("Up/down", KeyGrid.TARGETS[s.yTarget], perPart = false) {
            Chips(KeyGrid.TARGETS, s.yTarget, accent) { k -> set { it.copy(yTarget = k) } }
            AxisHelp()
        },
        Option("Vibrato", PadSettings.VIBRATO[s.vibrato], perPart = false) {
            Chips(PadSettings.VIBRATO, s.vibrato, accent) { k -> set { it.copy(vibrato = k) } }
            Text("Wiggle a held square quickly from side to side. A slow move is left to Across and Up/down.",
                 color = TextDim, fontSize = 12.sp)
        },
        Option("Latch", if (s.latch) (if (s.latchMode == KeyGrid.LATCH_TAP) "Tap" else "On") else "Off", perPart = false) {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                SwitchRow("Latch", s.latch, "Lift your fingers and the notes keep sounding, arp or not. Turning it off lets them go.",
                          accent = accent) { v -> set { it.copy(latch = v) } }
                Text("What the next touch does", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
                Segmented(KeyGrid.LATCH_MODES, s.latchMode, accent) { k -> set { it.copy(latchMode = k) } }
                Text(if (s.latchMode == KeyGrid.LATCH_TAP)
                         "Tap a square to hold it; tap it again to let it go. Build a chord a note at a time."
                     else "Fingers that go down together make a chord; once they have all lifted, the next touch lets that chord go " +
                          "and starts a new one. Add fingers while some are still down to add notes.",
                     color = TextDim, fontSize = 12.sp)
                Text("A held square keeps a bright edge and a dot in its corner. Hold (beside the grid) is the same while you press it, like a sustain pedal.",
                     color = TextDim, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp))
            }
        },
        Option("Arp", if (s.arp.on) ARP_ORDERS[s.arp.order] else "Off", perPart = false) {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                ArpControls(s.arp, accent, latch = false) { c -> set { it.copy(arp = c(it.arp)) } }
                if (s.arp.on) Text("To keep it going after you lift your hand, turn on Latch.", color = TextDim, fontSize = 12.sp)
                TempoControls(accent)
            }
        },
        Option("Sides", KeyGrid.SIDE_CONTROLS.indices.count { s.side(it) != KeyGrid.OFF }.let { if (it == 0) "Off" else "$it" },
               perPart = false) {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                Text("Controls in the room either side of the grid. Each takes a strip; the grid fits what is left.",
                     color = TextDim, fontSize = 12.sp)
                for ((i, name) in KeyGrid.SIDE_CONTROLS.withIndex()) {
                    Text("$name · ${SIDE_HELP[i]}", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
                    Segmented(listOf("Off", "Left", "Right"), s.side(i), accent) { k ->
                        set { st -> st.copy(sides = KeyGrid.SIDE_CONTROLS.indices.map { if (it == i) k else st.side(it) }) } }
                }
            }
        },
    )
}

private val SIDE_HELP = listOf(
    "tap to latch, tap again to let go", "press and hold to sustain", "tap to arpeggiate",
    "slide for the octave", "slide to bend every note, springs back", "slide to brighten or darken every note")

@Composable
private fun AxisHelp() = Text(
    "Hold a square and move it; a whole square's move is the full change. Bend glides toward the note you drag to, " +
    "reaching it a square away. Loudness and tone start in the middle; volume only fades.", color = TextDim, fontSize = 12.sp)

/**
 * A look for the grid: the background, the squares (a shade apart from it, so every square is
 * plain to see), the light a played square fills with, and its hot white middle.
 */
class GridTheme(val name: String, val background: Color, val square: Color, val light: Color, val hot: Color,
                val onLight: Color, val bright: Boolean = false)

val GRID_THEMES = listOf(
    GridTheme("Neon", Color(0xFF000000), Color(0xFF0B1C18), Color(0xFF23FFD0), Color(0xFFDCFFF8), Color(0xFF002A22)),
    GridTheme("Ember", Color(0xFF0D0705), Color(0xFF27170F), Color(0xFFFF8A4C), Color(0xFFFFE2CC), Color(0xFF3A1200)),
    GridTheme("Ice", Color(0xFF04070D), Color(0xFF111A2A), Color(0xFF7FC4FF), Color(0xFFE6F3FF), Color(0xFF02203A)),
    GridTheme("Violet", Color(0xFF08040F), Color(0xFF1C1030), Color(0xFFC58BFF), Color(0xFFF3E6FF), Color(0xFF240A40)),
    GridTheme("Sunset", Color(0xFF0E0610), Color(0xFF2A1226), Color(0xFFFF5FA2), Color(0xFFFFE0EE), Color(0xFF3A0020)),
    GridTheme("Paper", Color(0xFFEDE9E1), Color(0xFFFFFFFF), Color(0xFF0E7C66), Color(0xFF6FE3C6), Color(0xFFFFFFFF), bright = true),
)
private val NEON = GRID_THEMES[0].light
private fun gridTheme(s: KeyGrid.Settings) = GRID_THEMES.getOrElse(s.theme) { GRID_THEMES[0] }

/** The grid's play surface. */
@Composable
fun KeyGridSurface() {
    val grid = remember { Grid() }
    var frame by remember { mutableIntStateOf(0) }
    DisposableEffect(Unit) { onDispose { grid.releaseAll() } }
    LaunchedEffect(Unit) {
        var last = 0L
        while (true) {
            val now = withFrameNanos { it }
            grid.tick(if (last == 0L) 0f else (now - last) / 1e9f)
            last = now
            if (!Eink || grid.takeDirty()) frame++
        }
    }
    val s = KeyGrid.settings
    grid.settings = s
    grid.center = (AppState.instrument(0)?.centerNote ?: 60f) + 12f * s.octave
    LaunchedEffect(s.latch) { if (!s.latch) grid.releaseLatched() }
    val th = gridTheme(s)
    val left = KeyGrid.SIDE_CONTROLS.indices.filter { s.side(it) == KeyGrid.LEFT }
    val right = KeyGrid.SIDE_CONTROLS.indices.filter { s.side(it) == KeyGrid.RIGHT }
    val turn = rememberPhoneTurn()
    Row(Modifier.fillMaxSize().background(if (Eink) Color.White else th.background)) {
        if (left.isNotEmpty()) SideStrip(left, grid, th, turn)
        GridCanvas(grid, Modifier.weight(1f).fillMaxHeight()) { frame }
        if (right.isNotEmpty()) SideStrip(right, grid, th, turn)
    }
}

@Composable
private fun GridCanvas(grid: Grid, modifier: Modifier, frame: () -> Int) {
    Canvas(modifier.clipToBounds().systemGestureExclusion()
        .pointerInput(Unit) {
            awaitPointerEventScope {
                while (true) {
                    val e = awaitPointerEvent()
                    grid.size(size.width.toFloat(), size.height.toFloat(), density)
                    for (c in e.changes) {
                        when {
                            c.pressed && !c.previousPressed -> grid.down(c.id.value, c.position.x, c.position.y)
                            c.pressed -> grid.move(c.id.value, c.position.x, c.position.y)
                            c.previousPressed -> grid.up(c.id.value)
                        }
                        c.consume()
                    }
                }
            }
        }) {
        frame()
        grid.size(size.width, size.height, density)
        grid.draw(this)
    }
}

/**
 * The squares and the fingers on them. Square n counts along the rows from the bottom left; the
 * grid keeps its squares square and sits at the bottom, leaving the top for the loop buttons.
 */
private class Grid {
    var settings = KeyGrid.Settings()
    var center = 60f
    private var w = 1f; private var h = 1f; private var density = 1f

    fun size(width: Float, height: Float, d: Float) { w = width; h = height; density = d }
    private val pad get() = 10f * density
    /** A square's side with its gap, and the grid's bottom left corner. */
    private val size get() = settings.squareSize.coerceIn(KeyGrid.SIZES)
    /** The height the grid may use: from the bottom up to below the loop buttons and sound chips. */
    private val room get() = h - pad - 140f * density
    /** The rows: as set, or with Fill as many as fit at the width's square size. */
    val rows get() = if (!settings.fillRows) settings.rows
        else floor(room / ((w - 2 * pad) / settings.cols * size)).toInt().coerceIn(1, KeyGrid.FILL_MAX)
    private val cell get() = minOf((w - 2 * pad) / settings.cols, room / rows) * size
    private val gap get() = cell * settings.spacing.coerceIn(KeyGrid.SPACINGS)
    private val left get() = (w - cell * settings.cols) / 2
    private val bottom get() = h - pad

    private val mask get() = SCALES[settings.scale].mask.let { if (it == 0) 0xFFF else it }
    private fun inScale(note: Int) = (mask shr (((note - settings.key) % 12 + 12) % 12)) and 1 == 1
    private val degrees get() = (0 until 12).filter { (mask shr it) and 1 == 1 }
    private val count get() = rows * settings.cols

    /** The note of square n. The grid starts on the key, so its middle sits near the middle of the instrument. */
    private fun noteOf(n: Int): Int {
        val step = if (settings.chromatic) 1 else degrees.size
        val span = if (settings.chromatic) count else 12 * count / step
        val base = settings.key + 12 * floor((center - span / 2f - settings.key) / 12f + 0.5f).toInt()
        val k = n.coerceIn(0, count - 1)
        return (if (settings.chromatic) base + k else base + 12 * (k / step) + degrees[k % step]).coerceIn(0, 127)
    }

    /** The column a square of the row is drawn in: right-handed the notes climb right to left. */
    private val mirrored get() = !settings.leftHanded
    private fun column(c: Int) = if (mirrored) settings.cols - 1 - c else c

    private fun squareAt(x: Float, y: Float): Int? {
        val c = column(floor((x - left) / cell).toInt().coerceIn(-1, settings.cols))
        val r = floor((bottom - y) / cell).toInt()
        if (c !in 0 until settings.cols || r !in 0 until rows) return null
        return r * settings.cols + c
    }
    private fun squareOrigin(n: Int) = Offset(left + column(n % settings.cols) * cell, bottom - (n / settings.cols + 1) * cell)

    private inner class Finger(val channel: Int, val n: Int, val note: Int, val x0: Float, val y0: Float) {
        var x = x0; var y = y0
        var sx = x0; var sy = y0            // the finger's slow position: the axes follow it, vibrato is what is left
        var pressure = -1; var slide = -1; var bend = -1; var volume = -1
        var t = 0f; var peak = 1e-4f; var light = 1f   // once latched: how long, how loud it was, how lit it is now
    }

    /**
     * A square still sounding after its finger lifted: its voice (by channel) is followed in the
     * engine, and the room's reverb rings on after it, as loud as this part's reverb send.
     */
    private class Ringing(val channel: Int, val note: Int, val n: Int, var peak: Float, val send: Float, val t60: Float) {
        var t = 0f; var voice = 1f; var glow = 1f
    }

    private val fingers = HashMap<Long, Finger>()
    /** Notes left sounding after their fingers lifted (latch, or hold), by channel, oldest first. */
    private val latched = LinkedHashMap<Int, Finger>()
    /** Fingers that came down to let a latched square go: they play nothing until they lift. */
    private val spent = HashSet<Long>()
    var latchedCount by mutableIntStateOf(0)
        private set
    private var holding = false
    /** From the side strips: a bend for every note (semitones), and a tone offset. */
    var sideBend = 0f
    var sideTone by mutableFloatStateOf(0f)
    private val ringing = mutableListOf<Ringing>()
    private val lastUsed = LongArray(16)
    private var clock = 0L
    private var dirty = true
    fun takeDirty() = dirty.also { dirty = false }
    private fun send(status: Int, d1: Int, d2: Int) = Native.nativeTouch(status, d1, d2)

    /** A channel no finger holds, the one silent longest, so a note still ringing on another is not cut off. */
    private fun freeChannel(): Int? {
        val used = fingers.values.map { it.channel }.toSet() + latched.keys
        (1..15).filter { it !in used }.minByOrNull { lastUsed[it] }?.let { return it }
        // Every channel busy: the oldest latched note makes way.
        val oldest = latched.keys.firstOrNull() ?: return null
        letGo(latched.remove(oldest)!!); latchedCount = latched.size
        return oldest
    }

    private val latching get() = settings.latch || holding

    fun down(id: Long, x: Float, y: Float) {
        val n = squareAt(x, y) ?: return
        if (settings.latch && settings.latchMode == KeyGrid.LATCH_TAP) {
            // Tap a held square to let it go.
            val held = latched.values.firstOrNull { it.n == n }
            if (held != null) { latched.remove(held.channel); letGo(held); latchedCount = latched.size; spent += id; dirty = true; return }
        } else if (settings.latch && fingers.isEmpty()) releaseLatched()    // a new hand: the last chord goes
        // The same note held over (hold, or a chord being added to) is played again, not doubled.
        latched.values.firstOrNull { it.n == n }?.let { latched.remove(it.channel); letGo(it); latchedCount = latched.size }
        val ch = freeChannel() ?: return
        lastUsed[ch] = ++clock
        ringing.removeAll { it.channel == ch }
        val f = Finger(ch, n, noteOf(n), x, y)
        fingers[id] = f
        val e = expression(f)
        f.volume = e.volume
        send(0xB0 or ch, 7, e.volume)
        send(0x90 or ch, f.note, (e.pressure / 5f).roundToInt().coerceIn(1, 127))
        sendExpression(f)
        dirty = true
    }

    fun move(id: Long, x: Float, y: Float) {
        val f = fingers[id] ?: return
        f.x = x; f.y = y
        sendExpression(f)
        dirty = true
    }

    fun up(id: Long) {
        if (spent.remove(id)) return
        val f = fingers.remove(id) ?: return
        if (latching) {
            // It stays as it is now (no vibrato left in it), until the latch or hold lets it go.
            f.sx = f.x; f.sy = f.y
            sendExpression(f)
            f.peak = levelOf(f.channel, f.note).coerceAtLeast(1e-4f); f.t = 0f
            latched[f.channel] = f; latchedCount = latched.size
        } else letGo(f)
        dirty = true
    }

    /** Note-off, and the square's afterglow follows the sound as it dies. */
    private fun letGo(f: Finger) {
        send(0x80 or f.channel, f.note, 64)
        lastUsed[f.channel] = ++clock
        val p = AppState.perf
        ringing += Ringing(f.channel, f.note, f.n, levelOf(f.channel, f.note).coerceAtLeast(1e-4f),
                           p.part(0).reverb, 0.7f + p.reverbSize * 5.3f)
        dirty = true
    }

    fun releaseLatched() {
        for (f in latched.values.toList()) letGo(f)
        latched.clear(); latchedCount = 0
    }

    /** The Hold strip: while pressed, lifted fingers leave their notes; let go and they go (unless latched). */
    fun hold(on: Boolean) {
        holding = on
        if (!on && !settings.latch) releaseLatched()
    }

    fun releaseAll() { for (id in fingers.keys.toList()) up(id); spent.clear(); holding = false; releaseLatched() }

    // ---- what the engine is playing ------------------------------------------------------------
    private val voices = FloatArray(32 * 8)
    private var nVoices = 0
    /**
     * How loud the voice playing [note] on [channel] is now. Once its finger lifts the engine no
     * longer says whose it was (finger -1), so a ringing voice is found by its note.
     */
    private fun levelOf(channel: Int, note: Int): Float {
        var level = 0f
        for (i in 0 until nVoices) {
            val b = 8 * i
            val finger = voices[b + 1].toInt()
            if (voices[b + 2].toInt() == note && (finger == channel || finger == -1)) level = maxOf(level, voices[b + 7])
        }
        return level
    }
    /** A level as light: 0 dB is full, 42 dB down is dark, so the light fades as the ear hears it fade. */
    private fun light(ratio: Float) = if (ratio <= 0f) 0f else (1f + 20f * log10(ratio) / 42f).coerceIn(0f, 1f)

    fun tick(dt: Float) {
        nVoices = Native.nativeVoices(voices)
        // The slow position trails the finger by about 0.12 s: a wiggle is faster than that.
        val k = 1f - exp(-dt / 0.12f)
        for (f in fingers.values) {
            f.sx += (f.x - f.sx) * k; f.sy += (f.y - f.sy) * k
            sendExpression(f)
        }
        for (f in latched.values) {
            sendExpression(f)
            f.t += dt
            val level = levelOf(f.channel, f.note)
            if (f.t < 0.05f) f.peak = maxOf(f.peak, level)
            f.light = light(level / f.peak)
        }
        for (r in ringing) {
            r.t += dt
            val level = levelOf(r.channel, r.note)
            if (r.t < 0.05f) r.peak = maxOf(r.peak, level)          // the level as it was let go
            r.voice = light(level / r.peak)
            // The reverb's tail: the send's share of the sound, dying away over the room's T60.
            val tail = if (r.send <= 0.001f) 0f else light(r.send * exp(-6.9f * r.t / r.t60)) * 0.8f
            r.glow = maxOf(r.voice, tail)
        }
        ringing.removeAll { it.glow <= 0.004f && it.t > 0.05f }
        if (fingers.isNotEmpty() || ringing.isNotEmpty() || latched.isNotEmpty()) dirty = true
    }

    private class Expr(val pressure: Int, val slide: Int, val bend: Int, val volume: Int)

    /** The axes, from how far the finger has moved (slowly) from where it came down; vibrato from the rest. */
    private fun expression(f: Finger): Expr {
        val dx = (f.sx - f.x0) / cell
        val dy = (f.y0 - f.sy) / cell
        var pressure = 0.75f; var slide = 0.5f; var semis = 0f; var volume = 1f
        fun apply(target: Int, d: Float, across: Boolean) {
            val v = d.coerceIn(-1f, 1f)
            when (target) {
                KeyGrid.LOUDNESS -> pressure = (0.75f + 0.5f * v).coerceIn(0.02f, 1f)
                KeyGrid.TONE -> slide = (0.5f + 0.5f * v).coerceIn(0f, 1f)
                // Toward the note you drag to: across, the next square or the one before; up and down, the row above or below.
                KeyGrid.BEND -> {
                    // Right-handed the next note is to the left: dragging right goes down the scale.
                    val step = if (!across) settings.cols else if (mirrored) -1 else 1
                    val to = noteOf(f.n + if (v >= 0) step else -step)
                    val amount = ((abs(v) - 0.08f) / 0.92f).coerceIn(0f, 1f)
                    semis += (to - f.note) * amount
                }
                KeyGrid.VOLUME -> volume = (1f + v).coerceIn(0f, 1f)
            }
        }
        apply(settings.xTarget, dx, true)
        apply(settings.yTarget, dy, false)
        semis += sideBend
        slide = (slide + sideTone).coerceIn(0f, 1f)
        val depth = PadSettings.VIBRATO_DEPTH.getOrElse(settings.vibrato) { 0f }
        if (depth > 0f) semis += ((f.x - f.sx) / density * 0.08f * depth / 0.5f).coerceIn(-depth, depth)
        val bend = (8192 + semis / 48f * 8192).roundToInt().coerceIn(0, 16383)
        return Expr((pressure * 127).roundToInt(), (slide * 127).roundToInt(), bend, (volume * 127).roundToInt())
    }

    private fun sendExpression(f: Finger) {
        val e = expression(f)
        if (e.volume != f.volume) { f.volume = e.volume; send(0xB0 or f.channel, 7, e.volume) }
        if (e.bend != f.bend) { f.bend = e.bend; send(0xE0 or f.channel, e.bend and 0x7F, e.bend shr 7) }
        if (e.slide != f.slide) { f.slide = e.slide; send(0xB0 or f.channel, 74, e.slide) }
        if (e.pressure != f.pressure) { f.pressure = e.pressure; send(0xD0 or f.channel, e.pressure, 0) }
    }

    // ---- drawing: lit outlines on squares a shade apart from the background; a held square full of light ----
    fun draw(d: DrawScope) = with(d) {
        val flat = Eink
        val th = gridTheme(settings)
        drawRect(if (flat) Color.White else th.background)
        val light = if (flat) Color.Black else th.light
        val text = android.graphics.Paint().apply { isAntiAlias = true; textAlign = android.graphics.Paint.Align.CENTER }
        val held = fingers.values.map { it.n }.toSet()
        val glowOf = FloatArray(count)
        for (r in ringing) if (r.n < count) glowOf[r.n] = maxOf(glowOf[r.n], r.glow)
        val kept = HashSet<Int>()
        for (f in latched.values) if (f.n < count) { kept += f.n; glowOf[f.n] = maxOf(glowOf[f.n], f.light) }
        val radius = CornerRadius(cell * 0.08f)
        val side = cell - gap
        val dash = PathEffect.dashPathEffect(floatArrayOf(6f * density, 5f * density))
        for (n in 0 until count) {
            val o = squareOrigin(n) + Offset(gap / 2, gap / 2)
            val sz = Size(side, side)
            val note = noteOf(n)
            val root = ((note - settings.key) % 12 + 12) % 12 == 0
            val off = settings.chromatic && !inScale(note)
            val on = n in held
            val glow = if (on) 1f else glowOf[n]
            // The square itself, a shade apart from the background; a note outside the scale sinks into it.
            if (!flat) drawRoundRect(if (off) th.square.copy(alpha = 0.35f) else th.square, o, sz, radius)
            // The light: an outer bloom, then the fill.
            if (glow > 0f) {
                if (!flat && !th.bright) drawRoundRect(light.copy(alpha = 0.28f * glow), o - Offset(6f * density, 6f * density),
                                                       Size(side + 12f * density, side + 12f * density), CornerRadius(radius.x + 6f * density))
                drawRoundRect(light.copy(alpha = (if (flat) 1f else 0.92f) * glow), o, sz, radius)
            }
            // The outline: the root's fully lit, the scale's soft, a note outside the scale broken.
            val latchedHere = n in kept
            val edge = when {
                on || glow > 0.6f || latchedHere -> if (flat) Color.Black else th.hot
                root -> light
                off -> light.copy(alpha = if (flat) 0.4f else 0.18f)
                else -> light.copy(alpha = if (flat) 0.8f else 0.38f)
            }
            if (root && !flat && !on && !th.bright) drawRoundRect(light.copy(alpha = 0.18f), o - Offset(3f * density, 3f * density),
                                                                  Size(side + 6f * density, side + 6f * density), CornerRadius(radius.x + 3f * density),
                                                                  style = Stroke(width = 6f * density))
            drawRoundRect(edge, o, sz, radius, style = Stroke(width = (if (root) 2.5f else 1.5f) * density,
                                                              pathEffect = if (off && glow == 0f) dash else null))
            // A latched square: a dot in its corner, as long as it is held.
            if (latchedHere && !on) drawCircle(if (glow > 0.5f) (if (flat) Color.White else th.onLight) else (if (flat) Color.Black else th.hot),
                                               side * 0.06f, o + Offset(side * 0.84f, side * 0.16f))
            if (settings.labels) {
                text.textSize = side * 0.16f
                text.isFakeBoldText = root
                text.color = (if (glow > 0.5f) (if (flat) Color.White else th.onLight) else light.copy(alpha = if (off) 0.4f else 0.75f)).toArgb()
                val name = NOTE_NAMES[note % 12] + if (root) "${note / 12 - 1}" else ""
                drawContext.canvas.nativeCanvas.drawText(name, o.x + side / 2, o.y + side / 2 + text.textSize * 0.35f, text)
            }
        }
        // A halo that follows each finger.
        if (!flat && !th.bright) for (f in fingers.values) {
            val r = cell * 0.9f
            drawCircle(Brush.radialGradient(listOf(th.hot.copy(alpha = 0.5f), light.copy(alpha = 0.22f), light.copy(alpha = 0f)),
                                            center = Offset(f.x, f.y), radius = r), r, Offset(f.x, f.y))
        }
    }
}


// ---- the side strips --------------------------------------------------------------------------

/** A strip of controls beside the grid, from the bottom up, where the thumb reaches. */
@Composable
private fun SideStrip(controls: List<Int>, grid: Grid, th: GridTheme, turn: Float) {
    val s = KeyGrid.settings
    val ink = if (Eink) Color.Black else th.light
    Column(Modifier.width(64.dp).fillMaxHeight().padding(start = 6.dp, end = 6.dp, top = 150.dp, bottom = 10.dp)
               .systemGestureExclusion(),
           verticalArrangement = Arrangement.spacedBy(10.dp, Alignment.Bottom)) {
        for (c in controls) when (c) {
            KeyGrid.S_LATCH -> SideButton("Latch", if (!s.latch) "off" else if (grid.latchedCount > 0) "${grid.latchedCount} held" else "on",
                                          s.latch, th, ink, turn, onTap = { KeyGrid.update { it.copy(latch = !it.latch) } })
            KeyGrid.S_HOLD -> {
                var down by remember { mutableStateOf(false) }
                SideButton("Hold", if (down) "holding" else "press", down, th, ink, turn,
                           onPress = { v -> down = v; grid.hold(v) })
            }
            KeyGrid.S_ARP -> SideButton("Arp", if (s.arp.on) ARP_ORDERS[s.arp.order] else "off", s.arp.on, th, ink, turn,
                                        onTap = { KeyGrid.update { it.copy(arp = it.arp.copy(on = !it.arp.on)) } })
            KeyGrid.S_OCTAVE -> SideSlider("Oct", if (s.octave == 0) "0" else "%+d".format(s.octave),
                                           (s.octave + 3) / 6f, th, ink, turn, ticks = 7, centre = false,
                                           drag = { _, at -> val o = (at * 6).roundToInt() - 3
                                                             if (o != KeyGrid.settings.octave) KeyGrid.update { it.copy(octave = o) } })
            KeyGrid.S_BEND -> {
                var shown by remember { mutableFloatStateOf(0f) }
                SideSlider("Bend", if (shown == 0f) "0" else "%+.1f".format(shown), 0.5f + shown / 4f, th, ink, turn, ticks = 5, centre = true,
                           drag = { from, at -> shown = ((at - from) * 4f).coerceIn(-2f, 2f); grid.sideBend = shown },
                           lift = { shown = 0f; grid.sideBend = 0f })
            }
            KeyGrid.S_TONE -> SideSlider("Tone", "%+d".format((grid.sideTone * 200).roundToInt()), 0.5f + grid.sideTone, th, ink, turn,
                                         ticks = 3, centre = true, drag = { _, at -> grid.sideTone = (at - 0.5f).coerceIn(-0.5f, 0.5f) })
        }
    }
}

/** A square button: a tap ([onTap]) or held down ([onPress] with true, then false). */
@Composable
private fun SideButton(label: String, value: String, on: Boolean, th: GridTheme, ink: Color, turn: Float,
                       onTap: (() -> Unit)? = null, onPress: ((Boolean) -> Unit)? = null) {
    val shape = RoundedCornerShape(10.dp)
    // Kept current without restarting the gesture: a restart mid-press would never see the lift.
    val tap by rememberUpdatedState(onTap)
    val press by rememberUpdatedState(onPress)
    Box(Modifier.fillMaxWidth().height(52.dp)
            .background(if (on) ink.copy(alpha = if (Eink) 1f else 0.85f) else if (Eink) Color.White else th.square, shape)
            .border(1.5.dp, if (on) (if (Eink) Color.Black else th.hot) else ink.copy(alpha = 0.5f), shape)
            .pointerInput(Unit) {
                awaitEachGesture {
                    awaitFirstDown().consume()
                    press?.invoke(true)
                    while (true) {
                        val e = awaitPointerEvent()
                        e.changes.forEach { it.consume() }
                        if (e.changes.none { it.pressed }) break
                    }
                    press?.invoke(false)
                    tap?.invoke()
                }
            },
        contentAlignment = Alignment.Center) {
        val text = if (on) (if (Eink) Color.White else th.onLight) else ink
        Column(Modifier.rotate(turn), horizontalAlignment = Alignment.CenterHorizontally) {
            Text(label, color = text, fontSize = 12.sp, fontWeight = FontWeight.SemiBold, maxLines = 1)
            Text(value, color = text.copy(alpha = 0.75f), fontSize = 10.sp, maxLines = 1)
        }
    }
}

/**
 * A tall slider: [at] runs 0 at the bottom to 1 at the top. [drag] has where the finger came down
 * and where it is now; [lift] when it lets go.
 */
@Composable
private fun ColumnScope.SideSlider(label: String, value: String, at: Float, th: GridTheme, ink: Color, turn: Float, ticks: Int,
                                   centre: Boolean, drag: (Float, Float) -> Unit, lift: () -> Unit = {}) {
    var touched by remember { mutableStateOf(false) }
    val onDrag by rememberUpdatedState(drag)
    val onLift by rememberUpdatedState(lift)
    // No taller than a thumb's reach, and clear of the loop buttons at the top.
    Box(Modifier.fillMaxWidth().weight(1f, fill = false).heightIn(max = 300.dp)
            .pointerInput(Unit) {
                awaitEachGesture {
                    val d = awaitFirstDown(); d.consume()
                    fun frac(y: Float) = (1f - y / size.height).coerceIn(0f, 1f)
                    val from = frac(d.position.y)
                    touched = true
                    onDrag(from, from)
                    while (true) {
                        val e = awaitPointerEvent()
                        val c = e.changes.firstOrNull { it.id == d.id } ?: break
                        c.consume()
                        if (!c.pressed) break
                        onDrag(from, frac(c.position.y))
                    }
                    touched = false
                    onLift()
                }
            }) {
        Canvas(Modifier.fillMaxSize()) {
            val r = CornerRadius(10.dp.toPx())
            drawRoundRect(if (Eink) Color.White else th.square, cornerRadius = r)
            drawRoundRect(ink.copy(alpha = if (touched) 0.9f else 0.5f), cornerRadius = r, style = Stroke(1.5.dp.toPx()))
            val top = 46.dp.toPx(); val bottom = size.height - 12.dp.toPx()
            fun yOf(f: Float) = bottom - (bottom - top) * f
            for (i in 0 until ticks) {
                val y = yOf(i / (ticks - 1f))
                drawLine(ink.copy(alpha = 0.3f), Offset(size.width * 0.3f, y), Offset(size.width * 0.7f, y), 1.dp.toPx())
            }
            val y = yOf(at.coerceIn(0f, 1f))
            val mid = yOf(0.5f)
            val x = size.width / 2
            if (centre) drawLine(ink.copy(alpha = 0.6f), Offset(x, mid), Offset(x, y), 4.dp.toPx())
            else drawLine(ink.copy(alpha = 0.6f), Offset(x, bottom), Offset(x, y), 4.dp.toPx())
            drawRoundRect(if (Eink) Color.Black else th.hot, Offset(size.width * 0.18f, y - 5.dp.toPx()),
                          Size(size.width * 0.64f, 10.dp.toPx()), CornerRadius(5.dp.toPx()))
        }
        Column(Modifier.align(Alignment.TopCenter).padding(top = 4.dp).rotate(turn), horizontalAlignment = Alignment.CenterHorizontally) {
            Text(label, color = ink, fontSize = 11.sp, fontWeight = FontWeight.SemiBold, maxLines = 1)
            Text(value, color = ink.copy(alpha = 0.75f), fontSize = 10.sp, maxLines = 1)
        }
    }
}
