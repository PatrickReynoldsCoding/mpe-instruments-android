package com.patrickreynolds.mpeinstruments.ui

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.ModalBottomSheet
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.Native
import com.patrickreynolds.mpeinstruments.Stats
import com.patrickreynolds.mpeinstruments.noteName
import kotlin.math.roundToInt

@Composable
fun PlayScreen(stats: Stats?, openPad: () -> Unit) {
    Column(Modifier.verticalScroll(rememberScrollState()).padding(bottom = 16.dp)) {
        PadCard(openPad)
        OctaveWarning(stats)
        val perf = AppState.perf
        val parts = if (perf.layout == 0) listOf(0) else listOf(0, 1)
        SectionCard(if (parts.size == 1) "Instrument" else "Instruments") {
            for (k in parts) InstrumentRow(k, label = partLabel(k, perf.layout))
        }
        SectionCard("Play") {
            LabeledSlider("Volume", AppState.volume, 0f..1f, pct(AppState.volume)) { AppState.changeVolume(it) }
            TransposeRow()
            Spacer(Modifier.height(8.dp))
            Button(onClick = { Native.nativePanic() }, modifier = Modifier.fillMaxWidth(),
                   colors = ButtonDefaults.buttonColors(containerColor = PanelHigh, contentColor = Fg), border = ButtonEdge) {
                Text("Panic: stop every note")
            }
        }
    }
}

/** The way into the fullscreen touch pad. */
@Composable
private fun PadCard(open: () -> Unit) {
    Card(Modifier.fillMaxWidth().padding(16.dp).height(150.dp).clickable(onClick = open), shape = RoundedCornerShape(22.dp),
         colors = CardDefaults.cardColors(containerColor = Panel),
         border = if (Eink) androidx.compose.foundation.BorderStroke(Edge, Line) else null) {
        Box(Modifier.fillMaxSize()) {
            Canvas(Modifier.fillMaxSize()) {
                if (!Eink) drawRect(Brush.linearGradient(listOf(Color(0xFF2A1F3D), Color(0xFF0E1014), Color(0xFF0F2A30))))
                for (i in 0 until 8) {
                    val x = (i + 0.5f) * size.width / 8
                    drawLine(if (Eink) Color.Black.copy(alpha = 0.3f) else Color.White.copy(alpha = 0.08f),
                             Offset(x, 0f), Offset(x, size.height), strokeWidth = 2f)
                }
                if (!Eink) for ((k, c) in PartColors.withIndex()) {
                    val at = Offset(size.width * (0.62f + 0.2f * k), size.height * (0.38f + 0.25f * k))
                    drawCircle(Brush.radialGradient(listOf(c.copy(alpha = 0.6f), c.copy(alpha = 0f)), center = at, radius = 90f), 90f, at)
                    drawCircle(Color.White.copy(alpha = 0.9f), 10f, at)
                }
            }
            Column(Modifier.align(Alignment.CenterStart).padding(20.dp)) {
                Text("Touch pad", fontSize = 24.sp, fontWeight = FontWeight.SemiBold)
                Text("Play with your fingers, full screen", color = TextDim, fontSize = 13.sp)
                Spacer(Modifier.height(10.dp))
                Button(onClick = open, colors = ButtonDefaults.buttonColors(containerColor = PartColors[0], contentColor = Ink)) {
                    Text("Play")
                }
            }
        }
    }
}

fun partLabel(k: Int, layout: Int) = when (layout) { 1 -> if (k == 0) "Left" else "Right"; 2 -> if (k == 0) "Main" else "Layer"; else -> "" }

@Composable
fun TransposeRow() {
    val t = AppState.transpose
    Row(Modifier.fillMaxWidth().padding(top = 4.dp), verticalAlignment = Alignment.CenterVertically) {
        Text("Transpose", fontSize = 14.sp, modifier = Modifier.weight(1f))
        for ((label, d) in listOf("−oct" to -12, "−1" to -1)) TextButton(onClick = { AppState.changeTranspose(t + d) }) { Text(label) }
        Text(if (t == 0) "0" else "%+d".format(t), modifier = Modifier.width(44.dp), fontWeight = FontWeight.SemiBold,
             textAlign = androidx.compose.ui.text.style.TextAlign.Center)
        for ((label, d) in listOf("+1" to 1, "+oct" to 12)) TextButton(onClick = { AppState.changeTranspose(t + d) }) { Text(label) }
    }
}

/**
 * The Seaboard's notes can come in octaves away from the instrument (its octave buttons, or a
 * setting in ROLI's app): then everything sounds strained or at one pitch. Offer the fix.
 */
