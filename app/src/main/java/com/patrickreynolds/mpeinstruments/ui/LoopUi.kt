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
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.Looper
import com.patrickreynolds.mpeinstruments.Part
import kotlinx.coroutines.delay
import kotlin.math.roundToInt

/** The looper as it is now, for the screen: read every frame while the pad is open. */
object LoopLive {
    var view by mutableStateOf(Looper.View())
}

/** Keeps LoopLive up to date while shown (every frame while something moves, a few times a second otherwise). */
@Composable
fun LoopTicker() {
    LaunchedEffect(Unit) {
        while (true) {
            val v = Looper.read()
            if (v != LoopLive.view) LoopLive.view = v
            if (v.state == Looper.EMPTY || (v.state == Looper.PLAYING && v.paused)) delay(200)
            else if (Eink) delay(500) else withFrameNanos { }
        }
    }
}

/** A word or two for the rail: "Off", "Ready", "Rec 3 s", "2 layers". */
fun loopSummary(v: Looper.View = LoopLive.view) = when (v.state) {
    Looper.ARMED -> "Ready"
    Looper.RECORDING -> "Rec ${v.recordedSeconds.roundToInt()} s"
    Looper.OVERDUBBING -> "Layer ${v.layers + 1}"
    Looper.PLAYING -> if (v.paused) "Paused" else if (v.layers == 1) "1 layer" else "${v.layers} layers"
    else -> "Empty"
}

/** What to do next, in a sentence. */
private fun loopHint(v: Looper.View) = when (v.state) {
    Looper.ARMED -> "Play: recording starts with your first note"
    Looper.RECORDING -> "Recording %.1f s · tap ● to close the loop".format(v.recordedSeconds)
    Looper.OVERDUBBING -> "Recording layer ${v.layers + 1} · one time round"
    Looper.PLAYING -> if (v.paused) "Paused · ▶ to carry on"
                      else if (v.layers >= Looper.MAX_LAYERS) "%.1f s loop · all 8 layers used".format(v.seconds)
                      else "%.1f s loop · pick a sound, ● to add a layer".format(v.seconds)
    else -> "Tap ● to record a loop"
}

private val GROUP_COLORS = mapOf(
    "drums" to Color(0xFFFF8A65), "plucked" to Color(0xFFFFD54F), "strings" to Color(0xFFE57373), "synths" to Color(0xFF4DD0E1),
    "voices" to Color(0xFFF06292), "winds" to Color(0xFF81C784), "struck" to Color(0xFFB39DDB),
)

private fun shortName(name: String) = if (name.length <= 7) name else name.substringBefore(' ').take(7)

/**
 * The loop buttons on the pad: the sound slots (tap to play that sound, long-press to keep the
 * sound now playing there), then record, undo and pause, and a line saying what happens next.
 */
@OptIn(ExperimentalFoundationApi::class)
@Composable
fun LoopDock(side: Int, t: PadTheme, modifier: Modifier = Modifier) {
    LoopTicker()
    val v = LoopLive.view
    val flat = t.flat
    val ink = if (flat) Color.Black else t.railText
    val bg = if (flat) Color.White else Color.Black.copy(alpha = if (t.name == "Daylight") 0.12f else 0.5f)
    val current = AppState.perf.part(side)
    Column(modifier.background(bg, RoundedCornerShape(14.dp))
               .then(if (flat) Modifier.border(BorderStroke(2.dp, Color.Black), RoundedCornerShape(14.dp)) else Modifier)
               .padding(5.dp),
           horizontalAlignment = Alignment.End) {
        Row(horizontalArrangement = Arrangement.spacedBy(5.dp), verticalAlignment = Alignment.CenterVertically) {
            for ((k, slot) in AppState.slots.withIndex()) SlotButton(k, slot, slot == current, side, t)
            Box(Modifier.width(4.dp))
            RecordButton(v, t)
            DockButton("↶", enabled = v.state != Looper.EMPTY, t) {
                val dropped = if (v.state == Looper.OVERDUBBING) v.layers else v.layers - 1
                Looper.undo()
                if (dropped >= 0) AppState.resetLoopLayers(dropped)
            }
            DockButton(if (v.paused) "▶" else "❚❚", enabled = v.state == Looper.PLAYING, t) { Looper.pause(!v.paused) }
        }
        Text(loopHint(v), color = ink.copy(alpha = 0.8f), fontSize = 11.sp, modifier = Modifier.padding(top = 3.dp, end = 4.dp))
    }
}

