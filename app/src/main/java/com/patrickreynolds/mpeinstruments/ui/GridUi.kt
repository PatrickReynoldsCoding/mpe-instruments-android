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
import kotlin.math.exp
import kotlin.math.floor
import kotlin.math.roundToInt

/** The grid's options on the rail. */
@Composable
fun keyGridOptions(): List<Option> {
    val s = KeyGrid.settings
    val accent = if (Eink) Color.Black else PartColors[0]
    fun set(change: (KeyGrid.Settings) -> KeyGrid.Settings) = KeyGrid.update(change)
    return listOf(
        Option("Grid", "${s.rows} × ${s.cols}", perPart = false) {
            Text("Rows", color = TextDim, fontSize = 13.sp)
            Chips(KeyGrid.ROWS.map { "$it" }, s.rows - KeyGrid.ROWS.first, accent) { k -> set { it.copy(rows = KeyGrid.ROWS.first + k) } }
            Text("Squares in a row", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
            Chips(KeyGrid.COLS.map { "$it" }, s.cols - KeyGrid.COLS.first, accent) { k -> set { it.copy(cols = KeyGrid.COLS.first + k) } }
            Text("Each row carries on from the one below it, like keyboards stacked.", color = TextDim, fontSize = 12.sp)
        },
        Option("Notes", if (s.chromatic) "Chromatic" else SCALES[s.scale].name, perPart = false) {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                Segmented(listOf("The scale's notes", "Chromatic"), if (s.chromatic) 1 else 0, accent) { k -> set { it.copy(chromatic = k == 1) } }
                Text(if (s.chromatic) "Every semitone; the notes outside the scale are darkened, like a piano's black keys."
                     else "Only the scale's notes, so every square fits.", color = TextDim, fontSize = 12.sp)
                Text("Scale", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
                Chips(SCALES.drop(1).map { it.name }, s.scale - 1, accent) { k -> set { it.copy(scale = k + 1) } }
                Text("Key", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
                Chips(NOTE_NAMES, s.key, accent) { k -> set { it.copy(key = k) } }
            }
        },
        Option("Octave", if (s.octave == 0) "0" else "%+d".format(s.octave), perPart = false) {
            Chips(listOf("−3", "−2", "−1", "0", "+1", "+2", "+3"), s.octave + 3, accent) { k -> set { it.copy(octave = k - 3) } }
        },
        Option("Across", KeyGrid.TARGETS[s.xTarget], perPart = false) {
            Chips(KeyGrid.TARGETS, s.xTarget, accent) { k -> set { it.copy(xTarget = k) } }
            axisHelp(s.bendRange)
            if (s.xTarget == KeyGrid.BEND || s.yTarget == KeyGrid.BEND) BendRange(s.bendRange, accent)
        },
        Option("Up/down", KeyGrid.TARGETS[s.yTarget], perPart = false) {
            Chips(KeyGrid.TARGETS, s.yTarget, accent) { k -> set { it.copy(yTarget = k) } }
            axisHelp(s.bendRange)
            if (s.xTarget == KeyGrid.BEND || s.yTarget == KeyGrid.BEND) BendRange(s.bendRange, accent)
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
        Option("Labels", if (s.labels) "On" else "Off", perPart = false, on = false, tap = { set { it.copy(labels = !it.labels) } }),
    )
}

@Composable
private fun axisHelp(bend: Int) = Text(
    "Hold a square and move it: a whole square's move is the full change. Loudness and tone start in the middle; volume " +
    "only fades; bend goes up to $bend semitones.", color = TextDim, fontSize = 12.sp)

@Composable
private fun BendRange(v: Int, accent: Color) {
    Text("Bend for a whole square", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
    Chips(listOf("1", "2", "3", "5", "7", "12"), listOf(1, 2, 3, 5, 7, 12).indexOf(v), accent) { k ->
        KeyGrid.update { it.copy(bendRange = listOf(1, 2, 3, 5, 7, 12)[k]) } }
}

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
    val theme = hostTheme()
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
        grid.draw(this, theme)
    }
}

/** The squares and the fingers on them. Row 0 is the bottom one; a row carries on from the one below. */
private class Grid {
    var settings = KeyGrid.Settings()
    var center = 60f
    private var w = 1f; private var h = 1f; private var density = 1f
    private val gap get() = 6f * density

    fun size(width: Float, height: Float, d: Float) { w = width; h = height; density = d }
    private val cw get() = w / settings.cols
    private val ch get() = h / settings.rows

    private val mask get() = SCALES[settings.scale].mask.let { if (it == 0) 0xFFF else it }
    private fun inScale(note: Int) = (mask shr (((note - settings.key) % 12 + 12) % 12)) and 1 == 1
    private val degrees get() = (0 until 12).filter { (mask shr it) and 1 == 1 }

    /** The note of square n (counted along the rows from the bottom left). */
    private fun noteOf(n: Int): Int {
        val count = settings.rows * settings.cols
        val step = if (settings.chromatic) 1 else degrees.size
        val span = if (settings.chromatic) count else 12 * count / step
        // Start on the key, so the middle of the grid sits near the middle of the instrument.
        val low = center - span / 2f
        val base = settings.key + 12 * floor((low - settings.key) / 12f + 0.5f).toInt()
        return (if (settings.chromatic) base + n else base + 12 * (n / step) + degrees[n % step]).coerceIn(0, 127)
    }

    private fun squareAt(x: Float, y: Float): Pair<Int, Int> {
        val c = (x / cw).toInt().coerceIn(0, settings.cols - 1)
        val r = (settings.rows - 1 - (y / ch).toInt()).coerceIn(0, settings.rows - 1)
        return r to c
    }

    private inner class Finger(val channel: Int, val note: Int, val r: Int, val c: Int, val x0: Float, val y0: Float) {
        var x = x0; var y = y0
        var sx = x0; var sy = y0            // the finger's slow position: the axes follow it, vibrato is what is left
        var pressure = -1; var slide = -1; var bend = -1; var volume = -1
        var glow = 1f
    }

    private val fingers = HashMap<Long, Finger>()
    private var dirty = true
    fun takeDirty() = dirty.also { dirty = false }
    private fun send(status: Int, d1: Int, d2: Int) = Native.nativeTouch(status, d1, d2)

    fun down(id: Long, x: Float, y: Float) {
        val used = fingers.values.map { it.channel }.toSet()
        val ch = (15 downTo 1).firstOrNull { it !in used } ?: return
        val (r, c) = squareAt(x, y)
        val f = Finger(ch, noteOf(r * settings.cols + c), r, c, x, y)
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
        lifted += f
        dirty = true
    }

    fun releaseAll() { for (id in fingers.keys.toList()) up(id) }

    private val lifted = mutableListOf<Finger>()

    fun tick(dt: Float) {
        // The slow position trails the finger by about 0.12 s: a wiggle is faster than that.
        val k = 1f - exp(-dt / 0.12f)
        for (f in fingers.values) {
            f.sx += (f.x - f.sx) * k; f.sy += (f.y - f.sy) * k
            sendExpression(f)
        }
        for (f in lifted) f.glow -= dt * 2.5f
        lifted.removeAll { it.glow <= 0f }
        if (fingers.isNotEmpty() || lifted.isNotEmpty()) dirty = true
    }

    private class Expr(val pressure: Int, val slide: Int, val bend: Int, val volume: Int)

    /** The axes, from how far the finger has moved (slowly) from where it came down; vibrato from the rest. */
    private fun expression(f: Finger): Expr {
        val dx = (f.sx - f.x0) / cw
        val dy = (f.y0 - f.sy) / ch
        var pressure = 0.7f; var slide = 0.5f; var semis = 0f; var volume = 1f
        fun apply(target: Int, d: Float) {
            val v = d.coerceIn(-1f, 1f)
            when (target) {
                KeyGrid.LOUDNESS -> pressure = (0.7f + 0.5f * v).coerceIn(0.02f, 1f)
                KeyGrid.TONE -> slide = (0.5f + 0.5f * v).coerceIn(0f, 1f)
                KeyGrid.BEND -> semis += v * settings.bendRange
                KeyGrid.VOLUME -> volume = (1f + v).coerceIn(0f, 1f)
            }
        }
        apply(settings.xTarget, dx)
        apply(settings.yTarget, dy)
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

    fun draw(d: DrawScope, t: PadTheme) = with(d) {
        val flat = t.flat
        if (flat) drawRect(Color.White) else drawRect(Brush.verticalGradient(listOf(t.top, t.mid, t.bottom)))
        val accent = if (flat) Color.Black else t.parts[0]
        val text = android.graphics.Paint().apply { isAntiAlias = true; textAlign = android.graphics.Paint.Align.CENTER }
        val held = fingers.values.associateBy { it.r to it.c }
        for (r in 0 until settings.rows) for (c in 0 until settings.cols) {
            val note = noteOf(r * settings.cols + c)
            val x = c * cw + gap / 2; val y = (settings.rows - 1 - r) * ch + gap / 2
            val sz = Size(cw - gap, ch - gap)
            val off = settings.chromatic && !inScale(note)
            val root = ((note - settings.key) % 12 + 12) % 12 == 0
            val f = held[r to c]
            val fade = lifted.filter { it.r == r && it.c == c }.maxOfOrNull { it.glow } ?: 0f
            val fill = when {
                flat && f != null -> Color.Black
                flat && off -> Color(0xFFBDBDBD)
                flat -> Color.White
                f != null -> accent.copy(alpha = 0.75f)
                off -> Color.Black.copy(alpha = 0.55f)
                root -> t.line.copy(alpha = 0.16f)
                else -> t.line.copy(alpha = 0.08f)
            }
            drawRoundRect(fill, Offset(x, y), sz, CornerRadius(14f * density))
            if (!flat && fade > 0f) drawRoundRect(accent.copy(alpha = 0.5f * fade), Offset(x, y), sz, CornerRadius(14f * density))
            drawRoundRect(if (flat) Color.Black else if (root) accent.copy(alpha = 0.7f) else t.line.copy(alpha = 0.18f), Offset(x, y), sz,
                          CornerRadius(14f * density), style = Stroke(width = if (root) 3f * density else 1.5f * density))
            if (settings.labels) {
                text.textSize = minOf(cw, ch) * 0.22f
                text.isFakeBoldText = root
                text.color = (when { flat && f != null -> Color.White; flat -> Color.Black; f != null -> Color.Black
                                     off -> t.label.copy(alpha = 0.45f); else -> t.label.copy(alpha = 0.9f) }).toArgb()
                val name = NOTE_NAMES[note % 12] + if (root) "${note / 12 - 1}" else ""
                drawContext.canvas.nativeCanvas.drawText(name, x + sz.width / 2, y + sz.height / 2 + text.textSize * 0.35f, text)
            }
        }
        // Each finger: a light where it is, a line back to where it came down (the axes' move).
        for (f in fingers.values) {
            if (!flat) drawLine(Color.White.copy(alpha = 0.5f), Offset(f.x0, f.y0), Offset(f.sx, f.sy), strokeWidth = 3f * density)
            if (!flat) drawCircle(Brush.radialGradient(listOf(accent.copy(alpha = 0.55f), accent.copy(alpha = 0f)), center = Offset(f.x, f.y),
                                                       radius = 70f * density), 70f * density, Offset(f.x, f.y))
            drawCircle(if (flat) Color.Black else Color.White, 9f * density, Offset(f.x, f.y))
        }
    }
}
