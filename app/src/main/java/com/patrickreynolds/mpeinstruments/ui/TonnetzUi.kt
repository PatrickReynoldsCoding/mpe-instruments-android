package com.patrickreynolds.mpeinstruments.ui

import android.os.Handler
import android.os.Looper
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
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.NOTE_NAMES
import com.patrickreynolds.mpeinstruments.Native
import com.patrickreynolds.mpeinstruments.controllers.Option
import com.patrickreynolds.mpeinstruments.controllers.Tonnetz
import kotlin.math.floor
import kotlin.math.roundToInt
import kotlin.math.sqrt

/** The Tonnetz's options on the rail. */
@Composable
fun tonnetzOptions(): List<Option> {
    val s = Tonnetz.settings
    val accent = if (Eink) Color.Black else PartColors[0]
    fun set(change: (Tonnetz.Settings) -> Tonnetz.Settings) = Tonnetz.update(change)
    return listOf(
        Option("Key", NOTE_NAMES[s.key], perPart = false) {
            Chips(NOTE_NAMES, s.key, accent) { k -> set { it.copy(key = k) } }
            Text("The key's note sits in the middle of the net, and its chords are lit.", color = TextDim, fontSize = 12.sp)
            SwitchRow("Light the key's chords", s.litKey, accent = accent) { v -> set { it.copy(litKey = v) } }
        },
        Option("Octave", if (s.octave == 0) "0" else "%+d".format(s.octave), perPart = false) {
            Chips(listOf("−3", "−2", "−1", "0", "+1", "+2", "+3"), s.octave + 3, accent) { k -> set { it.copy(octave = k - 3) } }
            Text("Every note is played within an octave of the middle of the instrument, moved by this.", color = TextDim, fontSize = 12.sp)
        },
        Option("Size", "${s.size}", perPart = false) {
            Chips(Tonnetz.SIZES.map { "$it" }, s.size - Tonnetz.SIZES.first, accent) { k -> set { it.copy(size = Tonnetz.SIZES.first + k) } }
            Text("Nodes across the screen: fewer are bigger to hit.", color = TextDim, fontSize = 12.sp)
        },
        Option("Chords", (if (s.chords == 1) "7ths" else "Triads") + if (s.bass) " + bass" else "", perPart = false) {
            Chips(listOf("Triads", "Sevenths"), s.chords, accent) { k -> set { it.copy(chords = k) } }
            SwitchRow("Bass", s.bass, "The chord's root an octave below too", accent = accent) { v -> set { it.copy(bass = v) } }
            Text("Strum", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
            Chips(Tonnetz.STRUMS.map { it.first }, s.strum, accent) { k -> set { it.copy(strum = k) } }
            Text("A triangle pointing up is a major chord, down a minor one. Slide to the next: the notes they share keep " +
                 "sounding and only the one that changes moves, the smoothest change there is.", color = TextDim, fontSize = 12.sp)
        },
        Option("Arp", if (s.arp.on) com.patrickreynolds.mpeinstruments.ARP_ORDERS[s.arp.order] else "Off", perPart = false) {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                ArpControls(s.arp, accent) { c -> set { it.copy(arp = c(it.arp)) } }
                TempoControls(accent)
            }
        },
        Option("Labels", listOf("Chords", "Notes", "None")[s.labels], perPart = false) {
            Chips(listOf("Notes and chords", "Notes", "None"), s.labels, accent) { k -> set { it.copy(labels = k) } }
            SwitchRow("Trails", s.trails, "The path each finger takes across the net", accent = accent) { v -> set { it.copy(trails = v) } }
        },
    )
}

