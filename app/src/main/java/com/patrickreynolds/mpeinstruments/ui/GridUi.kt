package com.patrickreynolds.mpeinstruments.ui

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Column
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
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Modifier
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
        Option("Grid", "${s.cols} × ${s.rows}", perPart = false) {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                Text("Squares across", color = TextDim, fontSize = 13.sp)
                Chips(KeyGrid.COLS.map { "$it" }, s.cols - KeyGrid.COLS.first, accent) { k -> set { it.copy(cols = KeyGrid.COLS.first + k) } }
                Text("Rows up", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
                Chips(KeyGrid.ROWS.map { "$it" }, s.rows - KeyGrid.ROWS.first, accent) { k -> set { it.copy(rows = KeyGrid.ROWS.first + k) } }
                LabeledSlider("Square size", s.squareSize, KeyGrid.SIZES, "${(s.squareSize * 100).roundToInt()}%", accent = accent) { v ->
                    set { it.copy(squareSize = v.coerceIn(KeyGrid.SIZES)) } }
                LabeledSlider("Spacing", s.spacing, KeyGrid.SPACINGS, "${(s.spacing * 100).roundToInt()}%", accent = accent) { v ->
                    set { it.copy(spacing = v.coerceIn(KeyGrid.SPACINGS)) } }
                Text("Size is a share of the biggest grid that fits the screen; spacing a share of each square's room.",
                     color = TextDim, fontSize = 12.sp)
                Text("Hand", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
                Segmented(listOf("Right-handed", "Left-handed"), if (s.leftHanded) 1 else 0, accent) { k -> set { it.copy(leftHanded = k == 1) } }
                Text("The notes climb row by row from the bottom " + (if (s.leftHanded) "right, right to left" else "left, left to right") +
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
        Option("Arp", if (s.arp.on) ARP_ORDERS[s.arp.order] else "Off", perPart = false) {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                ArpControls(s.arp, accent) { c -> set { it.copy(arp = c(it.arp)) } }
                TempoControls(accent)
            }
        },
    )
}

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
    grid.settings = KeyGrid.settings
    grid.center = (AppState.instrument(0)?.centerNote ?: 60f) + 12f * KeyGrid.settings.octave
    Canvas(Modifier.fillMaxSize().clipToBounds().systemGestureExclusion()
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
        frame.let { }
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
    private val cell get() = minOf((w - 2 * pad) / settings.cols, (h - 2 * pad) / settings.rows) *
        settings.squareSize.coerceIn(KeyGrid.SIZES)
    private val gap get() = cell * settings.spacing.coerceIn(KeyGrid.SPACINGS)
    private val left get() = (w - cell * settings.cols) / 2
    private val bottom get() = h - pad

    private val mask get() = SCALES[settings.scale].mask.let { if (it == 0) 0xFFF else it }
    private fun inScale(note: Int) = (mask shr (((note - settings.key) % 12 + 12) % 12)) and 1 == 1
    private val degrees get() = (0 until 12).filter { (mask shr it) and 1 == 1 }
    private val count get() = settings.rows * settings.cols

    /** The note of square n. The grid starts on the key, so its middle sits near the middle of the instrument. */
    private fun noteOf(n: Int): Int {
        val step = if (settings.chromatic) 1 else degrees.size
        val span = if (settings.chromatic) count else 12 * count / step
        val base = settings.key + 12 * floor((center - span / 2f - settings.key) / 12f + 0.5f).toInt()
        val k = n.coerceIn(0, count - 1)
        return (if (settings.chromatic) base + k else base + 12 * (k / step) + degrees[k % step]).coerceIn(0, 127)
    }

    /** The column a square of the row is drawn in: mirrored for the left hand. */
    private fun column(c: Int) = if (settings.leftHanded) settings.cols - 1 - c else c

    private fun squareAt(x: Float, y: Float): Int? {
        val c = column(floor((x - left) / cell).toInt().coerceIn(-1, settings.cols))
        val r = floor((bottom - y) / cell).toInt()
        if (c !in 0 until settings.cols || r !in 0 until settings.rows) return null
        return r * settings.cols + c
    }
    private fun squareOrigin(n: Int) = Offset(left + column(n % settings.cols) * cell, bottom - (n / settings.cols + 1) * cell)

    private inner class Finger(val channel: Int, val n: Int, val note: Int, val x0: Float, val y0: Float) {
        var x = x0; var y = y0
        var sx = x0; var sy = y0            // the finger's slow position: the axes follow it, vibrato is what is left
        var pressure = -1; var slide = -1; var bend = -1; var volume = -1
    }

    /**
     * A square still sounding after its finger lifted: its voice (by channel) is followed in the
     * engine, and the room's reverb rings on after it, as loud as this part's reverb send.
     */
    private class Ringing(val channel: Int, val note: Int, val n: Int, var peak: Float, val send: Float, val t60: Float) {
        var t = 0f; var voice = 1f; var glow = 1f
    }

    private val fingers = HashMap<Long, Finger>()
    private val ringing = mutableListOf<Ringing>()
    private val lastUsed = LongArray(16)
    private var clock = 0L
    private var dirty = true
    fun takeDirty() = dirty.also { dirty = false }
    private fun send(status: Int, d1: Int, d2: Int) = Native.nativeTouch(status, d1, d2)

    /** A channel no finger holds, the one silent longest, so a note still ringing on another is not cut off. */
    private fun freeChannel(): Int? {
        val used = fingers.values.map { it.channel }.toSet()
        return (1..15).filter { it !in used }.minByOrNull { lastUsed[it] }
    }

    fun down(id: Long, x: Float, y: Float) {
        val n = squareAt(x, y) ?: return
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
        val f = fingers.remove(id) ?: return
        send(0x80 or f.channel, f.note, 64)
        lastUsed[f.channel] = ++clock
        val p = AppState.perf
        ringing += Ringing(f.channel, f.note, f.n, levelOf(f.channel, f.note).coerceAtLeast(1e-4f),
                           p.part(0).reverb, 0.7f + p.reverbSize * 5.3f)
        dirty = true
    }

    fun releaseAll() { for (id in fingers.keys.toList()) up(id) }

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
        if (fingers.isNotEmpty() || ringing.isNotEmpty()) dirty = true
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
                    // Left-handed the next note is to the left: dragging right goes down the scale.
                    val step = if (!across) settings.cols else if (settings.leftHanded) -1 else 1
                    val to = noteOf(f.n + if (v >= 0) step else -step)
                    val amount = ((abs(v) - 0.08f) / 0.92f).coerceIn(0f, 1f)
                    semis += (to - f.note) * amount
                }
                KeyGrid.VOLUME -> volume = (1f + v).coerceIn(0f, 1f)
            }
        }
        apply(settings.xTarget, dx, true)
        apply(settings.yTarget, dy, false)
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
            val edge = when {
                on || glow > 0.6f -> if (flat) Color.Black else th.hot
                root -> light
                off -> light.copy(alpha = if (flat) 0.4f else 0.18f)
                else -> light.copy(alpha = if (flat) 0.8f else 0.38f)
            }
            if (root && !flat && !on && !th.bright) drawRoundRect(light.copy(alpha = 0.18f), o - Offset(3f * density, 3f * density),
                                                                  Size(side + 6f * density, side + 6f * density), CornerRadius(radius.x + 3f * density),
                                                                  style = Stroke(width = 6f * density))
            drawRoundRect(edge, o, sz, radius, style = Stroke(width = (if (root) 2.5f else 1.5f) * density,
                                                              pathEffect = if (off && glow == 0f) dash else null))
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
