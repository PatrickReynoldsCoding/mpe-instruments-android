package com.patrickreynolds.mpeinstruments.ui

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.geometry.Size
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
import com.patrickreynolds.mpeinstruments.Chords
import com.patrickreynolds.mpeinstruments.controllers.TouchPad
import com.patrickreynolds.mpeinstruments.PadSettings
import kotlin.math.PI
import kotlin.math.atan2
import kotlin.math.cos
import kotlin.math.floor
import kotlin.math.hypot
import kotlin.math.sin

/** The chord picker of pad [side]: the wheel or the grid. What they hold is changed in the Scale panel. */
@Composable
fun ChordPicker(side: Int, s: PadSettings, t: PadTheme, modifier: Modifier) {
    Box(modifier.background(if (t.flat) Color.White else Color.Black.copy(alpha = 0.42f))
            .then(if (t.flat) Modifier.border(BorderStroke(2.dp, Color.Black)) else Modifier)) {
        if (s.chordMode == 1) ChordWheel(side, s, t) else ChordGrid(side, s, t)
    }
}

/** Play a chord on pad [side]: from now on its notes are this chord's (and fingers held there follow it). */
private fun pick(side: Int, root: Int, type: String) {
    val p = TouchPad.pads[side]
    if (p.chordRoot == root && p.chordType == type) return
    TouchPad.update(side) { it.copy(chordRoot = root, chordType = type) }
}

/**
 * Follows one finger over a picker: the chord under it plays as it lands, and again each time it
 * slides onto another, so you can move through chords without lifting it.
 */
private fun Modifier.slideToPick(key1: Any?, key2: Any?, at: (Offset, androidx.compose.ui.unit.IntSize) -> Pair<Int, String>?, play: (Int, String) -> Unit) =
    pointerInput(key1, key2) {
        awaitEachGesture {
            val down = awaitFirstDown()
            val id = down.id
            at(down.position, size)?.let { play(it.first, it.second) }
            down.consume()
            while (true) {
                val e = awaitPointerEvent()
                val c = e.changes.firstOrNull { it.id == id } ?: break
                if (!c.pressed) break
                at(c.position, size)?.let { play(it.first, it.second) }
                c.consume()
            }
        }
    }

// ---- chord names that fit ------------------------------------------------------------------

/**
 * How a set of chord names is drawn: the way they are written on a lead sheet, the root large and
 * the rest of the name smaller and raised beside it ("C" then "maj7♭9"); or, where that will not fit,
 * the root with the rest of the name under it. One style for a whole ring or row, set by its
 * longest name, so the names around a ring all match.
 */
private class NameStyle(val twoLines: Boolean, val root: Float, val suffix: Float)

private val namePaint = android.graphics.Paint().apply { isAntiAlias = true }
private fun textWidth(s: String, size: Float): Float { namePaint.textSize = size; namePaint.isFakeBoldText = false; return namePaint.measureText(s) }

/** The largest style, up to [maxRoot], in which every one of [names] (root, suffix) fits a [boxW] by [boxH] box. */
private fun fitNames(names: List<Pair<String, String>>, boxW: Float, boxH: Float, maxRoot: Float): NameStyle {
    fun widest(rs: Float, ss: Float, two: Boolean) = names.maxOf { (r, x) ->
        if (two) maxOf(textWidth(r, rs), textWidth(x, ss)) else textWidth(r, rs) + textWidth(x, ss) }
    // One line, the suffix 70% of the root.
    var rs = minOf(maxRoot, boxH * 0.78f)
    val w1 = widest(rs, rs * 0.7f, false)
    if (w1 <= boxW || names.all { it.second.isEmpty() }) {
        if (w1 > boxW) rs *= boxW / w1
        return NameStyle(false, rs, rs * 0.7f)
    }
    // One line, a little smaller, still well readable: take it if the root stays at least 80% as big.
    val oneK = boxW / w1
    // Two lines: the root above, the suffix (70%) under it.
    var r2 = minOf(maxRoot, boxH / 1.62f)
    val w2 = widest(r2, r2 * 0.7f, true)
    if (w2 > boxW) r2 *= boxW / w2
    return if (rs * oneK >= r2 * 0.8f) NameStyle(false, rs * oneK, rs * oneK * 0.7f) else NameStyle(true, r2, r2 * 0.7f)
}