/** The Tonnetz's play surface. */
@Composable
fun TonnetzSurface() {
    val net = remember { Net() }
    var frame by remember { mutableIntStateOf(0) }
    DisposableEffect(Unit) { onDispose { net.releaseAll() } }
    LaunchedEffect(Unit) {
        var last = 0L
        while (true) {
            val now = withFrameNanos { it }
            net.tick(if (last == 0L) 0f else (now - last) / 1e9f)
            last = now
            if (!Eink || net.takeDirty()) frame++
        }
    }
    val theme = hostTheme()
    net.settings = Tonnetz.settings
    net.center = (AppState.instrument(0)?.centerNote ?: 60f) + 12f * Tonnetz.settings.octave
    Canvas(Modifier.fillMaxSize().clipToBounds().systemGestureExclusion()
        .pointerInput(Unit) {
            awaitPointerEventScope {
                while (true) {
                    val e = awaitPointerEvent()
                    net.size(size.width.toFloat(), size.height.toFloat(), density)
                    for (c in e.changes) {
                        when {
                            c.pressed && !c.previousPressed -> net.down(c.id.value, c.position.x, c.position.y)
                            c.pressed -> net.move(c.id.value, c.position.x, c.position.y)
                            c.previousPressed -> net.up(c.id.value)
                        }
                        c.consume()
                    }
                }
            }
        }) {
        frame.let { }
        net.size(size.width, size.height, density)
        net.draw(this, theme)
    }
}

/**
 * The net and the fingers on it. Node (i, j) is the note key + 7i + 4j: i to the right in
 * fifths, j up and to the right in major thirds. The cell (i, j) holds two triangles: pointing
 * up, (i, j) (i+1, j) (i, j+1), the major chord on node (i, j); pointing down, (i+1, j) (i, j+1)
 * (i+1, j+1), the minor chord on node (i, j+1).
 */
private class Net {
    var settings = Tonnetz.Settings()
    var center = 60f
    private var w = 1f; private var h = 1f; private var density = 1f
    private var dx = 100f; private var dy = 86.6f

    fun size(width: Float, height: Float, d: Float) {
        w = width; h = height; density = d
        dx = w / settings.size.coerceAtLeast(2); dy = dx * sqrt(3f) / 2
    }

    // ---- the lattice ---------------------------------------------------------------------------
    private fun pc(i: Int, j: Int) = (((settings.key + 7 * i + 4 * j) % 12) + 12) % 12
    private fun nodeX(i: Int, j: Int) = w / 2 + (i + j / 2f) * dx
    private fun nodeY(j: Int) = h / 2 - j * dy

    /** What a touch at (x, y) is on: a node, within a third of the spacing of it; else a triangle. */
    sealed class Target {
        data class Node(val i: Int, val j: Int) : Target()
        data class Tri(val i: Int, val j: Int, val up: Boolean) : Target()
    }

    private fun hit(x: Float, y: Float): Target {
        val fj = (h / 2 - y) / dy
        val fi = (x - w / 2) / dx - fj / 2
        val ci = floor(fi).toInt(); val cj = floor(fj).toInt()
        val up = (fi - ci) + (fj - cj) < 1f
        val corners = if (up) listOf(ci to cj, ci + 1 to cj, ci to cj + 1) else listOf(ci + 1 to cj, ci to cj + 1, ci + 1 to cj + 1)
        val near = corners.minBy { (i, j) -> dist2(x, y, nodeX(i, j), nodeY(j)) }
        return if (sqrt(dist2(x, y, nodeX(near.first, near.second), nodeY(near.second))) < dx * 0.27f) Target.Node(near.first, near.second)
               else Target.Tri(ci, cj, up)
    }
    private fun dist2(ax: Float, ay: Float, bx: Float, by: Float) = (ax - bx) * (ax - bx) + (ay - by) * (ay - by)

    /** Where a target is drawn: a node, or a triangle's middle. */
    private fun centre(t: Target): Offset = when (t) {
        is Target.Node -> Offset(nodeX(t.i, t.j), nodeY(t.j))
        is Target.Tri -> if (t.up) Offset((nodeX(t.i, t.j) + nodeX(t.i + 1, t.j) + nodeX(t.i, t.j + 1)) / 3, (2 * nodeY(t.j) + nodeY(t.j + 1)) / 3)
                         else Offset((nodeX(t.i + 1, t.j) + nodeX(t.i, t.j + 1) + nodeX(t.i + 1, t.j + 1)) / 3, (nodeY(t.j) + 2 * nodeY(t.j + 1)) / 3)
    }

    /** A triangle's root (pitch class) and whether it is major. */
    private fun chordOf(t: Target.Tri) = if (t.up) pc(t.i, t.j) to true else pc(t.i, t.j + 1) to false

