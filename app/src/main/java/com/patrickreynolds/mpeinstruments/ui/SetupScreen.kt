package com.patrickreynolds.mpeinstruments.ui

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.BuildConfig
import com.patrickreynolds.mpeinstruments.MidiConnector
import com.patrickreynolds.mpeinstruments.Native
import com.patrickreynolds.mpeinstruments.Stats

/** What the Setup tab needs from the activity (Bluetooth scanning lives there). */
class SetupActions(
    val bleFound: List<android.bluetooth.BluetoothDevice>,
    val scanning: Boolean,
    val scan: () -> Unit,
    val stop: () -> Unit,
)

@Composable
fun SetupScreen(stats: Stats?, actions: SetupActions, @Suppress("UNUSED_PARAMETER") midiTick: Int) {
    Column(Modifier.verticalScroll(rememberScrollState()).padding(vertical = 10.dp)) {
        SectionCard("Controller") {
            val connected = MidiConnector.connectedName
            val waiting = MidiConnector.reconnecting
            Text(when {
                connected.isNotEmpty() -> connected
                waiting.isNotEmpty() -> "Waiting for $waiting (Bluetooth)…"
                else -> "Nothing connected"
            }, fontSize = 17.sp, fontWeight = FontWeight.SemiBold, color = if (connected.isNotEmpty()) PartColors[1] else Color.Unspecified)
            if (connected.isNotEmpty()) OutlinedButton(onClick = { MidiConnector.disconnect() }, modifier = Modifier.padding(top = 6.dp)) {
                Text("Disconnect")
            }
            for (info in MidiConnector.devices().filter { it.id != MidiConnector.connectedId })
                OutlinedButton(onClick = { MidiConnector.connect(info) }, modifier = Modifier.fillMaxWidth().padding(top = 6.dp)) {
                    Text("Connect ${MidiConnector.name(info)}")
                }
            for (d in actions.bleFound)
                OutlinedButton(onClick = { MidiConnector.connectBluetooth(d) }, modifier = Modifier.fillMaxWidth().padding(top = 6.dp)) {
                    Text("Connect ${d.name ?: d.address} (Bluetooth)")
                }
            Button(onClick = actions.scan, modifier = Modifier.fillMaxWidth().padding(top = 8.dp),
                   colors = ButtonDefaults.buttonColors(containerColor = PanelHigh, contentColor = PartColors[1])) {
                Text(if (actions.scanning) "Looking for Bluetooth controllers…" else "Find a Bluetooth controller")
            }
            Text("USB: plug the Seaboard in and it connects by itself. Bluetooth: switch the Seaboard on, then find it here once.",
                 color = TextDim, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp))
            SwitchRow("Reconnect by itself", MidiConnector.autoBluetooth,
                      "Plays the moment the Bluetooth controller is on, also with the app in the background", accent = PartColors[1]) {
                MidiConnector.autoBluetooth = it
            }
        }
        SectionCard("Controller settings") {
            val ranges = listOf(48f, 24f, 12f, 2f)
            Text("Pitch bend range (the Seaboard sends ±48)", color = TextDim, fontSize = 13.sp)
            Chips(ranges.map { "±${it.toInt()}" }, ranges.indexOf(AppState.bendRange)) { AppState.changeBendRange(ranges[it]) }
            val dogs = listOf(-1, 0, 500, 1500, 3000)
            Text("Release notes when the link goes silent", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
            Chips(dogs.map { when (it) { -1 -> "Auto"; 0 -> "Off"; else -> "${it} ms" } }, dogs.indexOf(MidiConnector.watchdogSetting)) {
                MidiConnector.watchdogSetting = dogs[it]
            }
            Text("Auto: 1.5 s on Bluetooth, where a lost link is not reported for up to 30 s; off on USB.",
                 color = TextDim, fontSize = 12.sp)
        }
        SectionCard("Latency") { LatencyPanel(stats) }
        SectionCard("App") {
            Button(onClick = actions.stop, modifier = Modifier.fillMaxWidth(),
                   colors = ButtonDefaults.buttonColors(containerColor = PanelHigh, contentColor = Color(0xFFFF6B6B))) {
                Text("Stop: sound and controller off until you open the app")
            }
            Text("MPE Instruments ${BuildConfig.VERSION_NAME}", color = TextDim, fontSize = 12.sp, modifier = Modifier.padding(top = 10.dp))
        }
    }
}

@Composable
private fun LatencyPanel(s: Stats?) {
    if (s == null) { Text("…"); return }
    val ms = { v: Double -> if (v.isNaN() || v < 0) "n/a" else "%.1f ms".format(v) }
    val played = s.events > 0
    Row(verticalAlignment = Alignment.Bottom) {
        Text(if (played) ms(s.totalMs) else ms(s.outputLatencyMs), fontSize = 34.sp, fontWeight = FontWeight.SemiBold)
        Text(if (played) "  note → sound" else "  synth → speaker", color = TextDim, modifier = Modifier.padding(bottom = 6.dp))
    }
    Spacer(Modifier.height(8.dp))
    val rows = listOf(
        "MIDI → synth" to if (played) "avg ${ms(s.eventAgeAvgMs)}, max ${ms(s.eventAgeMaxMs)}" else "play a note",
        "synth → speaker" to ms(s.outputLatencyMs),
        "buffer" to "${s.bufferFrames} frames (${ms(1000.0 * s.bufferFrames / s.sampleRate.coerceAtLeast(1))}), burst ${s.burstFrames}",
        "audio path" to "${s.pathName}, ${if (s.exclusive) "exclusive" else "shared"}, ${s.sampleRate} Hz",
        "underruns" to "${s.xruns}   restarts ${s.streamRestarts}",
        "cpu" to "avg %.1f%%, peak %.1f%%".format(s.loadAvg * 100, s.loadPeak * 100),
        "voices" to "${s.voices}",
        "events" to "${s.events}, dropped ${s.dropped}, panics ${s.panics}",
    )
    for ((k, v) in rows) Row(Modifier.padding(vertical = 2.dp)) {
        Text(k, color = TextDim, fontSize = 13.sp, modifier = Modifier.weight(0.42f))
        Text(v, fontSize = 13.sp, fontFamily = FontFamily.Monospace, modifier = Modifier.weight(0.58f))
    }
    OutlinedButton(onClick = { Native.nativeResetLatencyTuner() }, modifier = Modifier.padding(top = 8.dp)) { Text("Reset latency tuner") }
}