/** One chord name, centred on (cx, cy), in a [style]. */
private fun DrawScope.drawName(root: String, suffix: String, cx: Float, cy: Float, style: NameStyle, color: Color, bold: Boolean) {
    val c = drawContext.canvas.nativeCanvas
    namePaint.color = color.toArgb()
    namePaint.isFakeBoldText = bold
    val rs = style.root; val ss = style.suffix
    if (!style.twoLines || suffix.isEmpty()) {
        namePaint.textSize = rs; val rw = namePaint.measureText(root)
        namePaint.textSize = ss; val sw = namePaint.measureText(suffix)
        val x = cx - (rw + sw) / 2
        val base = cy + rs * 0.36f
        namePaint.textSize = rs; c.drawText(root, x, base, namePaint)
        namePaint.textSize = ss; c.drawText(suffix, x + rw, base - rs * 0.3f, namePaint)      // raised, as written
    } else {
        val top = cy - (rs + ss * 1.05f) / 2
        namePaint.textSize = rs; c.drawText(root, cx - namePaint.measureText(root) / 2, top + rs * 0.8f, namePaint)
        namePaint.textSize = ss; c.drawText(suffix, cx - namePaint.measureText(suffix) / 2, top + rs + ss * 0.85f, namePaint)
    }
    namePaint.isFakeBoldText = false
}

/** A chord type on its own, readable: "maj", "m", "dim", "maj7"... */
private fun typeLabel(type: String) = when (type) { "maj" -> "maj"; "dim" -> "dim"; else -> Chords.suffix(type) }

/** The root and suffix of a chord, as drawn: "A" and "m7", "E♭" and "maj9", "B" and "°". */
private fun nameParts(root: Int, type: String) = Chords.PITCH_NAMES[((root % 12) + 12) % 12] to Chords.suffix(type)

// ---- the wheel ------------------------------------------------------------------------------

private const val R_OUT = 1f; private const val R_MID = 0.69f; private const val R_IN = 0.43f; private const val R_HUB = 0.22f
val RING_NAMES = listOf("Outer ring", "Middle ring", "Inner ring")

/**
 * The circle of fifths. Three rings of twelve: each ring plays its own chord type on its roots
 * (the outer ring's roots go round in fifths; the others line up with them, by default as their
 * relative minors and 7th degrees). Touch to play a chord, slide to the next without lifting.
 * Under the wheel, the quick changes: 7, 9, 6, sus4, add9.
 */
