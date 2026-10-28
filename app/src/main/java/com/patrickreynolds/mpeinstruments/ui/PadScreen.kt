package com.patrickreynolds.mpeinstruments.ui

import androidx.compose.foundation.Canvas
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
import androidx.compose.ui.graphics.Color
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
    pad.layout(s, inst?.centerNote ?: 60f)
    DisposableEffect(Unit) { onDispose { pad.releaseAll() } }
    LaunchedEffect(Unit) {
        var last = 0L
        while (true) {
            val now = withFrameNanos { it }
            pad.tick(if (last == 0L) 0f else (now - last) / 1e9f)
            last = now
            frame++
        }
    }
    Box(Modifier.fillMaxSize().background(Ink)) {
      Row(Modifier.fillMaxSize()) {
        Rail(s, inst?.name ?: "", panel, { panel = if (panel == it) null else it }, onClose)
        Canvas(Modifier.weight(1f).fillMaxHeight().systemGestureExclusion()
            .pointerInput(Unit) {
                awaitPointerEventScope {
                    while (true) {
                        val e = awaitPointerEvent()
                        for (c in e.changes) {
                            val x = c.position.x / size.width; val y = 1f - c.position.y / size.height
                            when {
                                c.pressed && !c.previousPressed -> pad.down(c.id.value, x, y)
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
            pad.draw(this)
        }
      }
        panel?.let { SettingPanel(it, s) { panel = null } }
    }
}

/** The settings rail at the left edge, Etherpad style: tap a setting to change it. */
@Composable
private fun Rail(s: PadSettings, sound: String, open: String?, pick: (String) -> Unit, onClose: () -> Unit) {
    Column(Modifier.fillMaxHeight().width(92.dp).background(Color.Black.copy(alpha = 0.35f))
               .verticalScroll(rememberScrollState()).padding(vertical = 4.dp),
           horizontalAlignment = Alignment.CenterHorizontally, verticalArrangement = Arrangement.spacedBy(6.dp)) {
        IconButton(onClick = onClose) { Icon(Icons.Filled.Close, "Close", tint = TextDim) }
        val items = listOf(
            "Sound" to sound,
            "Size" to "${s.size}",
            "Octave" to (if (s.octave == 0) "0" else "%+d".format(s.octave)),
            "Key" to NOTE_NAMES[s.key],
            "Scale" to (if (s.scale == 0) "All notes" else SCALES[s.scale].name),
            "Up/down" to PadSettings.VERTICAL[s.vertical],
            "Glide" to if (s.snap) "Snap" else "Free",
        )
        for ((label, value) in items) Column(
            Modifier.clickable { pick(label) }.padding(horizontal = 4.dp, vertical = 2.dp),
            horizontalAlignment = Alignment.CenterHorizontally) {
            Text(label, fontSize = 12.sp, color = if (open == label) PartColors[0] else Color.White.copy(alpha = 0.85f),
                 fontWeight = FontWeight.SemiBold)
            Text(value, fontSize = 10.sp, color = TextDim, maxLines = 1)
        }
    }
}

@Composable
private fun SettingPanel(which: String, s: PadSettings, close: () -> Unit) {
    Surface(Modifier.padding(start = 100.dp, top = 12.dp, bottom = 12.dp).widthIn(max = 380.dp),
            shape = RoundedCornerShape(18.dp), color = Panel.copy(alpha = 0.96f)) {
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
                "Octave" -> Stepper(if (s.octave == 0) "Middle of the instrument" else "%+d octaves".format(s.octave)) { d ->
                    AppState.updatePad { it.copy(octave = (it.octave + d).coerceIn(-4, 4)) } }
                "Key" -> Chips(NOTE_NAMES, s.key) { k -> AppState.updatePad { it.copy(key = k) } }
                "Scale" -> Chips(SCALES.map { if (it.mask == 0) "All notes" else it.name }, s.scale) { k ->
                    AppState.updatePad { it.copy(scale = k) } }
                "Up/down" -> {
                    Chips(PadSettings.VERTICAL, s.vertical) { k -> AppState.updatePad { it.copy(vertical = k) } }
                    Text("Higher up the screen is louder and brighter.", color = TextDim, fontSize = 12.sp)
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

/**
 * Touches to MPE. Each finger gets a channel (16 down to 2, so a Seaboard on 2 up rarely
 * collides), a note from its column, and pitch bend for everything after: the glide, and in
 * Snap mode the pull back onto the note.
 */
private class Pad {
    class Finger(val channel: Int, val note: Int, var x: Float, var y: Float, var raw: Float) {
        var offset = 0f            // pitch = raw + offset; Snap sets it so the touch starts on the note
        var speed = 0f             // decaying peak, semitones per second
        var bend = -1; var pressure = -1; var slide = -1
        val trail = ArrayDeque<Offset>()
        var age = 0f
        val pitch get() = raw + offset
    }
    private class Ripple(val x: Float, val y: Float, val color: Color) { var age = 0f }

    private val fingers = HashMap<Long, Finger>()
    private val ripples = ArrayList<Ripple>()
    private var columns = IntArray(8) { 60 + it }      // sounding note of each column
    private var settings = PadSettings()
    private val ext = FloatArray(16 * 5)
    private var nExt = 0

    fun layout(s: PadSettings, center: Float) {
        settings = s
        val mask = SCALES[s.scale].mask.takeIf { it != 0 } ?: 0xFFF
        val inScale = (0..127).filter { (mask shr (((it - s.key) % 12 + 12) % 12)) and 1 == 1 }
        val want = center + 12 * s.octave
        val k = inScale.indices.minByOrNull { abs(inScale[it] - want) } ?: 0
        val start = (k - s.size / 2).coerceIn(0, (inScale.size - s.size).coerceAtLeast(0))
        columns = IntArray(s.size) { inScale.getOrElse(start + it) { 127 } }
    }

    /** x (0..1 across) to sounding pitch: straight lines between the column notes. */
    private fun pitchAt(x: Float): Float {
        val u = x * columns.size - 0.5f
        val i = floor(u).toInt().coerceIn(0, columns.size - 2)
        return columns[i] + (u - i) * (columns[i + 1] - columns[i])
    }
    private fun columnAt(x: Float) = (x * columns.size - 0.5f).roundToInt().coerceIn(0, columns.lastIndex)
    /** Sounding pitch back to x, for drawing (and for the Seaboard's fingers). */
    private fun xOf(pitch: Float): Float {
        var i = 0
        while (i < columns.size - 2 && pitch > columns[i + 1]) i++
        val span = (columns[i + 1] - columns[i]).coerceAtLeast(1)
        return (i + (pitch - columns[i]) / span + 0.5f) / columns.size
    }

    private fun send(status: Int, d1: Int, d2: Int) = Native.nativeTouch(status, d1, d2)

    fun down(id: Long, x: Float, y: Float) {
        val used = fingers.values.map { it.channel }.toSet()
        val ch = (15 downTo 1).firstOrNull { it !in used } ?: return
        val col = columnAt(x)
        // The engine adds the app's transpose (set for the Seaboard): take it off so the pad plays what it shows.
        val note = (columns[col] - AppState.transpose).coerceIn(0, 127)
        val f = Finger(ch, note, x, y, pitchAt(x))
        if (settings.snap) f.offset = columns[col] - f.raw
        fingers[id] = f
        sendExpression(f)
        send(0x90 or ch, note, (40 + 80 * y).roundToInt().coerceIn(1, 127))
        ripples += Ripple(x, y, colorOf(note))
    }

    fun move(id: Long, x: Float, y: Float) {
        val f = fingers[id] ?: return
        val raw = pitchAt(x)
        f.speed = maxOf(f.speed, abs(raw - f.raw) * 60f)    // per event, events come about once a frame
        f.raw = raw; f.x = x; f.y = y.coerceIn(0f, 1f)
        sendExpression(f)
    }

    fun up(id: Long) {
        val f = fingers.remove(id) ?: return
        send(0x80 or f.channel, f.note, 64)
    }

    fun releaseAll() { for (id in fingers.keys.toList()) up(id) }

    fun tick(dt: Float) {
        for (f in fingers.values) {
            f.age += dt
            f.speed *= exp(-dt / 0.06f)
            // Resting (not gliding, not wobbling): pull onto the note of the column under the finger.
            if (settings.snap && f.speed < 1.5f && dt > 0) {
                val target = columns[columnAt(f.x)] - f.raw
                f.offset += (target - f.offset) * (1 - exp(-dt / 0.1f))
            }
            sendExpression(f)
            f.trail.addLast(Offset(xOf(f.pitch), f.y))
            if (f.trail.size > 28) f.trail.removeFirst()
        }
        ripples.forEach { it.age += dt }
        ripples.removeAll { it.age > 0.7f }
        nExt = Native.nativeFingers(ext)
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
        return PartColors[if (p.layout == 1 && sentNote >= p.splitNote) 1 else 0]
    }

    fun draw(d: androidx.compose.ui.graphics.drawscope.DrawScope) = with(d) {
        val w = size.width; val h = size.height
        drawRect(Brush.verticalGradient(listOf(Color(0xFF1A1626), Color(0xFF0E1014), Color(0xFF0B1418))))
        val cw = w / columns.size
        val held = fingers.values.map { columnAt(it.x) }.toSet()
        val paint = android.graphics.Paint().apply { textSize = 30f; isAntiAlias = true; textAlign = android.graphics.Paint.Align.CENTER }
        for ((i, n) in columns.withIndex()) {
            val x = (i + 0.5f) * cw
            val c = colorOf(n - AppState.transpose)
            if (i in held) drawRect(Brush.horizontalGradient(listOf(c.copy(alpha = 0f), c.copy(alpha = 0.16f), c.copy(alpha = 0f)),
                                                            startX = x - cw / 2, endX = x + cw / 2),
                                    topLeft = Offset(x - cw / 2, 0f), size = androidx.compose.ui.geometry.Size(cw, h))
            val root = ((n - settings.key) % 12 + 12) % 12 == 0
            drawLine(c.copy(alpha = if (root) 0.55f else 0.22f), Offset(x, 0f), Offset(x, h), strokeWidth = if (root) 3f else 2f)
            paint.color = (if (root) 0xCCFFFFFF else 0x77FFFFFF).toInt()
            drawContext.canvas.nativeCanvas.drawText(noteName(n), x, h - 18f, paint)
        }
        for (r in ripples) {
            val t = r.age / 0.7f
            drawCircle(r.color.copy(alpha = 0.5f * (1 - t)), radius = 40f + 220f * t, center = Offset(r.x * w, (1 - r.y) * h),
                       style = Stroke(width = 4f * (1 - t) + 1f))
        }
        val mine = fingers.values.map { it.channel }.toSet()
        for (i in 0 until nExt) {                           // a controller's fingers (ours are drawn below)
            val b = 5 * i
            if (ext[b].toInt() in mine) continue
            orb(Offset(xOf(ext[b + 2]) * w, (1 - ext[b + 4]) * h), ext[b + 3], colorOf(ext[b + 1].toInt()))
        }
        for (f in fingers.values) {
            val c = colorOf(f.note)
            for ((k, p) in f.trail.withIndex()) {
                val a = (k + 1f) / f.trail.size
                drawCircle(c.copy(alpha = 0.22f * a), radius = 8f + 26f * a, center = Offset(p.x * w, (1 - p.y) * h))
            }
            orb(Offset(xOf(f.pitch) * w, (1 - f.y) * h), f.pressure / 127f, c)
        }
    }

    private fun androidx.compose.ui.graphics.drawscope.DrawScope.orb(at: Offset, pressure: Float, c: Color) {
        val r = 60f + 70f * pressure
        drawCircle(Brush.radialGradient(listOf(c.copy(alpha = 0.55f), c.copy(alpha = 0f)), center = at, radius = r * 1.8f),
                   radius = r * 1.8f, center = at)
        drawCircle(c.copy(alpha = 0.8f), radius = r, center = at, style = Stroke(width = 3f))
        drawCircle(Ink.copy(alpha = 0.55f), radius = r * 0.62f, center = at)
        drawCircle(Color.White.copy(alpha = 0.9f), radius = r * 0.24f, center = at)
        drawCircle(c, radius = r * 0.17f, center = at)
    }
}
