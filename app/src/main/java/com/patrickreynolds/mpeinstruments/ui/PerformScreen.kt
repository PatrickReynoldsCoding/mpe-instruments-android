package com.patrickreynolds.mpeinstruments.ui

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.patrickreynolds.mpeinstruments.ARP_ORDERS
import com.patrickreynolds.mpeinstruments.ARP_RATES
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.DELAY_TIMES
import com.patrickreynolds.mpeinstruments.NOTE_NAMES
import com.patrickreynolds.mpeinstruments.SCALES
import com.patrickreynolds.mpeinstruments.noteName
import kotlin.math.roundToInt

@Composable
fun PerformScreen() {
    val perf = AppState.perf
    Column(Modifier.verticalScroll(rememberScrollState()).padding(vertical = 10.dp)) {
        SectionCard("Layout") {
            Segmented(listOf("Single", "Split", "Layer"), perf.layout) { i -> AppState.update { it.copy(layout = i) } }
            when (perf.layout) {
                1 -> {
                    Spacer(Modifier.height(10.dp))
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Text("Split at", modifier = Modifier.weight(1f))
                        TextButton(onClick = { AppState.update { it.copy(splitNote = it.splitNote - 1) } }) { Text("−") }
                        Text(noteName(perf.splitNote), fontWeight = FontWeight.SemiBold, modifier = Modifier.width(48.dp),
                             textAlign = androidx.compose.ui.text.style.TextAlign.Center)
                        TextButton(onClick = { AppState.update { it.copy(splitNote = it.splitNote + 1) } }) { Text("+") }
                        OutlinedButton(onClick = { AppState.learningSplit = !AppState.learningSplit }) {
                            Text(if (AppState.learningSplit) "Play a key…" else "Learn")
                        }
                    }
                    Text("Left of the split plays the left part, the rest the right. A finger keeps its part when it glides across.",
                         color = TextDim, fontSize = 12.sp)
                }
                2 -> Text("Every finger plays both parts. Give the layer its own octave and volume below.",
                          color = TextDim, fontSize = 12.sp, modifier = Modifier.padding(top = 8.dp))
                else -> {}
            }
        }
        val parts = if (perf.layout == 0) listOf(0) else listOf(0, 1)
        for (k in parts) PartCard(k)
        TempoCard()
        ScaleCard()
    }
}

@Composable
private fun PartCard(k: Int) {
    val perf = AppState.perf
    val p = perf.part(k)
    val c = PartColors[k]
    val title = partLabel(k, perf.layout).ifEmpty { "Playing" }
    SectionCard(title, accent = c) {
        if (perf.layout != 0) InstrumentRow(k, label = "")
        Segmented(listOf("Play", "Arpeggiate"), if (p.arp) 1 else 0, accent = c) { i -> AppState.updatePart(k) { it.copy(arp = i == 1) } }
        if (p.arp) {
            Spacer(Modifier.height(12.dp))
            Text("Order", color = TextDim, fontSize = 13.sp)
            Chips(ARP_ORDERS, p.order, accent = c) { i -> AppState.updatePart(k) { it.copy(order = i) } }
            Text("Rate", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 6.dp))
            Chips(ARP_RATES.map { it.first }, ARP_RATES.indexOfFirst { it.second == p.rate }, accent = c) { i ->
                AppState.updatePart(k) { it.copy(rate = ARP_RATES[i].second) }
            }
            LabeledSlider("Gate", p.gate, 0.05f..1f, pct(p.gate), accent = c) { v -> AppState.updatePart(k) { it.copy(gate = v) } }
            Text("Octaves", color = TextDim, fontSize = 13.sp)
            Segmented(listOf("1", "2", "3", "4"), p.octaves - 1, accent = c) { i -> AppState.updatePart(k) { it.copy(octaves = i + 1) } }
            SwitchRow("Latch", p.latch, "Keep playing after you lift your hand; a new chord replaces it", accent = c) { v ->
                AppState.updatePart(k) { it.copy(latch = v) }
            }
            Text("Each arp note follows the finger that holds it: press to swell, slide to change the tone, wiggle for vibrato.",
                 color = TextDim, fontSize = 12.sp)
        }
        Spacer(Modifier.height(8.dp))
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("Octave", modifier = Modifier.weight(1f))
            TextButton(onClick = { AppState.updatePart(k) { it.copy(transpose = it.transpose - 12) } }) { Text("−") }
            Text(if (p.transpose == 0) "0" else "%+d".format(p.transpose / 12), fontWeight = FontWeight.SemiBold)
            TextButton(onClick = { AppState.updatePart(k) { it.copy(transpose = it.transpose + 12) } }) { Text("+") }
        }
        LabeledSlider("Level", p.volume, 0f..1.5f, pct(p.volume), accent = c) { v -> AppState.updatePart(k) { it.copy(volume = v) } }
    }
}