@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun SlotButton(k: Int, slot: Part?, active: Boolean, side: Int, t: PadTheme) {
    var menu by remember { mutableStateOf(false) }
    val inst = slot?.let { s -> AppState.instruments.firstOrNull { it.id == s.instrument } }
    val col = if (t.flat) Color.Black else GROUP_COLORS[inst?.group] ?: t.parts[0]
    Box {
        Box(Modifier.size(44.dp)
                .background(when { t.flat && active -> Color.Black; t.flat -> Color.White; slot == null -> Color.Transparent
                                   else -> col.copy(alpha = if (active) 0.45f else 0.18f) }, RoundedCornerShape(10.dp))
                .border(BorderStroke(if (active) 2.dp else 1.dp, if (t.flat) Color.Black else if (active) col else t.railDim.copy(alpha = 0.6f)),
                        RoundedCornerShape(10.dp))
                .combinedClickable(onClick = {
                    if (slot != null) AppState.loadSlot(k, side) else AppState.saveSlot(k, AppState.perf.part(side))
                }, onLongClick = { menu = true }),
            contentAlignment = Alignment.Center) {
            Text(inst?.let { shortName(it.name) } ?: "+", fontSize = if (inst == null) 18.sp else 10.sp, maxLines = 2,
                 fontWeight = if (active) FontWeight.Bold else FontWeight.Normal,
                 color = when { t.flat && active -> Color.White; t.flat -> Color.Black; slot == null -> t.railDim; else -> t.railText })
        }
        DropdownMenu(expanded = menu, onDismissRequest = { menu = false }) {
            DropdownMenuItem(text = { Text("Keep the sound playing now here") },
                             onClick = { AppState.saveSlot(k, AppState.perf.part(side)); menu = false })
            if (slot != null) DropdownMenuItem(text = { Text("Empty this slot") }, onClick = { AppState.saveSlot(k, null); menu = false })
        }
    }
}

/** The big button: a ring that fills as the loop goes round; red while recording. */
@Composable
private fun RecordButton(v: Looper.View, t: PadTheme) {
    val red = if (t.flat) Color.Black else Color(0xFFFF5252)
    val green = if (t.flat) Color.Black else Color(0xFF69F0AE)
    val recording = v.state == Looper.RECORDING || v.state == Looper.OVERDUBBING
    val blink = v.state == Looper.ARMED && (System.currentTimeMillis() / 400) % 2 == 0L
    Box(Modifier.size(52.dp).clickable { Looper.press() }, contentAlignment = Alignment.Center) {
        Canvas(Modifier.fillMaxSize()) {
            val sw = 5.dp.toPx()
            val r = size.minDimension / 2 - sw
            val c = Offset(size.width / 2, size.height / 2)
            drawCircle(if (t.flat) Color.White else Color.Black.copy(alpha = 0.35f), radius = r + sw / 2, center = c)
            drawCircle((if (t.flat) Color.Black else t.railDim).copy(alpha = 0.5f), radius = r, center = c, style = Stroke(sw / 2))
            val ring = if (recording) red else green
            if (v.length > 0) drawArc(ring, -90f, 360f * v.progress, false, topLeft = Offset(c.x - r, c.y - r), size = Size(2 * r, 2 * r),
                                      style = Stroke(sw))
            else if (v.state == Looper.RECORDING) drawCircle(red, radius = r, center = c, style = Stroke(sw))
            // The dot: red to record, hollow while armed, a square to stop the first loop.
            when {
                v.state == Looper.RECORDING -> drawRect(red, Offset(c.x - r * 0.35f, c.y - r * 0.35f), Size(r * 0.7f, r * 0.7f))
                v.state == Looper.ARMED -> drawCircle(red, radius = r * 0.42f, center = c, style = if (blink) Stroke(sw / 2) else androidx.compose.ui.graphics.drawscope.Fill)
                else -> drawCircle(red, radius = r * 0.42f, center = c)
            }
        }
        if (v.layers > 0) Text("${v.layers}", fontSize = 10.sp, fontWeight = FontWeight.Bold,
                               color = if (t.flat) Color.Black else t.railText, modifier = Modifier.align(Alignment.BottomEnd))
    }
}