    private fun chordName(t: Target.Tri): String {
        val (r, major) = chordOf(t)
        return NOTE_NAMES[r] + when { settings.chords == 1 && major -> "maj7"; settings.chords == 1 -> "m7"; major -> ""; else -> "m" }
    }

    /** A pitch class played within the octave around the middle. */
    private fun fold(pc: Int): Int {
        val low = center.roundToInt() - 6
        return low + ((pc - low) % 12 + 12) % 12
    }

    /** The notes a target plays: a node its own; a chord root, third, fifth (and seventh, and bass). */
    private fun notes(t: Target): List<Int> = when (t) {
        is Target.Node -> listOf(fold(pc(t.i, t.j)))
        is Target.Tri -> {
            val (r, major) = chordOf(t)
            val root = fold(r)
            val list = mutableListOf(root, root + if (major) 4 else 3, root + 7)
            if (settings.chords == 1) list += root + if (major) 11 else 10
            if (settings.bass) list += root - 12
            list.map { it.coerceIn(0, 127) }
        }
    }

    // ---- fingers and voices --------------------------------------------------------------------
    private class Voice(val channel: Int, val note: Int, var started: Boolean = false)
    private inner class Touch(var target: Target, var x: Float, var y: Float) {
        val voices = mutableListOf<Voice>()
        val trail = ArrayDeque<Offset>()
        var ax = x; var ay = y                 // where it came onto its target, for its shaping
    }

    private val touches = HashMap<Long, Touch>()
    private val handler = Handler(Looper.getMainLooper())
    private var dirty = true
    private var time = 0f
    /** Light that fades from a note just played (by pitch class), for the glow on its nodes. */
    private val glow = FloatArray(12)

    fun takeDirty() = dirty.also { dirty = false }

    private fun send(status: Int, d1: Int, d2: Int) = Native.nativeTouch(status, d1, d2)
    private fun freeChannel(): Int? {
        val used = touches.values.flatMap { t -> t.voices.map { it.channel } }.toSet()
        return (15 downTo 1).firstOrNull { it !in used }
    }

    private val level = 0.8f
    private fun start(t: Touch, v: Voice) {
        if (v.started || !touches.containsValue(t) || v !in t.voices) return
        v.started = true
        send(0xB0 or v.channel, 7, 127)
        send(0x90 or v.channel, v.note, (level * 127 / 5).roundToInt().coerceIn(1, 127))
        shape(t, v)
        glow[v.note % 12] = 1f
        dirty = true
    }
    private fun stop(v: Voice) { if (v.started) send(0x80 or v.channel, v.note, 64); v.started = false }

    /**
     * Rocking the finger inside its node or triangle shapes the notes: up brighter and a little
     * louder, side to side (on a node) a vibrato.
     */
    private fun shape(t: Touch, v: Voice) {
        if (!v.started) return
        val up = ((t.ay - t.y) / (dx * 0.5f)).coerceIn(-1f, 1f)
        val pressure = ((level + 0.2f * up) * 127).roundToInt().coerceIn(1, 127)
        val slide = ((0.55f + 0.45f * up) * 127).roundToInt().coerceIn(0, 127)
        val semis = if (t.target is Target.Node) ((t.x - t.ax) / (dx * 0.25f)).coerceIn(-1f, 1f) * 0.35f else 0f
        val bend = (8192 + semis / 48f * 8192).roundToInt().coerceIn(0, 16383)
        send(0xD0 or v.channel, pressure, 0)
        send(0xB0 or v.channel, 74, slide)
        send(0xE0 or v.channel, bend and 0x7F, bend shr 7)
    }

    /** Sound [t]'s target: notes it already plays keep sounding; the rest stop, the new ones start (strummed, low first). */
    private fun voice(t: Touch) {
        val want = notes(t.target)
        val keep = t.voices.filter { it.note in want }
        for (v in t.voices - keep.toSet()) stop(v)
        t.voices.retainAll(keep.toSet())
        val delay = Tonnetz.STRUMS.getOrElse(settings.strum) { Tonnetz.STRUMS[0] }.second
        var k = 0
        for (n in want.sorted()) if (t.voices.none { it.note == n }) {
            val ch = freeChannel() ?: break
            val v = Voice(ch, n)
            t.voices += v
            val wait = delay * k++
            if (wait == 0L) start(t, v) else handler.postDelayed({ start(t, v) }, wait)
        }
        for (v in keep) shape(t, v)
    }

