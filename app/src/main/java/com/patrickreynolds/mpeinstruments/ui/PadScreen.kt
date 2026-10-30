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
import androidx.compose.foundation.layout.padding
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
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.NOTE_NAMES
import com.patrickreynolds.mpeinstruments.Native
import com.patrickreynolds.mpeinstruments.PadSettings
import com.patrickreynolds.mpeinstruments.SCALES
import com.patrickreynolds.mpeinstruments.noteName
import kotlin.math.abs
import kotlin.math.exp
import kotlin.math.floor
import kotlin.math.roundToInt

/**
 * The screen as an instrument: columns of notes left to right, each finger its own MPE voice.
 * Slide sideways to glide, up for louder and brighter. The notes go into the engine through
 * the same parser as the Seaboard's (Native.nativeTouch), so the arp, split and effects all
 * apply. A Seaboard played at the same time shows here too.
 */
@Composable
fun PadScreen(onClose: () -> Unit) {
    val pad = remember { Pad() }
    var frame by remember { mutableIntStateOf(0) }
    var panel by remember { mutableStateOf<String?>(null) }
    val s = AppState.pad
    val inst = AppState.instrument(0)
    val theme = padTheme(s)
    pad.layout(s, inst?.centerNote ?: 60f, theme)
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
    Box(Modifier.fillMaxSize().background(theme.bottom)) {
      Row(Modifier.fillMaxSize()) {
        Rail(s, theme, inst?.name ?: "", panel, { panel = if (panel == it) null else it }, onClose)
        Canvas(Modifier.weight(1f).fillMaxHeight().systemGestureExclusion()
            .pointerInput(Unit) {
                awaitPointerEventScope {
                    while (true) {
                        val e = awaitPointerEvent()
                        val strip = STRIP.toPx()
                        val playH = (size.height - strip).coerceAtLeast(1f)
                        for (c in e.changes) {
                            val x = c.position.x / size.width; val y = 1f - c.position.y / playH
                            when {
                                c.pressed && !c.previousPressed ->
                                    if (!AppState.pad.locked && c.position.y > size.height - strip) pad.grab(c.id.value, c.position.x / size.width)
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
      }
        panel?.let { SettingPanel(it, s, theme) { panel = null } }
    }
}

/** Height of the note strip along the bottom: drag it sideways to move the notes. */
private val STRIP = 64.dp

/** The settings rail at the left edge, Etherpad style: tap a setting to change it. */
@Composable
private fun Rail(s: PadSettings, t: PadTheme, sound: String, open: String?, pick: (String) -> Unit, onClose: () -> Unit) {
    Column(Modifier.fillMaxHeight().width(92.dp).background(t.rail)
               .then(if (t.flat) Modifier.border(BorderStroke(2.dp, Color.Black)) else Modifier)
               .verticalScroll(rememberScrollState()).padding(vertical = 4.dp),
           horizontalAlignment = Alignment.CenterHorizontally, verticalArrangement = Arrangement.spacedBy(6.dp)) {
        IconButton(onClick = onClose) { Icon(Icons.Filled.Close, "Close", tint = t.railDim) }
        val items = listOf(
            "Sound" to sound,
            "Size" to "${s.size}",
            "Octave" to (if (s.octave == 0) "0" else "%+d".format(s.octave)),
            "Key" to NOTE_NAMES[s.key],
            "Scale" to (if (s.scale == 0) "All notes" else SCALES[s.scale].name),
            "Up/down" to PadSettings.VERTICAL[s.vertical],
            "Glide" to if (s.snap) "Snap" else "Free",
            "Look" to if (Eink) "E-ink" else PAD_THEMES[s.theme].name,
        )
        for ((label, value) in items) RailItem(label, value, open == label, t) { pick(label) }
        // Not a panel: one tap locks the note strip so a stray touch cannot move the notes.
        RailItem(if (s.locked) "Locked" else "Lock", if (s.locked) "Strip plays" else "Strip moves", s.locked, t) {
            AppState.updatePad { it.copy(locked = !it.locked) }
        }
    }
}

@Composable
private fun RailItem(label: String, value: String, on: Boolean, t: PadTheme, click: () -> Unit) {
    Column(Modifier.clickable(onClick = click)
               .then(if (on && t.flat) Modifier.background(Color.Black) else Modifier)
               .padding(horizontal = 4.dp, vertical = 2.dp),
           horizontalAlignment = Alignment.CenterHorizontally) {
        Text(label, fontSize = 12.sp, fontWeight = FontWeight.SemiBold,
             color = when { on && t.flat -> Color.White; on -> t.parts[0]; else -> t.railText })
        Text(value, fontSize = 10.sp, color = if (on && t.flat) Color.White else t.railDim, maxLines = 1)
    }
}

@Composable
private fun SettingPanel(which: String, s: PadSettings, t: PadTheme, close: () -> Unit) {
    Surface(Modifier.padding(start = 100.dp, top = 12.dp, bottom = 12.dp).widthIn(max = 380.dp),
            shape = RoundedCornerShape(18.dp), color = if (t.flat) Color.White else Panel.copy(alpha = 0.96f), contentColor = Fg,
            border = if (t.flat) BorderStroke(Edge, Color.Black) else null) {
        Column(Modifier.padding(14.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(which, fontSize = 17.sp, fontWeight = FontWeight.SemiBold, modifier = Modifier.weight(1f))
                TextButton(onClick = close) { Text("Done") }
            }
            when (which) {
                "Sound" -> LazyColumn {
                    for ((group, list) in AppState.instruments.groupBy { it.group }) {
                        item { Text(group.uppercase(), color = TextDim, fontSize = 11.sp, modifier = Modifier.padding(top = 8.dp)) }
                        for (i in list) item {
                            val chosen = i.id == AppState.perf.part(0).instrument
                            Text(i.name, fontSize = 16.sp, color = if (chosen) PartColors[0] else Color.Unspecified,
                                 modifier = Modifier.clickable { AppState.updatePart(0) { it.copy(instrument = i.id) } }
                                     .fillMaxWidth().padding(vertical = 7.dp))
                        }
                    }
                }
                "Size" -> Stepper("${s.size} notes across") { d -> AppState.updatePad { it.copy(size = (it.size + d).coerceIn(3, 24)) } }
                "Octave" -> {
                    Stepper(if (s.octave == 0) "Middle of the instrument" else "%+d octaves".format(s.octave)) { d ->
                        AppState.updatePad { it.copy(octave = (it.octave + d).coerceIn(-4, 4)) } }
                    Text(if (s.locked) "The note strip is locked (Lock on the rail)."
                         else "Or drag the note strip along the bottom sideways.", color = TextDim, fontSize = 12.sp)
                    if (s.shift != 0) TextButton(onClick = { AppState.updatePad { it.copy(shift = 0) } }) {
                        Text("Move the notes back (%+d)".format(s.shift)) }
                }
                "Key" -> Chips(NOTE_NAMES, s.key) { k -> AppState.updatePad { it.copy(key = k) } }
                "Scale" -> Chips(SCALES.map { if (it.mask == 0) "All notes" else it.name }, s.scale) { k ->
                    AppState.updatePad { it.copy(scale = k) } }
                "Up/down" -> {
                    Chips(PadSettings.VERTICAL, s.vertical) { k -> AppState.updatePad { it.copy(vertical = k) } }
                    Text("Higher up the screen is louder and brighter.", color = TextDim, fontSize = 12.sp)
                }
                "Look" -> {
                    if (Eink) Text("E-ink is on (Setup tab), so the pad is black on white with no motion.",
                                   color = TextDim, fontSize = 12.sp)
                    else Chips(PAD_THEMES.map { it.name }, s.theme) { k -> AppState.updatePad { it.copy(theme = k) } }
                    Text("Lines", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
                    Chips(PadSettings.LINES, s.lines) { k -> AppState.updatePad { it.copy(lines = k) } }
                }
                "Glide" -> {
                    Chips(listOf("Snap", "Free"), if (s.snap) 0 else 1) { k -> AppState.updatePad { it.copy(snap = k == 0) } }
                    Text(if (s.snap) "A touch starts on the column's note, and a resting finger settles back onto it. Slide to glide."
                         else "Pitch follows the finger exactly, like a fretless string.", color = TextDim, fontSize = 12.sp)
                }
            }
        }
    }
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
 * collides), a note from its column, and pitch bend for everything after: the glide, and in
 * Snap mode the pull back onto the note. A finger keeps the columns it went down on, so
 * dragging the note strip while holding a note does not bend it.
 */
private class Pad {
    class Finger(val channel: Int, val note: Int, val cols: IntArray, var x: Float, var y: Float, var raw: Float) {
        var offset = 0f            // pitch = raw + offset; Snap sets it so the touch starts on the note
        var speed = 0f             // decaying peak, semitones per second
        var bend = -1; var pressure = -1; var slide = -1
        val trail = ArrayDeque<Offset>()
        var age = 0f
        val pitch get() = raw + offset
    }
    private class Ripple(val x: Float, val y: Float, val color: Color) { var age = 0f }
    /** A finger dragging the note strip: where it started, and the shift then. */
    private class Grab(val x: Float, val shift: Int)

    private val fingers = HashMap<Long, Finger>()
    private val grabs = HashMap<Long, Grab>()
    private val ripples = ArrayList<Ripple>()
    private var columns = IntArray(8) { 60 + it }      // sounding note of each column
    private var settings = PadSettings()
    var theme = PAD_THEMES[0]; private set
    private var shiftRange = 0..0
    private val ext = FloatArray(16 * 5)
    private val lastExt = FloatArray(16 * 5)
    private var nExt = 0
    private var dirty = true

    fun takeDirty() = dirty.also { dirty = false }

    fun layout(s: PadSettings, center: Float, t: PadTheme) {
        if (s != settings || t !== theme) dirty = true
        settings = s; theme = t
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

    /** x (0..1 across) to sounding pitch: straight lines between the column notes. */
    private fun pitchAt(cols: IntArray, x: Float): Float {
        val u = x * cols.size - 0.5f
        val i = floor(u).toInt().coerceIn(0, cols.size - 2)
        return cols[i] + (u - i) * (cols[i + 1] - cols[i])
    }
    private fun columnAt(cols: IntArray, x: Float) = (x * cols.size - 0.5f).roundToInt().coerceIn(0, cols.lastIndex)
    /** Sounding pitch back to x, for drawing (and for the Seaboard's fingers). */
    private fun xOf(pitch: Float): Float {
        var i = 0
        while (i < columns.size - 2 && pitch > columns[i + 1]) i++
        val span = (columns[i + 1] - columns[i]).coerceAtLeast(1)
        return (i + (pitch - columns[i]) / span + 0.5f) / columns.size
    }

    private fun send(status: Int, d1: Int, d2: Int) = Native.nativeTouch(status, d1, d2)

    fun down(id: Long, x: Float, y: Float) {
        dirty = true
        val used = fingers.values.map { it.channel }.toSet()
        val ch = (15 downTo 1).firstOrNull { it !in used } ?: return
        val cols = columns
        val col = columnAt(cols, x)
        // The engine adds the app's transpose (set for the Seaboard): take it off so the pad plays what it shows.
        val note = (cols[col] - AppState.transpose).coerceIn(0, 127)
        val y0 = y.coerceIn(0f, 1f)
        val f = Finger(ch, note, cols, x, y0, pitchAt(cols, x))
        if (settings.snap) f.offset = cols[col] - f.raw
        fingers[id] = f
        sendExpression(f)
        send(0x90 or ch, note, (40 + 80 * y0).roundToInt().coerceIn(1, 127))
        if (!theme.flat) ripples += Ripple(x, y0, colorOf(note))
    }

    /** A touch on the note strip: dragging it moves the notes, one column per column width. */
    fun grab(id: Long, x: Float) { grabs[id] = Grab(x, settings.shift); dirty = true }

    fun move(id: Long, x: Float, y: Float) {
        grabs[id]?.let { g ->
            val shift = (g.shift - ((x - g.x) * columns.size).roundToInt())
                .coerceIn(shiftRange.first.coerceAtMost(g.shift), shiftRange.last.coerceAtLeast(g.shift))
            if (shift != settings.shift) AppState.updatePad { it.copy(shift = shift) }
            return
        }
        val f = fingers[id] ?: return
        val raw = pitchAt(f.cols, x)
        f.speed = maxOf(f.speed, abs(raw - f.raw) * 60f)    // per event, events come about once a frame
        f.raw = raw; f.x = x; f.y = y.coerceIn(0f, 1f)
        sendExpression(f)
        dirty = true
    }

    fun up(id: Long) {
        if (grabs.remove(id) != null) { dirty = true; return }
        val f = fingers.remove(id) ?: return
        send(0x80 or f.channel, f.note, 64)
        dirty = true
    }

    fun releaseAll() { grabs.clear(); for (id in fingers.keys.toList()) up(id) }

    fun tick(dt: Float) {
        for (f in fingers.values) {
            f.age += dt
            f.speed *= exp(-dt / 0.06f)
            // Resting (not gliding, not wobbling): pull onto the note of the column under the finger.
            if (settings.snap && f.speed < 1.5f && dt > 0) {
                val target = f.cols[columnAt(f.cols, f.x)] - f.raw
                f.offset += (target - f.offset) * (1 - exp(-dt / 0.1f))
            }
            sendExpression(f)
            if (!theme.flat) {
                f.trail.addLast(Offset(xOf(f.pitch), f.y))
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

    private fun sendExpression(f: Finger) {
        val range = AppState.bendRange.coerceAtLeast(1f)
        val semis = f.pitch - (f.note + AppState.transpose)
        val bend = (8192 + semis / range * 8192).roundToInt().coerceIn(0, 16383)
        val (p, sl) = when (settings.vertical) {
            1 -> 0.7f to f.y
            2 -> (0.15f + 0.85f * f.y) to 0.5f
            else -> (0.25f + 0.75f * f.y) to f.y
        }
        val pressure = (p * 127).roundToInt().coerceIn(0, 127)
        val slide = (sl * 127).roundToInt().coerceIn(0, 127)
        if (bend != f.bend) { f.bend = bend; send(0xE0 or f.channel, bend and 0x7F, bend shr 7) }
        if (slide != f.slide) { f.slide = slide; send(0xB0 or f.channel, 74, slide) }
        if (pressure != f.pressure) { f.pressure = pressure; send(0xD0 or f.channel, pressure, 0) }
    }

    /** Part colour of a note as sent: the engine splits on that. */
    private fun colorOf(sentNote: Int): Color {
        val p = AppState.perf
        return theme.parts[if (p.layout == 1 && sentNote >= p.splitNote) 1 else 0]
    }

    private fun isRoot(n: Int) = ((n - settings.key) % 12 + 12) % 12 == 0

    fun draw(d: androidx.compose.ui.graphics.drawscope.DrawScope, strip: Float) = with(d) {
        val t = theme
        val w = size.width; val h = size.height
        val playH = h - strip
        if (t.flat) drawRect(t.top) else drawRect(Brush.verticalGradient(listOf(t.top, t.mid, t.bottom)))
        val cw = w / columns.size
        // Soft, Clear, Bold: (line alpha, root line alpha, line width, root width, band alpha)
        val (la, ra, lw, rw, ba) = when (settings.lines) {
            0 -> listOf(0.22f, 0.5f, 2f, 3f, 0f)
            2 -> listOf(0.75f, 1f, 4f, 7f, 0.10f)
            else -> listOf(0.45f, 0.85f, 3f, 5f, 0.05f)
        }
        val held = fingers.values.map { columnAt(columns, xOf(it.pitch)) }.toSet()
        for ((i, n) in columns.withIndex()) {
            val x = (i + 0.5f) * cw
            val c = colorOf(n - AppState.transpose)
            // Every other column shaded, so it is easy to see which one a finger is in.
            if (i % 2 == 1 && ba > 0f) drawRect(t.line.copy(alpha = ba), Offset(i * cw, 0f), Size(cw, playH))
            if (i in held) {
                if (t.flat) drawRect(Color(0xFFAAAAAA), Offset(i * cw, 0f), Size(cw, playH))
                else drawRect(Brush.horizontalGradient(listOf(c.copy(alpha = 0f), c.copy(alpha = 0.2f), c.copy(alpha = 0f)),
                                                       startX = x - cw / 2, endX = x + cw / 2),
                              topLeft = Offset(x - cw / 2, 0f), size = Size(cw, playH))
            }
            val root = isRoot(n)
            val lineColor = if (t.flat) Color.Black else if (root) c else t.line
            drawLine(lineColor.copy(alpha = if (t.flat) 1f else if (root) ra else la), Offset(x, 0f), Offset(x, playH),
                     strokeWidth = if (t.flat) (if (root) rw + 2f else lw) else if (root) rw else lw)
        }
        drawStrip(this, strip, cw, held)
        for (r in ripples) {
            val k = r.age / 0.7f
            drawCircle(r.color.copy(alpha = 0.5f * (1 - k)), radius = 40f + 220f * k, center = Offset(r.x * w, (1 - r.y) * playH),
                       style = Stroke(width = 4f * (1 - k) + 1f))
        }
        val mine = fingers.values.map { it.channel }.toSet()
        for (i in 0 until nExt) {                           // a controller's fingers (ours are drawn below)
            val b = 5 * i
            if (ext[b].toInt() in mine) continue
            orb(Offset(xOf(ext[b + 2]) * w, (1 - ext[b + 4]) * playH), ext[b + 3], colorOf(ext[b + 1].toInt()))
        }
        for (f in fingers.values) {
            val c = colorOf(f.note)
            for ((k, p) in f.trail.withIndex()) {
                val a = (k + 1f) / f.trail.size
                drawCircle(c.copy(alpha = 0.22f * a), radius = 8f + 26f * a, center = Offset(p.x * w, (1 - p.y) * playH))
            }
            orb(Offset(xOf(f.pitch) * w, (1 - f.y) * playH), f.pressure / 127f, c)
        }
    }

    /** The note names along the bottom. Unlocked it is a handle: drag sideways to move the notes. */
    private fun drawStrip(d: androidx.compose.ui.graphics.drawscope.DrawScope, strip: Float, cw: Float, held: Set<Int>) = with(d) {
        val t = theme
        val w = size.width; val h = size.height; val top = h - strip
        val dragging = grabs.isNotEmpty()
        drawRect(if (dragging && !t.flat) t.parts[0].copy(alpha = 0.25f) else t.strip, Offset(0f, top), Size(w, strip))
        drawLine(t.line.copy(alpha = if (t.flat) 1f else 0.5f), Offset(0f, top), Offset(w, top), strokeWidth = if (t.flat) 3f else 2f)
        val paint = android.graphics.Paint().apply {
            isAntiAlias = true; textAlign = android.graphics.Paint.Align.CENTER
            textSize = (strip * 0.36f).coerceAtMost(cw * 0.42f)
        }
        val canvas = drawContext.canvas.nativeCanvas
        for ((i, n) in columns.withIndex()) {
            val x = (i + 0.5f) * cw
            val root = isRoot(n)
            if (i in held && t.flat) drawRect(Color.Black, Offset(i * cw, top), Size(cw, strip))
            if (i > 0) drawLine(t.label.copy(alpha = if (t.flat) 0.6f else 0.18f), Offset(i * cw, top + strip * 0.2f),
                                Offset(i * cw, h - strip * 0.2f), strokeWidth = 2f)
            paint.color = when {
                i in held && t.flat -> android.graphics.Color.WHITE
                t.flat -> android.graphics.Color.BLACK
                root -> colorOf(n - AppState.transpose).toArgb()
                else -> t.label.copy(alpha = 0.85f).toArgb()
            }
            paint.isFakeBoldText = root
            canvas.drawText(noteName(n), x, top + strip * 0.62f, paint)
        }
        // Grip marks at both ends while the strip can be dragged.
        if (!settings.locked) {
            val g = t.label.copy(alpha = if (t.flat) 1f else 0.45f)
            for (k in 0..2) {
                val y = top + strip * (0.38f + 0.12f * k)
                drawLine(g, Offset(8f, y), Offset(26f, y), strokeWidth = 2f)
                drawLine(g, Offset(w - 26f, y), Offset(w - 8f, y), strokeWidth = 2f)
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
