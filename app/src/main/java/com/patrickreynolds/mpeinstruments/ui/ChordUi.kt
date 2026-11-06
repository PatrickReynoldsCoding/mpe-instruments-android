package com.patrickreynolds.mpeinstruments.ui

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.Chords
import com.patrickreynolds.mpeinstruments.PadSettings
import kotlin.math.PI
import kotlin.math.atan2
import kotlin.math.cos
import kotlin.math.floor
import kotlin.math.hypot
import kotlin.math.sin

/** The chord picker of pad [side]: the wheel or the grid, with its editor open over it while you change a ring or a row. */
@Composable
fun ChordPicker(side: Int, s: PadSettings, t: PadTheme, modifier: Modifier) {
    var editing by remember { mutableStateOf(-1) }            // the ring or row being changed, or -1
    Box(modifier.background(if (t.flat) Color.White else Color.Black.copy(alpha = 0.42f))
            .then(if (t.flat) Modifier.border(BorderStroke(2.dp, Color.Black)) else Modifier)) {
        if (s.chordMode == 1) ChordWheel(side, s, t) { editing = it } else ChordGrid(side, s, t) { editing = it }
        if (editing >= 0) ChordTypeEditor(side, s, editing, t) { editing = -1 }
    }
}

/** Play a chord on pad [side]: from now on its notes are this chord's. */
private fun pick(side: Int, root: Int, type: String, keepMod: Boolean = true) =
    AppState.updatePad(side) { it.copy(chordRoot = root, chordType = type, chordMod = if (keepMod) it.chordMod else "") }

private const val R_OUT = 1f; private const val R_MID = 0.69f; private const val R_IN = 0.43f; private const val R_HUB = 0.22f
private val RING_NAMES = listOf("Outer ring", "Middle ring", "Inner ring")

/**
 * The circle of fifths. Three rings of twelve: each ring plays its own chord type on its roots
 * (the outer ring's roots go round in fifths; the others line up with them, by default as their
 * relative minors and 7th degrees). Tap to play a chord; hold a ring to change it. Under the
 * wheel, the quick changes: 7, 9, 6, sus4, add9.
 */