    fun down(id: Long, x: Float, y: Float) {
        val t = Touch(hit(x, y), x, y)
        touches[id] = t
        t.trail.addLast(centre(t.target))
        voice(t)
        dirty = true
    }

    fun move(id: Long, x: Float, y: Float) {
        val t = touches[id] ?: return
        t.x = x; t.y = y
        val now = hit(x, y)
        // Keep to a target until well into the next: a finger on a border does not flicker.
        if (now != t.target && !nearBorder(t.target, x, y)) {
            t.target = now; t.ax = x; t.ay = y
            if (settings.trails) { t.trail.addLast(centre(now)); while (t.trail.size > 24) t.trail.removeFirst() }
            voice(t)
        } else for (v in t.voices) shape(t, v)
        dirty = true
    }

    /** Still within a small margin of the old target (its centre closer than 60% of the spacing). */
    private fun nearBorder(old: Target, x: Float, y: Float): Boolean {
        val c = centre(old)
        val r = if (old is Target.Node) dx * 0.33f else dx * 0.36f
        return sqrt(dist2(x, y, c.x, c.y)) < r
    }

    fun up(id: Long) {
        val t = touches.remove(id) ?: return
        for (v in t.voices) stop(v)
        t.voices.clear()
        fading += Fade(t.trail.toList(), 1f)
        dirty = true
    }

    fun releaseAll() { for (id in touches.keys.toList()) up(id); handler.removeCallbacksAndMessages(null) }

    private class Fade(val points: List<Offset>, var life: Float)
    private val fading = mutableListOf<Fade>()

    fun tick(dt: Float) {
        time += dt
        for (k in 0 until 12) if (glow[k] > 0f) { glow[k] = (glow[k] - dt * 1.2f).coerceAtLeast(0f); dirty = true }
        fading.forEach { it.life -= dt * 0.8f }
        if (fading.removeAll { it.life <= 0f } || fading.isNotEmpty()) dirty = true
    }

    // ---- drawing -------------------------------------------------------------------------------
    private val SCALE_MAJOR = 0b101010110101

    private fun inKey(pc: Int) = (SCALE_MAJOR shr (((pc - settings.key) % 12 + 12) % 12)) and 1 == 1

