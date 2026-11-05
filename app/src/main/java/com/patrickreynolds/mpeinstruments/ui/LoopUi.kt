package com.patrickreynolds.mpeinstruments.ui

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
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
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Fill
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.Looper
import com.patrickreynolds.mpeinstruments.Preset
import kotlinx.coroutines.delay
import kotlin.math.roundToInt

/** The loops as they are now, for the screen, and which one the buttons act on. */
object LoopLive {
    var views by mutableStateOf(List(Looper.LOOPS) { Looper.View() })
    var focus by mutableIntStateOf(0)
    val view get() = views[focus]
}

/** Keeps LoopLive up to date while shown (every frame while a loop moves, a few times a second otherwise). */
@Composable
fun LoopTicker() {
    LaunchedEffect(Unit) {
        while (true) {
            val vs = List(Looper.LOOPS) { Looper.read(it) }
            if (vs != LoopLive.views) LoopLive.views = vs
            val moving = vs.any { it.state == Looper.ARMED || it.state == Looper.RECORDING || it.state == Looper.OVERDUBBING ||
                                  (it.state == Looper.PLAYING && !it.paused) }
            if (!moving) delay(200) else if (Eink) delay(500) else withFrameNanos { }
        }
    }
}

/** A word or two for the rail: "Empty", "Rec 3 s", "2 loops". */
fun loopSummary(vs: List<Looper.View> = LoopLive.views): String {
    val used = vs.count { it.used }
    val f = LoopLive.view
    return when {
        f.state == Looper.ARMED -> "Ready"
        f.state == Looper.RECORDING -> "Rec ${f.recordedSeconds.roundToInt()} s"
        f.state == Looper.OVERDUBBING -> "Layer ${f.layers + 1}"
        used == 0 -> "Empty"
        used == 1 -> if (f.layers == 1) "1 layer" else "${vs.first { it.used }.layers} layers"
        else -> "$used loops"
    }
}

/** What to do next with loop t, in a sentence. */
private fun loopHint(t: Int, v: Looper.View) = "Loop ${t + 1}: " + when (v.state) {
    Looper.ARMED -> "play, recording starts with your first note"
    Looper.RECORDING -> "recording %.1f s, tap it to close the loop".format(v.recordedSeconds)
    Looper.OVERDUBBING -> "recording layer ${v.layers + 1}, one time round"
    Looper.PLAYING -> when {
        v.muted -> "muted (hold it for more)"
        v.paused -> "paused (hold it for more)"
        v.layers >= Looper.MAX_LAYERS -> "%.1f s, all 8 layers used".format(v.seconds)
        else -> "%.1f s, tap it to add a layer".format(v.seconds)
    }
    else -> "tap a ring to record"
}

private fun shortName(name: String) = if (name.length <= 9) name else name.take(8) + "…"

/**
 * The loop buttons on the pad, top right. A row of your presets (tap to switch to it, + to keep
 * what is playing as a new one, hold one for more); then the loops: a ring each, filling as it
 * goes round, red while recording (tap one to record or add a layer, hold it to mute, pause, undo
 * or clear), and an empty ring for a new loop of its own length; undo, and pause all.
 */
@Composable
fun LoopDock(t: PadTheme, modifier: Modifier = Modifier) {
    val vs = LoopLive.views
    val flat = t.flat
    val ink = if (flat) Color.Black else t.railText
    val bg = if (flat) Color.White else Color.Black.copy(alpha = if (t.name == "Daylight") 0.12f else 0.5f)
    var renaming by remember { mutableStateOf<Preset?>(null) }
    Column(modifier.background(bg, RoundedCornerShape(14.dp))
               .then(if (flat) Modifier.border(BorderStroke(2.dp, Color.Black), RoundedCornerShape(14.dp)) else Modifier)
               .padding(5.dp),
           horizontalAlignment = Alignment.End) {
        Row(horizontalArrangement = Arrangement.spacedBy(5.dp), verticalAlignment = Alignment.CenterVertically) {
            // All four, always in the same place: a ring must not move under a finger about to tap it.
            for (k in 0 until Looper.LOOPS) LoopRing(k, vs[k], k == LoopLive.focus, t)
            Box(Modifier.width(2.dp))
            val f = LoopLive.focus
            val fv = vs[f]
            DockButton("↶", enabled = fv.used, t) {
                val dropped = if (fv.state == Looper.OVERDUBBING) fv.layers else fv.layers - 1
                Looper.undo(f)
                if (dropped >= 0) AppState.resetLoopLayers(f, dropped)
            }
            val anyPlaying = vs.any { it.state == Looper.PLAYING && !it.paused }
            val anyPaused = vs.any { it.state == Looper.PLAYING && it.paused }
            DockButton(if (!anyPlaying && anyPaused) "▶" else "❚❚", enabled = anyPlaying || anyPaused, t) {
                val pause = anyPlaying
                for (k in 0 until Looper.LOOPS) if (vs[k].state == Looper.PLAYING) Looper.pause(k, pause)
            }
        }
        Text(loopHint(LoopLive.focus, vs[LoopLive.focus]), color = ink.copy(alpha = 0.8f), fontSize = 11.sp,
             modifier = Modifier.padding(top = 2.dp, end = 4.dp, bottom = 4.dp))
        // Your presets, to switch between while the loops play.
        Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(5.dp)) {
            val list = AppState.userPresets
            val state = rememberLazyListState()
            LaunchedEffect(AppState.presetName) {
                val i = list.indexOfFirst { it.name == AppState.presetName }
                if (i >= 0) state.animateScrollToItem(i)
            }
            LazyRow(Modifier.widthIn(max = 330.dp), state = state, horizontalArrangement = Arrangement.spacedBy(5.dp)) {
                itemsIndexed(list, key = { _, p -> p.name }) { _, p -> PresetChip(p, p.name == AppState.presetName, t) { renaming = p } }
            }
            DockButton("+", enabled = true, t) { AppState.newPreset() }
        }
    }
    renaming?.let { RenameDialog(it) { renaming = null } }
}

