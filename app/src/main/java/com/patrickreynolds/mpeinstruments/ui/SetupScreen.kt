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

/**
 * Settings: what belongs to the app and the phone rather than to a controller or a preset. The
 * MIDI devices are here as well as on External MIDI's rail: a device can play beside any
 * controller.
 */
@Composable
fun SetupScreen(stats: Stats?, actions: SetupActions, @Suppress("UNUSED_PARAMETER") midiTick: Int) {
    Column(Modifier.verticalScroll(rememberScrollState()).padding(vertical = 10.dp)) {
        SectionCard("MIDI devices") {
            MidiDevices()
            LinkWatchdog()
        }
        SectionCard("Look") {
            Segmented(listOf("Dark", "E-ink"), if (AppState.eink) 1 else 0) { AppState.changeEink(it == 1) }
            Text("E-ink: black on white, thick borders, no colour, glow or motion, and the touch pad only redraws when "
                 + "something changes. For e-ink phones such as the Hisense A9.", color = TextDim, fontSize = 12.sp,
                 modifier = Modifier.padding(top = 6.dp))
        }
        ExperimentalCard()
        SectionCard("Latency") { LatencyPanel(stats) }
        UpdatesCard()
        SectionCard("App") {
            Button(onClick = actions.stop, modifier = Modifier.fillMaxWidth(),
                   colors = ButtonDefaults.buttonColors(containerColor = PanelHigh, contentColor = Danger), border = ButtonEdge) {
                Text("Stop: sound and controller off until you open the app")
            }
            Text("odo ${BuildConfig.VERSION_NAME}", color = TextDim, fontSize = 12.sp, modifier = Modifier.padding(top = 10.dp))
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
