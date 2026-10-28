package com.patrickreynolds.mpeinstruments

import android.Manifest
import android.bluetooth.BluetoothDevice
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.content.pm.ActivityInfo
import android.os.Bundle
import android.view.WindowManager
import androidx.activity.compose.BackHandler
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationBarItemDefaults
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.patrickreynolds.mpeinstruments.ui.Ink
import com.patrickreynolds.mpeinstruments.ui.MpeIcons
import com.patrickreynolds.mpeinstruments.ui.MpeTheme
import com.patrickreynolds.mpeinstruments.ui.Panel
import com.patrickreynolds.mpeinstruments.ui.PadScreen
import com.patrickreynolds.mpeinstruments.ui.PartColors
import com.patrickreynolds.mpeinstruments.ui.PerformScreen
import com.patrickreynolds.mpeinstruments.ui.PlayScreen
import com.patrickreynolds.mpeinstruments.ui.PresetSheet
import com.patrickreynolds.mpeinstruments.ui.SetupActions
import com.patrickreynolds.mpeinstruments.ui.SetupScreen
import com.patrickreynolds.mpeinstruments.ui.SoundScreen
import com.patrickreynolds.mpeinstruments.ui.TextDim
import com.patrickreynolds.mpeinstruments.ui.Warn
import kotlinx.coroutines.delay

/** Four tabs: Play (the way into the fullscreen touch pad), Perform (split, arp, tempo, scale), Sound (effects), Setup. */
class MainActivity : ComponentActivity() {
    private var ble: BleScanner? = null
    private val bleFound = mutableStateListOf<BluetoothDevice>()
    private var scanning by mutableStateOf(false)
    private var midiTick by mutableIntStateOf(0)            // bumped when devices come, go or connect
    private var padOpen by mutableStateOf(false)

    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge()
        super.onCreate(savedInstanceState)
        requestNeededPermissions()
        startForegroundService(Intent(this, SynthService::class.java))
        MidiConnector.init(applicationContext)
        AppState.init(applicationContext)
        MidiConnector.onChange = { runOnUiThread { midiTick++ } }
        SynthService.onStopped = { runOnUiThread { finishAndRemoveTask() } }
        setContent { MpeTheme { App() } }
    }

    override fun onPause() { ble?.stop(); scanning = false; super.onPause() }

    private fun requestNeededPermissions() {
        val want = mutableListOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
        if (Build.VERSION.SDK_INT >= 33) want += Manifest.permission.POST_NOTIFICATIONS
        val missing = want.filter { checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED }
        if (missing.isNotEmpty()) requestPermissions(missing.toTypedArray(), 1)
    }

    private fun scanBle() {
        if (checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) { requestNeededPermissions(); return }
        val scanner = ble ?: BleScanner(this) { d -> runOnUiThread { if (bleFound.none { it.address == d.address }) bleFound += d } }.also { ble = it }
        if (!scanner.available) return
        scanner.start()
        scanning = true
    }

    /** The touch pad takes the whole screen: landscape, no system bars, screen kept on. */
    private fun showPad(open: Boolean) {
        padOpen = open
        val bars = WindowCompat.getInsetsController(window, window.decorView)
        if (open) {
            requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE
            bars.systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
            bars.hide(WindowInsetsCompat.Type.systemBars())
            window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        } else {
            requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED
            bars.show(WindowInsetsCompat.Type.systemBars())
            window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        }
    }

    private fun stopApp() {
        stopService(Intent(this, SynthService::class.java))
        finishAndRemoveTask()
    }

    @OptIn(ExperimentalMaterial3Api::class)
    @Composable
    private fun App() {
        var tab by remember { mutableIntStateOf(0) }
        var stats by remember { mutableStateOf<Stats?>(null) }
        var presets by remember { mutableStateOf(false) }
        LaunchedEffect(Unit) { while (true) { stats = Stats.from(Native.nativeStats()); delay(250) } }
        // Split > Learn: the next note played sets the split point (the engine splits on the note as sent).
        LaunchedEffect(AppState.learningSplit) {
            val buf = FloatArray(16 * 5)
            while (AppState.learningSplit) {
                if (Native.nativeFingers(buf) > 0) {
                    AppState.update { it.copy(splitNote = buf[1].toInt()) }
                    AppState.learningSplit = false
                }
                delay(15)
            }
        }
        if (padOpen) {
            BackHandler { showPad(false) }
            PadScreen { showPad(false) }
            return
        }
        val tick = midiTick
        Scaffold(
            containerColor = Ink,
            topBar = {
                TopAppBar(
                    colors = TopAppBarDefaults.topAppBarColors(containerColor = Ink),
                    title = {
                        Column {
                            Text(AppState.presetName.ifEmpty { "MPE Instruments" }, fontWeight = FontWeight.SemiBold, fontSize = 20.sp)
                            Status(stats, tick)
                        }
                    },
                    actions = { TextButton(onClick = { presets = true }) { Text("Presets", color = PartColors[0]) } },
                )
            },
            bottomBar = {
                NavigationBar(containerColor = Panel) {
                    val items = listOf(
                        "Play" to Icons.Filled.PlayArrow, "Perform" to MpeIcons.Arp,
                        "Sound" to MpeIcons.Sliders, "Setup" to Icons.Filled.Settings)
                    items.forEachIndexed { i, (label, icon) ->
                        NavigationBarItem(selected = tab == i, onClick = { tab = i }, icon = { Icon(icon, label) }, label = { Text(label) },
                            colors = NavigationBarItemDefaults.colors(selectedIconColor = PartColors[0], selectedTextColor = PartColors[0],
                                                                      indicatorColor = PartColors[0].copy(alpha = 0.16f)))
                    }
                }
            },
        ) { pad ->
            Box(Modifier.padding(pad).fillMaxSize()) {
                when (tab) {
                    0 -> PlayScreen(stats) { showPad(true) }
                    1 -> PerformScreen()
                    2 -> SoundScreen()
                    else -> SetupScreen(stats, SetupActions(bleFound, scanning, ::scanBle, ::stopApp), tick)
                }
            }
        }
        if (presets) PresetSheet { presets = false }
    }

    @Composable
    private fun Status(stats: Stats?, @Suppress("UNUSED_PARAMETER") tick: Int) {
        val name = MidiConnector.connectedName
        val waiting = MidiConnector.reconnecting
        val (color, text) = when {
            name.isNotEmpty() -> PartColors[1] to name
            waiting.isNotEmpty() -> Warn to "Waiting for $waiting"
            else -> TextDim to "No controller"
        }
        Row(verticalAlignment = Alignment.CenterVertically) {
            Box(Modifier.size(8.dp).clip(CircleShape).background(color))
            Text("  $text", color = TextDim, fontSize = 12.sp, maxLines = 1)
            val lat = stats?.let { if (it.events > 0) it.totalMs else it.outputLatencyMs }
            if (lat != null && lat > 0) Text("  ·  ${lat.toInt()} ms", color = TextDim, fontSize = 12.sp)
        }
    }
}