@Composable
private fun ChordWheel(side: Int, s: PadSettings, t: PadTheme) {
    val accent = if (t.flat) Color.Black else t.parts[side]
    val ink = if (t.flat) Color.Black else t.railText
    Column(Modifier.fillMaxSize().padding(6.dp), horizontalAlignment = Alignment.CenterHorizontally,
           verticalArrangement = Arrangement.spacedBy(6.dp)) {
        BoxWithConstraints(Modifier.weight(1f).fillMaxWidth(), contentAlignment = Alignment.Center) {
            val d = if (maxWidth < maxHeight) maxWidth else maxHeight
            Canvas(Modifier.size(d).slideToPick(s.wheelRings, s.wheelAlign, at = { o, sz ->
                val c = Offset(sz.width / 2f, sz.height / 2f)
                val r = hypot(o.x - c.x, o.y - c.y) / (sz.width / 2f)
                if (r > R_OUT || r < R_HUB) null else {
                    val ring = if (r > R_MID) 0 else if (r > R_IN) 1 else 2
                    var a = atan2(o.x - c.x, -(o.y - c.y)).toDouble()            // clockwise from the top
                    if (a < 0) a += 2 * PI
                    val i = (floor(a / (PI / 6) + 0.5).toInt()) % 12
                    (Chords.FIFTHS[i] + s.wheelAlign[ring]) % 12 to s.wheelRings[ring]
                }
            }) { root, type -> pick(side, root, type) }) {
                val c = Offset(size.width / 2f, size.height / 2f)
                val rad = size.width / 2f
                val bounds = listOf(R_OUT to R_MID, R_MID to R_IN, R_IN to R_HUB)
                for (ring in 0..2) {
                    val (r1, r0) = bounds[ring]
                    val roots = List(12) { (Chords.FIFTHS[it] + s.wheelAlign[ring]) % 12 }
                    val type = s.wheelRings[ring]
                    // The square that sits inside every segment of the ring, whatever its angle: the
                    // smaller of the ring's thickness and the segment's width across, with a margin.
                    val mid = rad * (r0 + r1) / 2
                    val side = minOf(rad * (r1 - r0), mid * (PI / 6).toFloat()) * 0.84f
                    val style = fitNames(roots.map { nameParts(it, type) }, side, side,
                                         maxRoot = rad * (if (ring == 0) 0.13f else if (ring == 1) 0.105f else 0.085f))
                    for (i in 0..11) {
                        val root = roots[i]
                        val on = s.chordRoot == root && s.chordType == type
                        val a0 = ((i - 0.5) * 30 - 90).toFloat()
                        val p = Path().apply {
                            arcTo(Rect(c, rad * r1), a0, 30f, true)
                            arcTo(Rect(c, rad * r0), a0 + 30f, -30f, false)
                            close()
                        }
                        val base = if (t.flat) Color.White else when (ring) { 0 -> Color(0xFF262A35); 1 -> Color(0xFF1D2029); else -> Color(0xFF171A21) }
                        drawPath(p, if (on) accent else base)
                        drawPath(p, if (t.flat) Color.Black else Color(0xFF0B0D11), style = Stroke(width = if (t.flat) 2f else 3f))
                        val a = Math.toRadians(i * 30.0)
                        val (rn, sf) = nameParts(root, type)
                        drawName(rn, sf, c.x + mid * sin(a).toFloat(), c.y - mid * cos(a).toFloat(), style,
                                 if (on) (if (t.flat) Color.White else Color(0xFF14161B)) else ink, bold = on)
                    }
                }
                // The hub: the chord in hand, with its quick change.
                drawCircle(if (t.flat) Color.White else Color(0xFF0F1116), radius = rad * R_HUB, center = c)
                drawCircle(accent, radius = rad * R_HUB, center = c, style = Stroke(width = 3f))
                val hub = nameParts(s.chordRoot, s.chord)
                val hubSide = rad * R_HUB * 1.45f
                drawName(hub.first, hub.second, c.x, c.y, fitNames(listOf(hub), hubSide, hubSide, rad * 0.14f), accent, bold = true)
            }
        }
        // The quick changes: tap one to put it on every chord you pick, tap again to take it off.
        Row(horizontalArrangement = Arrangement.spacedBy(4.dp)) {
            for (m in Chords.MODS.drop(1)) {
                val on = s.chordMod == m
                Box(Modifier.background(if (on) accent.copy(alpha = if (t.flat) 1f else 0.3f) else Color.Transparent, RoundedCornerShape(50))
                        .border(BorderStroke(1.dp, if (on) accent else ink.copy(alpha = 0.4f)), RoundedCornerShape(50))
                        .pointerInput(on, side) { detectTapGestures(onPress = {
                            TouchPad.update(side) { it.copy(chordMod = if (on) "" else m) } }) }
                        .padding(horizontal = 9.dp, vertical = 5.dp)) {
                    Text(m, fontSize = 12.sp, color = if (on && t.flat) Color.White else if (on) accent else ink)
                }
            }
        }
    }
}

// ---- the grid -------------------------------------------------------------------------------

/**
 * The chord grid: the twelve roots across (in fifths, C in the middle) and a chord type in each
 * row. Touch a cell to play that chord, slide to the next without lifting. The corner shows the
 * chord in hand.
 */