@Composable
private fun ChordWheel(side: Int, s: PadSettings, t: PadTheme, edit: (Int) -> Unit) {
    val accent = if (t.flat) Color.Black else t.parts[side]
    val ink = if (t.flat) Color.Black else t.railText
    Column(Modifier.fillMaxSize().padding(6.dp), horizontalAlignment = Alignment.CenterHorizontally,
           verticalArrangement = Arrangement.spacedBy(6.dp)) {
        BoxWithConstraints(Modifier.weight(1f).fillMaxWidth(), contentAlignment = Alignment.Center) {
            val d = if (maxWidth < maxHeight) maxWidth else maxHeight
            Canvas(Modifier.size(d).pointerInput(s.wheelRings, s.wheelAlign, side) {
                fun hit(o: Offset): Pair<Int, Int>? {
                    val c = Offset(size.width / 2f, size.height / 2f)
                    val r = hypot(o.x - c.x, o.y - c.y) / (size.width / 2f)
                    if (r > R_OUT || r < R_HUB) return null
                    val ring = if (r > R_MID) 0 else if (r > R_IN) 1 else 2
                    var a = atan2(o.x - c.x, -(o.y - c.y)).toDouble()          // clockwise from the top
                    if (a < 0) a += 2 * PI
                    val i = (floor(a / (PI / 6) + 0.5).toInt()) % 12
                    return ring to i
                }
                // A press plays the chord at once; held, it opens that ring's editor and puts back the chord there was.
                var before = s.chordRoot to s.chordType
                detectTapGestures(
                    onPress = { o -> hit(o)?.let { (ring, i) ->
                        before = AppState.pads[side].let { it.chordRoot to it.chordType }
                        pick(side, (Chords.FIFTHS[i] + s.wheelAlign[ring]) % 12, s.wheelRings[ring]) } },
                    onLongPress = { o -> hit(o)?.let { (ring, _) -> pick(side, before.first, before.second); edit(ring) } })
            }) {
                val c = Offset(size.width / 2f, size.height / 2f)
                val rad = size.width / 2f
                val paint = android.graphics.Paint().apply { isAntiAlias = true; textAlign = android.graphics.Paint.Align.CENTER }
                val bounds = listOf(R_OUT to R_MID, R_MID to R_IN, R_IN to R_HUB)
                for (ring in 0..2) {
                    val (r1, r0) = bounds[ring]
                    for (i in 0..11) {
                        val root = (Chords.FIFTHS[i] + s.wheelAlign[ring]) % 12
                        val type = s.wheelRings[ring]
                        val on = s.chordRoot == root && s.chordType == type
                        val a0 = ((i - 0.5) * 30 - 90).toFloat()
                        val p = Path().apply {
                            arcTo(androidx.compose.ui.geometry.Rect(c, rad * r1), a0, 30f, true)
                            arcTo(androidx.compose.ui.geometry.Rect(c, rad * r0), a0 + 30f, -30f, false)
                            close()
                        }
                        val base = if (t.flat) Color.White else when (ring) { 0 -> Color(0xFF262A35); 1 -> Color(0xFF1D2029); else -> Color(0xFF171A21) }
                        drawPath(p, if (on) accent else base)
                        drawPath(p, if (t.flat) Color.Black else Color(0xFF0B0D11), style = Stroke(width = if (t.flat) 2f else 3f))
                        val mid = rad * (r0 + r1) / 2
                        val a = Math.toRadians(i * 30.0)
                        val label = Chords.name(root, type)
                        // Long names shrink to fit their segment (the inner ones are narrowest).
                        paint.textSize = rad * (if (ring == 0) 0.115f else if (ring == 1) 0.09f else 0.08f)
                        val room = (rad * (r0 + r1) / 2) * (PI / 6).toFloat() * 0.92f     // the segment's width at its middle
                        val wide = paint.measureText(label)
                        if (wide > room) paint.textSize *= room / wide
                        paint.color = (if (on) (if (t.flat) Color.White else Color(0xFF14161B)) else ink).toArgb()
                        paint.isFakeBoldText = on
                        drawContext.canvas.nativeCanvas.drawText(label, c.x + mid * sin(a).toFloat(),
                                                                 c.y - mid * cos(a).toFloat() + paint.textSize * 0.35f, paint)
                    }
                }
                // The hub: the chord in hand, with its quick change.
                drawCircle(if (t.flat) Color.White else Color(0xFF0F1116), radius = rad * R_HUB, center = c)
                drawCircle(accent, radius = rad * R_HUB, center = c, style = Stroke(width = 3f))
                paint.textSize = rad * 0.1f; paint.color = accent.toArgb(); paint.isFakeBoldText = true
                drawContext.canvas.nativeCanvas.drawText(Chords.name(s.chordRoot, s.chord), c.x, c.y + paint.textSize * 0.35f, paint)
            }
        }
        // The quick changes: tap one to put it on every chord you pick, tap again to take it off.
        Row(horizontalArrangement = Arrangement.spacedBy(4.dp)) {
            for (m in Chords.MODS.drop(1)) {
                val on = s.chordMod == m
                Box(Modifier.background(if (on) accent.copy(alpha = if (t.flat) 1f else 0.3f) else Color.Transparent, RoundedCornerShape(50))
                        .border(BorderStroke(1.dp, if (on) accent else ink.copy(alpha = 0.4f)), RoundedCornerShape(50))
                        .pointerInput(on, side) { detectTapGestures(onPress = {
                            AppState.updatePad(side) { it.copy(chordMod = if (on) "" else m) } }) }
                        .padding(horizontal = 9.dp, vertical = 5.dp)) {
                    Text(m, fontSize = 12.sp, color = if (on && t.flat) Color.White else if (on) accent else ink)
                }
            }
        }
        Text("Hold a ring to change it", fontSize = 10.sp, color = ink.copy(alpha = 0.5f))
    }
}

/**
 * The chord grid: the twelve roots across (in fifths, C in the middle) and a chord type in each
 * row. Tap a cell to play that chord; hold a row's name to change its type.
 */