@Composable
private fun DockButton(label: String, enabled: Boolean, t: PadTheme, click: () -> Unit) {
    Box(Modifier.size(40.dp).background(if (t.flat) Color.White else Color.Black.copy(alpha = 0.3f), CircleShape)
            .border(BorderStroke(1.dp, if (t.flat) Color.Black else t.railDim.copy(alpha = 0.6f)), CircleShape)
            .then(if (enabled) Modifier.clickable(onClick = click) else Modifier),
        contentAlignment = Alignment.Center) {
        Text(label, fontSize = 16.sp, color = (if (t.flat) Color.Black else t.railText).copy(alpha = if (enabled) 1f else 0.35f))
    }
}

/** The Loop panel on the pad's rail: the buttons on or off, snap, tempo, and each layer. */
@Composable
fun LoopControls(accent: Color) {
    LoopTicker()
    val v = LoopLive.view
    var confirmClear by remember { mutableStateOf(false) }
    LaunchedEffect(confirmClear) { if (confirmClear) { delay(3000); confirmClear = false } }
    Text("Record a loop, then play over it with other sounds: tap a sound slot, tap ● and play one time round. " +
         "Each pass is a layer you can mute or turn down.", color = TextDim, fontSize = 12.sp)
    SwitchRow("Loop buttons on the pad", AppState.loopDock, "Sound slots, record, undo and pause, at the top right", accent = accent) {
        AppState.changeLoopDock(it) }
    SwitchRow("Snap the length to beats", AppState.loopSnap, "The loop is a whole number of beats at the tempo, so the arpeggiator and the delay stay in time",
              accent = accent) { AppState.changeLoopSnap(it) }
    val bpm = AppState.perf.bpm
    LabeledSlider("Tempo", bpm, 40f..220f, "${bpm.roundToInt()} bpm", accent = accent) { x ->
        AppState.update { it.copy(bpm = x.roundToInt().toFloat()) } }
    Text(loopHint(v), fontSize = 14.sp, fontWeight = FontWeight.SemiBold, modifier = Modifier.padding(top = 8.dp))
    Row(Modifier.fillMaxWidth().padding(vertical = 4.dp), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
        TextButton(onClick = { Looper.press() }) { Text(when (v.state) {
            Looper.EMPTY -> "Record"; Looper.ARMED -> "Cancel"; Looper.RECORDING -> "Close the loop"
            Looper.OVERDUBBING -> "Stop the layer"; else -> "Add a layer" }) }
        TextButton(onClick = {
            val dropped = if (v.state == Looper.OVERDUBBING) v.layers else v.layers - 1
            Looper.undo(); if (dropped >= 0) AppState.resetLoopLayers(dropped)
        }, enabled = v.state != Looper.EMPTY) { Text("Undo") }
        TextButton(onClick = {
            if (confirmClear) { Looper.clear(); AppState.resetLoopLayers(); confirmClear = false } else confirmClear = true
        }, enabled = v.state != Looper.EMPTY) { Text(if (confirmClear) "Tap again to clear" else "Clear", color = if (confirmClear) Warn else Color.Unspecified) }
    }
    for (i in 0 until v.layers) {
        val (vol, muted) = AppState.loopLayers[i]
        Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.padding(top = 4.dp)) {
            Text("Layer ${i + 1}", fontSize = 14.sp, modifier = Modifier.width(70.dp))
            Canvas(Modifier.width(40.dp).height(8.dp)) {               // how loud it was recorded
                drawRect(TextDim.copy(alpha = 0.3f), size = size)
                drawRect(if (muted) TextDim else accent, size = Size(size.width * v.level[i].coerceIn(0f, 1f), size.height))
            }
            TextButton(onClick = { AppState.changeLoopLayer(i, vol, !muted) }) {
                Text(if (muted) "Muted" else "Mute", color = if (muted) Warn else Color.Unspecified) }
        }
        LabeledSlider("Volume", vol, 0f..1.5f, pct(vol), accent = accent) { x -> AppState.changeLoopLayer(i, x, muted) }
    }
}