@Composable
private fun TempoCard() {
    val perf = AppState.perf
    val taps = remember { mutableStateListOf<Long>() }
    SectionCard("Tempo", trailing = {
        Text("${perf.bpm.roundToInt()} bpm", fontWeight = FontWeight.SemiBold, fontSize = 16.sp)
    }) {
        LabeledSlider("For the arpeggiator and the delay", perf.bpm, 40f..220f, "") { v -> AppState.update { it.copy(bpm = v.roundToInt().toFloat()) } }
        Button(onClick = {
            val now = System.currentTimeMillis()
            if (taps.isNotEmpty() && now - taps.last() > 2000) taps.clear()
            taps.add(now)
            while (taps.size > 6) taps.removeAt(0)
            if (taps.size >= 3) {
                val bpm = 60000.0 * (taps.size - 1) / (taps.last() - taps.first())
                AppState.update { it.copy(bpm = bpm.roundToInt().coerceIn(40, 220).toFloat()) }
            }
        }, modifier = Modifier.fillMaxWidth(), colors = ButtonDefaults.buttonColors(containerColor = PanelHigh, contentColor = PartColors[0]), border = ButtonEdge) {
            Text("Tap tempo")
        }
    }
}

@Composable
private fun ScaleCard() {
    val perf = AppState.perf
    SectionCard("Pitch assist") {
        Text("When a finger comes to rest it is drawn onto the nearest note of the scale. Glides and vibrato are left alone.",
             color = TextDim, fontSize = 12.sp)
        Spacer(Modifier.height(8.dp))
        Chips(SCALES.map { it.name }, perf.scale) { i -> AppState.update { it.copy(scale = i) } }
        if (perf.scale != 0) {
            Text("Key", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
            Chips(NOTE_NAMES, perf.scaleRoot) { i -> AppState.update { it.copy(scaleRoot = i) } }
            LabeledSlider("Strength", perf.assist, 0f..1f, pct(perf.assist)) { v -> AppState.update { it.copy(assist = v) } }
        }
    }
}

@Composable
fun SoundScreen() {
    val perf = AppState.perf
    Column(Modifier.verticalScroll(rememberScrollState()).padding(vertical = 10.dp)) {
        val parts = if (perf.layout == 0) listOf(0) else listOf(0, 1)
        for (k in parts) {
            val p = perf.part(k)
            val c = PartColors[k]
            val name = AppState.instrument(k)?.name ?: ""
            SectionCard(listOf(partLabel(k, perf.layout), name).filter { it.isNotEmpty() }.joinToString(" · "), accent = c) {
                LabeledSlider("Brightness", p.brightness, 0f..1f, if (p.brightness >= 0.999f) "open" else pct(p.brightness), accent = c) { v ->
                    AppState.updatePart(k) { it.copy(brightness = v) }
                }
                LabeledSlider("Drive", p.drive, 0f..1f, if (p.drive < 0.01f) "clean" else pct(p.drive), accent = c) { v ->
                    AppState.updatePart(k) { it.copy(drive = v) }
                }
                // Slider left = sensitive (curve < 1), right = firm (curve > 1), on a log scale.
                val pos = (kotlin.math.ln(p.pressureCurve) / kotlin.math.ln(3f)).coerceIn(-1f, 1f)
                LabeledSlider("Touch", pos, -1f..1f,
                              when { pos < -0.15f -> "light"; pos > 0.15f -> "firm"; else -> "even" }, accent = c) { v ->
                    AppState.updatePart(k) { it.copy(pressureCurve = Math.pow(3.0, v.toDouble()).toFloat()) }
                }
                LabeledSlider("Reverb", p.reverb, 0f..1f, pct(p.reverb), accent = c) { v -> AppState.updatePart(k) { it.copy(reverb = v) } }
                LabeledSlider("Delay", p.delay, 0f..1f, pct(p.delay), accent = c) { v -> AppState.updatePart(k) { it.copy(delay = v) } }
            }
        }
        SectionCard("Reverb") {
            LabeledSlider("Size", perf.reverbSize, 0f..1f, pct(perf.reverbSize)) { v -> AppState.update { it.copy(reverbSize = v) } }
            LabeledSlider("Damping", perf.reverbDamp, 0f..1f, pct(perf.reverbDamp)) { v -> AppState.update { it.copy(reverbDamp = v) } }
        }
        SectionCard("Delay") {
            Text("Time, in beats of the tempo (${perf.bpm.roundToInt()} bpm)", color = TextDim, fontSize = 13.sp)
            Chips(DELAY_TIMES.map { it.first }, DELAY_TIMES.indexOfFirst { it.second == perf.delayBeats }) { i ->
                AppState.update { it.copy(delayBeats = DELAY_TIMES[i].second) }
            }
            LabeledSlider("Feedback", perf.delayFeedback, 0f..0.9f, pct(perf.delayFeedback)) { v -> AppState.update { it.copy(delayFeedback = v) } }
            SwitchRow("Ping-pong", perf.delayPingPong, "Echoes bounce between left and right") { v -> AppState.update { it.copy(delayPingPong = v) } }
        }
    }
}