@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun PresetChip(p: Preset, active: Boolean, t: PadTheme, onRename: () -> Unit) {
    var menu by remember { mutableStateOf(false) }
    val col = if (t.flat) Color.Black else t.parts[0]
    Box {
        Box(Modifier.height(36.dp).widthIn(min = 52.dp)
                .background(when { t.flat && active -> Color.Black; t.flat -> Color.White; active -> col.copy(alpha = 0.4f)
                                   else -> Color.Black.copy(alpha = 0.3f) }, RoundedCornerShape(10.dp))
                .border(BorderStroke(if (active) 2.dp else 1.dp, if (t.flat) Color.Black else if (active) col else t.railDim.copy(alpha = 0.6f)),
                        RoundedCornerShape(10.dp))
                .combinedClickable(onClick = { AppState.load(p) }, onLongClick = { menu = true })
                .padding(horizontal = 8.dp),
            contentAlignment = Alignment.Center) {
            Text(shortName(p.name), fontSize = 11.sp, maxLines = 1, fontWeight = if (active) FontWeight.Bold else FontWeight.Normal,
                 color = when { t.flat && active -> Color.White; t.flat -> Color.Black; else -> t.railText })
        }
        PresetMenu(p, menu, { menu = false }, onRename)
    }
}

/** One loop's button: a ring that fills as the loop goes round; red while recording. */
@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun LoopRing(k: Int, v: Looper.View, focused: Boolean, t: PadTheme) {
    var menu by remember { mutableStateOf(false) }
    val red = if (t.flat) Color.Black else Color(0xFFFF5252)
    val green = if (t.flat) Color.Black else if (v.muted) t.railDim else Color(0xFF69F0AE)
    val recording = v.state == Looper.RECORDING || v.state == Looper.OVERDUBBING
    val blink = v.state == Looper.ARMED && (System.currentTimeMillis() / 400) % 2 == 0L
    Box {
        Box(Modifier.size(48.dp).combinedClickable(
                onClick = { LoopLive.focus = k; Looper.press(k) },
                onLongClick = { LoopLive.focus = k; if (v.used) menu = true }),
            contentAlignment = Alignment.Center) {
            Canvas(Modifier.fillMaxSize()) {
                val sw = 4.5.dp.toPx()
                val r = size.minDimension / 2 - sw
                val c = Offset(size.width / 2, size.height / 2)
                drawCircle(if (t.flat) Color.White else Color.Black.copy(alpha = 0.35f), radius = r + sw / 2, center = c)
                drawCircle((if (t.flat) Color.Black else t.railDim).copy(alpha = if (focused) 0.9f else 0.45f), radius = r, center = c,
                           style = Stroke(if (focused) sw else sw / 2))
                val ring = if (recording) red else green
                if (v.length > 0) drawArc(ring, -90f, 360f * v.progress, false, topLeft = Offset(c.x - r, c.y - r), size = Size(2 * r, 2 * r),
                                          style = Stroke(sw))
                else if (v.state == Looper.RECORDING) drawCircle(red, radius = r, center = c, style = Stroke(sw))
                when {
                    v.state == Looper.RECORDING -> drawRect(red, Offset(c.x - r * 0.33f, c.y - r * 0.33f), Size(r * 0.66f, r * 0.66f))
                    v.state == Looper.ARMED -> drawCircle(red, radius = r * 0.4f, center = c, style = if (blink) Stroke(sw / 2) else Fill)
                    v.state == Looper.EMPTY -> drawCircle(red.copy(alpha = if (focused) 0.6f else 0.3f), radius = r * 0.26f, center = c)
                    v.state == Looper.OVERDUBBING -> drawCircle(red, radius = r * 0.4f, center = c)
                    else -> drawCircle(red, radius = r * 0.3f, center = c)
                }
            }
            Text("${k + 1}", fontSize = 10.sp, fontWeight = FontWeight.Bold, color = if (t.flat) Color.Black else t.railText,
                 modifier = Modifier.align(Alignment.TopStart))
            if (v.layers > 0) Text("${v.layers}", fontSize = 10.sp, color = if (t.flat) Color.Black else t.railDim,
                                   modifier = Modifier.align(Alignment.BottomEnd))
        }
        DropdownMenu(expanded = menu, onDismissRequest = { menu = false }) {
            DropdownMenuItem(text = { Text(if (v.muted) "Unmute" else "Mute") }, onClick = { Looper.mute(k, !v.muted); menu = false })
            if (v.state == Looper.PLAYING) DropdownMenuItem(text = { Text(if (v.paused) "Play" else "Pause") },
                                                             onClick = { Looper.pause(k, !v.paused); menu = false })
            DropdownMenuItem(text = { Text("Undo the last layer") }, onClick = {
                val dropped = if (v.state == Looper.OVERDUBBING) v.layers else v.layers - 1
                Looper.undo(k); if (dropped >= 0) AppState.resetLoopLayers(k, dropped); menu = false })
            DropdownMenuItem(text = { Text("Clear this loop", color = Warn) }, onClick = {
                Looper.clear(k); AppState.resetLoopLayers(k); menu = false })
        }
    }
}