@Composable
fun OctaveWarning(stats: Stats?) {
    val inst = AppState.instrument(0) ?: return
    val center = stats?.noteCenter ?: return
    if (center < 0) return
    val playing = center + AppState.transpose + AppState.perf.part(0).transpose
    val octaves = ((inst.centerNote - playing) / 12.0).roundToInt()
    if (kotlin.math.abs(octaves) < 2) return
    Card(Modifier.fillMaxWidth().padding(horizontal = 16.dp, vertical = 6.dp), shape = RoundedCornerShape(18.dp),
         colors = CardDefaults.cardColors(containerColor = Warn.copy(alpha = if (Eink) 0f else 0.14f)),
         border = if (Eink) androidx.compose.foundation.BorderStroke(Edge, Line) else null) {
        Column(Modifier.padding(16.dp)) {
            Text("Your notes are ${kotlin.math.abs(octaves)} octaves ${if (octaves < 0) "above" else "below"} the ${inst.name.lowercase()}",
                 color = Warn, fontWeight = FontWeight.SemiBold)
            Text("Check the octave buttons on the Seaboard, or transpose here.", color = TextDim, fontSize = 13.sp)
            Spacer(Modifier.height(8.dp))
            Button(onClick = { AppState.changeTranspose(AppState.transpose + 12 * octaves) },
                   colors = ButtonDefaults.buttonColors(containerColor = Warn, contentColor = Ink)) {
                Text("Transpose ${if (octaves > 0) "+" else "−"}${kotlin.math.abs(octaves)} octaves")
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun InstrumentRow(part: Int, label: String) {
    var open by remember { mutableStateOf(false) }
    val inst = AppState.instrument(part)
    Row(Modifier.fillMaxWidth().clickable { open = true }.padding(vertical = 8.dp), verticalAlignment = Alignment.CenterVertically) {
        Canvas(Modifier.width(10.dp).height(36.dp)) { drawRoundRect(PartColors[part], cornerRadius = CornerRadius(8f)) }
        Spacer(Modifier.width(12.dp))
        Column(Modifier.weight(1f)) {
            if (label.isNotEmpty()) Text(label, color = TextDim, fontSize = 12.sp)
            Text(inst?.name ?: "?", fontSize = 18.sp, fontWeight = FontWeight.SemiBold)
            Text(inst?.description ?: "", color = TextDim, fontSize = 12.sp, maxLines = 1)
        }
        Text("Change", color = PartColors[part], fontSize = 13.sp)
    }
    if (open) ModalBottomSheet(onDismissRequest = { open = false }, containerColor = Panel) {
        LazyColumn(Modifier.padding(bottom = 24.dp)) {
            val groups = AppState.instruments.groupBy { it.group }
            for ((group, list) in groups) {
                item { Text(group.uppercase(), color = TextDim, fontSize = 12.sp, letterSpacing = 1.2.sp,
                            modifier = Modifier.padding(start = 20.dp, top = 14.dp, bottom = 4.dp)) }
                for (i in list) item {
                    val chosen = i.id == inst?.id
                    Column(Modifier.fillMaxWidth().clickable { AppState.updatePart(part) { it.copy(instrument = i.id) }; open = false }
                               .padding(horizontal = 20.dp, vertical = 10.dp)) {
                        Text(i.name, fontSize = 17.sp, color = if (chosen) PartColors[part] else Color.Unspecified,
                             fontWeight = if (chosen) FontWeight.SemiBold else FontWeight.Normal)
                        Text(i.description, color = TextDim, fontSize = 13.sp)
                    }
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun PresetSheet(onClose: () -> Unit) {
    var naming by remember { mutableStateOf(false) }
    var name by remember { mutableStateOf("") }
    ModalBottomSheet(onDismissRequest = onClose, containerColor = Panel) {
        Column(Modifier.padding(horizontal = 20.dp).padding(bottom = 28.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text("Presets", fontSize = 22.sp, fontWeight = FontWeight.SemiBold, modifier = Modifier.weight(1f))
                OutlinedButton(onClick = { naming = true }) { Text("Save current") }
            }
            if (naming) {
                Row(Modifier.padding(top = 10.dp), verticalAlignment = Alignment.CenterVertically) {
                    androidx.compose.material3.OutlinedTextField(value = name, onValueChange = { name = it }, singleLine = true,
                        label = { Text("Name") }, modifier = Modifier.weight(1f))
                    TextButton(onClick = { AppState.saveAs(name); naming = false; name = "" }) { Text("Save") }
                }
            }
            Spacer(Modifier.height(10.dp))
            LazyColumn {
                val list = AppState.allPresets
                for ((i, p) in list.withIndex()) item {
                    if (i == AppState.allPresets.count { it.builtIn } && i > 0) {
                        HorizontalDivider(color = Line)
                        Text("YOURS", color = TextDim, fontSize = 12.sp, letterSpacing = 1.2.sp, modifier = Modifier.padding(vertical = 8.dp))
                    }
                    Row(Modifier.fillMaxWidth().clickable { AppState.load(p); onClose() }.padding(vertical = 12.dp),
                        verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                        val current = p.name == AppState.presetName
                        Column(Modifier.weight(1f)) {
                            Text(p.name, fontSize = 17.sp, color = if (current) PartColors[0] else Color.Unspecified,
                                 fontWeight = if (current) FontWeight.SemiBold else FontWeight.Normal)
                            Text(presetSummary(p.perf), color = TextDim, fontSize = 12.sp, maxLines = 1)
                        }
                        if (!p.builtIn) TextButton(onClick = { AppState.delete(p) }) { Text("Delete", color = TextDim) }
                    }
                }
            }
        }
    }
}

private fun presetSummary(p: com.patrickreynolds.mpeinstruments.Perf): String {
    fun name(id: String) = AppState.instruments.firstOrNull { it.id == id }?.name ?: id
    val a = p.part(0); val b = p.part(1)
    fun desc(x: com.patrickreynolds.mpeinstruments.Part) = name(x.instrument) + if (x.arp) " arp" else ""
    return when (p.layout) {
        1 -> "${desc(a)} | ${desc(b)}, split at ${noteName(p.splitNote)}"
        2 -> "${desc(a)} + ${desc(b)}"
        else -> desc(a)
    } + if (p.scale != 0) ", in key" else ""
}