@Composable
private fun ChordGrid(side: Int, s: PadSettings, t: PadTheme, edit: (Int) -> Unit) {
    val accent = if (t.flat) Color.Black else t.parts[side]
    val ink = if (t.flat) Color.Black else t.railText
    Column(Modifier.fillMaxSize().padding(5.dp), verticalArrangement = Arrangement.spacedBy(3.dp)) {
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(3.dp)) {
            Box(Modifier.width(40.dp))
            for (r in Chords.GRID_ROOTS) Text(Chords.PITCH_NAMES[r], fontSize = 10.sp, color = ink.copy(alpha = 0.7f),
                                             textAlign = TextAlign.Center, maxLines = 1, modifier = Modifier.weight(1f))
        }
        for ((row, type) in s.gridRows.withIndex()) {
            Row(Modifier.fillMaxWidth().weight(1f), horizontalArrangement = Arrangement.spacedBy(3.dp)) {
                Box(Modifier.width(40.dp).fillMaxHeight()
                        .pointerInput(row) { detectTapGestures(onLongPress = { edit(row) }, onTap = { edit(row) }) },
                    contentAlignment = Alignment.CenterStart) {
                    Text(if (type == "maj") "Major" else if (type == "min") "Minor" else type, fontSize = 11.sp, maxLines = 1,
                         color = ink.copy(alpha = 0.8f), fontWeight = FontWeight.SemiBold)
                }
                for (r in Chords.GRID_ROOTS) {
                    val on = s.chordRoot == r && s.chordType == type
                    Box(Modifier.weight(1f).fillMaxHeight()
                            .background(if (on) accent else if (t.flat) Color.White else Color(0xFF1C1F27), RoundedCornerShape(4.dp))
                            .border(BorderStroke(1.dp, if (t.flat) Color.Black else Color(0xFF333846)), RoundedCornerShape(4.dp))
                            .pointerInput(r, type, side) { detectTapGestures(onPress = { pick(side, r, type, keepMod = false) }) })
                }
            }
        }
    }
}

/** Change what a wheel ring (0 outer, 1 middle, 2 inner) or a grid row plays, right over the picker. */
@Composable
private fun ChordTypeEditor(side: Int, s: PadSettings, which: Int, t: PadTheme, done: () -> Unit) {
    val wheel = s.chordMode == 1
    val accent = if (t.flat) Color.Black else t.parts[side]
    val current = if (wheel) s.wheelRings[which] else s.gridRows[which]
    Surface(Modifier.fillMaxSize().padding(4.dp), shape = RoundedCornerShape(14.dp),
            color = if (t.flat) Color.White else Panel.copy(alpha = 0.97f), contentColor = Fg,
            border = if (t.flat) BorderStroke(2.dp, Color.Black) else null) {
        Column(Modifier.padding(10.dp).verticalScroll(rememberScrollState())) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(if (wheel) RING_NAMES[which] else "Row ${which + 1}", fontSize = 15.sp, fontWeight = FontWeight.SemiBold,
                     modifier = Modifier.weight(1f))
                TextButton(onClick = done) { Text("Done") }
            }
            for ((g, types) in Chords.GROUPS) {
                Text(g, color = TextDim, fontSize = 12.sp, modifier = Modifier.padding(top = 4.dp))
                Chips(types, types.indexOf(current), accent) { k ->
                    val type = types[k]
                    AppState.updatePad(side) { p ->
                        // A new type lines its roots up the natural way (the editor below can change it).
                        if (wheel) p.copy(wheelRings = p.wheelRings.mapIndexed { i, x -> if (i == which) type else x },
                                          wheelAlign = p.wheelAlign.mapIndexed { i, x -> if (i == which && i > 0) naturalAlign(type) else x })
                        else p.copy(gridRows = p.gridRows.mapIndexed { i, x -> if (i == which) type else x })
                    }
                }
            }
            if (wheel && which > 0) {
                Text("Its roots, from the outer ring's", color = TextDim, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp))
                Chips(Chords.ALIGN.map { it.second }, Chords.ALIGN.indexOfFirst { it.first == s.wheelAlign[which] }, accent) { k ->
                    AppState.updatePad(side) { p -> p.copy(wheelAlign = p.wheelAlign.mapIndexed { i, x -> if (i == which) Chords.ALIGN[k].first else x }) }
                }
            }
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