@Composable
private fun ChordGrid(side: Int, s: PadSettings, t: PadTheme) {
    val accent = if (t.flat) Color.Black else t.parts[side]
    val ink = if (t.flat) Color.Black else t.railText
    val density = androidx.compose.ui.platform.LocalDensity.current
    val labelW = with(density) { 46.dp.toPx() }
    val headH = with(density) { 22.dp.toPx() }
    val gap = with(density) { 3.dp.toPx() }
    fun cellAt(o: Offset, w: Float, h: Float): Pair<Int, Int>? {
        if (o.x < labelW || o.y < headH) return null
        val col = ((o.x - labelW) / ((w - labelW) / 12f)).toInt().coerceIn(0, 11)
        val row = ((o.y - headH) / ((h - headH) / s.gridRows.size)).toInt().coerceIn(0, s.gridRows.lastIndex)
        return col to row
    }
    Canvas(Modifier.fillMaxSize().padding(5.dp).slideToPick(s.gridRows, side, at = { o, sz ->
        cellAt(o, sz.width.toFloat(), sz.height.toFloat())?.let { (col, row) -> Chords.GRID_ROOTS[col] to s.gridRows[row] }
    }) { root, type -> pick(side, root, type) }) {
        val cw = (size.width - labelW) / 12f
        val rh = (size.height - headH) / s.gridRows.size
        // The chord in hand, in the corner.
        val now = nameParts(s.chordRoot, s.chord)
        drawName(now.first, now.second, labelW / 2, headH / 2, fitNames(listOf(now), labelW - gap, headH, headH * 0.8f), accent, bold = true)
        // The roots along the top, all one size.
        val rootStyle = fitNames(Chords.GRID_ROOTS.map { Chords.PITCH_NAMES[it] to "" }, cw - gap, headH, headH * 0.62f)
        for ((k, r) in Chords.GRID_ROOTS.withIndex())
            drawName(Chords.PITCH_NAMES[r], "", labelW + cw * (k + 0.5f), headH / 2, rootStyle, ink.copy(alpha = 0.75f), bold = false)
        // The rows' types, all one size, set by the longest.
        val rowStyle = fitNames(s.gridRows.map { typeLabel(it) to "" }, labelW - gap * 2, rh, minOf(rh * 0.42f, headH * 0.7f))
        for ((row, type) in s.gridRows.withIndex()) {
            val y = headH + rh * row
            // The row's type, written as a chord's suffix is ("maj" for a plain major chord).
            drawName(typeLabel(type), "", labelW / 2, y + rh / 2, rowStyle, ink.copy(alpha = 0.85f), bold = true)
            for ((k, r) in Chords.GRID_ROOTS.withIndex()) {
                val on = s.chordRoot == r && s.chordType == type
                val x = labelW + cw * k
                drawRoundRect(if (on) accent else if (t.flat) Color.White else Color(0xFF1C1F27), Offset(x + gap / 2, y + gap / 2),
                              Size(cw - gap, rh - gap), androidx.compose.ui.geometry.CornerRadius(gap * 1.5f))
                drawRoundRect(if (t.flat) Color.Black else Color(0xFF333846), Offset(x + gap / 2, y + gap / 2),
                              Size(cw - gap, rh - gap), androidx.compose.ui.geometry.CornerRadius(gap * 1.5f), style = Stroke(width = 1.5f))
            }
        }
    }
}

// ---- changing what the wheel and the grid hold (in the Scale panel) ------------------------

/**
 * In the Scale panel: pick a ring of the wheel or a row of the grid, then what it plays, and for a
 * ring where its roots sit. Changes show on the pad's picker at once.
 */
@Composable
fun ChordLayoutEditor(side: Int, s: PadSettings, accent: Color) {
    val wheel = s.chordMode == 1
    var which by remember(s.chordMode) { mutableIntStateOf(0) }
    val names = if (wheel) RING_NAMES.mapIndexed { i, n -> "$n: ${typeLabel(s.wheelRings[i])}" }
                else s.gridRows.mapIndexed { i, x -> "${i + 1}: ${typeLabel(x)}" }
    Text(if (wheel) "Rings" else "Rows", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 10.dp))
    Chips(names, which, accent) { which = it }
    val current = if (wheel) s.wheelRings[which] else s.gridRows[which]
    for ((g, types) in Chords.GROUPS) {
        Text(g, color = TextDim, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp))
        Chips(types, types.indexOf(current), accent) { k ->
            val type = types[k]
            TouchPad.update(side) { p ->
                // A new type lines its roots up the natural way (changed below if you like).
                if (wheel) p.copy(wheelRings = p.wheelRings.mapIndexed { i, x -> if (i == which) type else x },
                                  wheelAlign = p.wheelAlign.mapIndexed { i, x -> if (i == which && i > 0) naturalAlign(type) else x })
                else p.copy(gridRows = p.gridRows.mapIndexed { i, x -> if (i == which) type else x })
            }
        }
    }
    if (wheel && which > 0) {
        Text("Its roots, from the outer ring's", color = TextDim, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp))
        Chips(Chords.ALIGN.map { it.second }, Chords.ALIGN.indexOfFirst { it.first == s.wheelAlign[which] }, accent) { k ->
            TouchPad.update(side) { p -> p.copy(wheelAlign = p.wheelAlign.mapIndexed { i, x -> if (i == which) Chords.ALIGN[k].first else x }) }
        }
    }
}

/** Where a ring of this type sits best under the outer ring: minor chords on the relative minor, diminished on the 7th, the rest on the same root. */
private fun naturalAlign(type: String): Int {
    val iv = Chords.TYPES[type] ?: return 0
    return when {
        3 in iv && 6 in iv -> 11
        3 in iv && 4 !in iv -> 9
        else -> 0
    }
}