@Composable
private fun DockButton(label: String, enabled: Boolean, t: PadTheme, click: () -> Unit) {
    Box(Modifier.size(38.dp).background(if (t.flat) Color.White else Color.Black.copy(alpha = 0.3f), CircleShape)
            .border(BorderStroke(1.dp, if (t.flat) Color.Black else t.railDim.copy(alpha = 0.6f)), CircleShape)
            .then(if (enabled) Modifier.clickable(onClick = click) else Modifier),
        contentAlignment = Alignment.Center) {
        Text(label, fontSize = 16.sp, color = (if (t.flat) Color.Black else t.railText).copy(alpha = if (enabled) 1f else 0.35f),
             textAlign = TextAlign.Center)
    }
}

/** The Loop panel on the pad's rail: the buttons on or off, snap, tempo, and each loop with its layers. */
@Composable
fun LoopControls(accent: Color) {
    val vs = LoopLive.views
    Text("Up to four loops, each its own length: a short drum loop and a long chord loop, say. On a loop, the first " +
         "press records it, from your first note; each press after that records one time round over it, as a new layer. " +
         "Switch presets between layers to build it up from different sounds.", color = TextDim, fontSize = 12.sp)
    SwitchRow("Loop buttons on the pad", AppState.loopDock, "Loops, undo, pause and your presets, at the top right", accent = accent) {
        AppState.changeLoopDock(it) }
    SwitchRow("Snap lengths to beats", AppState.loopSnap, "Every loop is a whole number of beats at the tempo, so loops of " +
              "different lengths, the arpeggiator and the delay stay in time", accent = accent) { AppState.changeLoopSnap(it) }
    val bpm = AppState.perf.bpm
    LabeledSlider("Tempo", bpm, 40f..220f, "${bpm.roundToInt()} bpm", accent = accent) { x ->
        AppState.update { it.copy(bpm = x.roundToInt().toFloat()) } }
    val shown = (vs.indexOfLast { it.used } + 2).coerceIn(1, Looper.LOOPS)
    for (k in 0 until shown) {
        val v = vs[k]
        HorizontalDivider(color = Line, modifier = Modifier.padding(vertical = 8.dp))
        Text(loopHint(k, v), fontSize = 14.sp, fontWeight = FontWeight.SemiBold)
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(2.dp)) {
            TextButton(onClick = { LoopLive.focus = k; Looper.press(k) }) { Text(when (v.state) {
                Looper.EMPTY -> "Record"; Looper.ARMED -> "Cancel"; Looper.RECORDING -> "Close it"
                Looper.OVERDUBBING -> "Stop layer"; else -> "Add layer" }) }
            if (v.used) {
                TextButton(onClick = { Looper.mute(k, !v.muted) }) { Text(if (v.muted) "Unmute" else "Mute", color = if (v.muted) Warn else Color.Unspecified) }
                TextButton(onClick = {
                    val dropped = if (v.state == Looper.OVERDUBBING) v.layers else v.layers - 1
                    Looper.undo(k); if (dropped >= 0) AppState.resetLoopLayers(k, dropped) }) { Text("Undo") }
                TextButton(onClick = { Looper.clear(k); AppState.resetLoopLayers(k) }) { Text("Clear") }
            }
        }
        for (i in 0 until v.layers) {
            val (vol, muted) = AppState.loopLayers[k][i]
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text("Layer ${i + 1}", fontSize = 13.sp, modifier = Modifier.width(64.dp))
                Canvas(Modifier.width(36.dp).height(8.dp)) {                // how loud it was recorded
                    drawRect(TextDim.copy(alpha = 0.3f), size = size)
                    drawRect(if (muted) TextDim else accent, size = Size(size.width * v.level[i].coerceIn(0f, 1f), size.height))
                }
                TextButton(onClick = { AppState.changeLoopLayer(k, i, vol, !muted) }) {
                    Text(if (muted) "Muted" else "Mute", color = if (muted) Warn else Color.Unspecified, fontSize = 13.sp) }
            }
            LabeledSlider("Volume", vol, 0f..1.5f, pct(vol), accent = accent) { x -> AppState.changeLoopLayer(k, i, x, muted) }
        }
    }
}