    fun draw(d: DrawScope, t: PadTheme) = with(d) {
        val flat = t.flat
        if (flat) drawRect(Color.White) else drawRect(Brush.verticalGradient(listOf(t.top, t.mid, t.bottom)))
        val sounding = BooleanArray(12)
        for (tt in touches.values) for (v in tt.voices) if (v.started) sounding[v.note % 12] = true
        val activeTris = touches.values.mapNotNull { it.target as? Target.Tri }.toSet()
        val jr = (h / 2 / dy).toInt() + 2
        val ir = (w / dx).toInt() + jr + 2
        val accent = if (flat) Color.Black else t.parts[0]
        val minor = if (flat) Color.Black else t.parts[1]
        val text = android.graphics.Paint().apply { isAntiAlias = true; textAlign = android.graphics.Paint.Align.CENTER }
        // Triangles: the key's chords lit, the held ones bright, chord names in the middle.
        for (j in -jr..jr) for (i in -ir..ir) for (up in listOf(true, false)) {
            val tri = Target.Tri(i, j, up)
            val c = centre(tri)
            if (c.x < -dx || c.x > w + dx || c.y < -dy || c.y > h + dy) continue
            val pts = if (up) listOf(i to j, i + 1 to j, i to j + 1) else listOf(i + 1 to j, i to j + 1, i + 1 to j + 1)
            val path = Path().apply {
                moveTo(nodeX(pts[0].first, pts[0].second), nodeY(pts[0].second))
                for (p in pts.drop(1)) lineTo(nodeX(p.first, p.second), nodeY(p.second))
                close()
            }
            val col = if (up) accent else minor
            val held = tri in activeTris
            val lit = settings.litKey && pts.all { (a, b) -> inKey(pc(a, b)) }
            when {
                held -> drawPath(path, if (flat) Color.Black.copy(alpha = 0.35f) else col.copy(alpha = 0.45f))
                lit -> drawPath(path, if (flat) Color.Black.copy(alpha = 0.06f) else col.copy(alpha = 0.10f))
                !flat -> drawPath(path, col.copy(alpha = 0.03f))
            }
            if (settings.labels == 0 && dx > 70f) {
                text.textSize = dx * 0.17f
                text.color = (if (held) (if (flat) Color.Black else Color.White) else t.label.copy(alpha = if (lit) 0.75f else 0.35f)).toArgb()
                text.isFakeBoldText = held
                drawContext.canvas.nativeCanvas.drawText(chordName(tri), c.x, c.y + text.textSize * 0.35f, text)
            }
        }
        // The lines of the net: fifths across, thirds on the slants.
        val line = if (flat) Color.Black.copy(alpha = 0.5f) else t.line.copy(alpha = 0.16f)
        for (j in -jr..jr) for (i in -ir..ir) {
            val x = nodeX(i, j); val y = nodeY(j)
            if (x < -dx * 2 || x > w + dx * 2) continue
            drawLine(line, Offset(x, y), Offset(nodeX(i + 1, j), y), strokeWidth = 2f)
            drawLine(line, Offset(x, y), Offset(nodeX(i, j + 1), nodeY(j + 1)), strokeWidth = 2f)
            drawLine(line, Offset(x, y), Offset(nodeX(i - 1, j + 1), nodeY(j + 1)), strokeWidth = 2f)
        }
        // Trails: where each finger went, fading after it lifts.
        if (settings.trails) {
            fun trail(points: List<Offset>, alpha: Float) {
                for (k in 1 until points.size) {
                    val a = alpha * k / points.size
                    drawLine(if (flat) Color.Black.copy(alpha = a) else Color.White.copy(alpha = 0.6f * a), points[k - 1], points[k],
                             strokeWidth = 6f)
                }
            }
            for (tt in touches.values) trail(tt.trail.toList() + Offset(tt.x, tt.y), 1f)
            for (f in fading) trail(f.points, f.life)
        }
        // Nodes: the notes, glowing while they sound, anywhere on the net.
        val r = dx * 0.17f
        for (j in -jr..jr) for (i in -ir..ir) {
            val x = nodeX(i, j); val y = nodeY(j)
            if (x < -r || x > w + r || y < -r || y > h + r) continue
            val p = pc(i, j)
            val on = sounding[p]
            val g = glow[p]
            if (!flat && (on || g > 0f)) drawCircle(Brush.radialGradient(listOf(accent.copy(alpha = 0.6f * maxOf(g, if (on) 0.7f else 0f)), accent.copy(alpha = 0f)),
                                                                         center = Offset(x, y), radius = r * 3f), r * 3f, Offset(x, y))
            val held = touches.values.any { it.target == Target.Node(i, j) }
            drawCircle(when { flat && (on || held) -> Color.Black; flat -> Color.White; on || held -> accent; p == settings.key -> t.mid.copy(alpha = 1f)
                              else -> t.bottom.copy(alpha = 1f) }, r, Offset(x, y))
            drawCircle(if (flat) Color.Black else if (inKey(p)) t.line.copy(alpha = 0.55f) else t.line.copy(alpha = 0.2f), r, Offset(x, y),
                       style = Stroke(width = if (p == settings.key) 4f else 2f))
            if (settings.labels != 2) {
                text.textSize = r * 0.95f
                text.isFakeBoldText = p == settings.key
                text.color = (if (flat) (if (on || held) Color.White else Color.Black) else if (on || held) Color.Black else t.label.copy(alpha = if (inKey(p)) 0.95f else 0.5f)).toArgb()
                drawContext.canvas.nativeCanvas.drawText(NOTE_NAMES[p], x, y + text.textSize * 0.35f, text)
            }
        }
        // Each finger: a ring where it is.
        for (tt in touches.values) drawCircle(if (flat) Color.Black else Color.White.copy(alpha = 0.85f), dx * 0.08f, Offset(tt.x, tt.y),
                                              style = Stroke(width = 4f))
    }

}
