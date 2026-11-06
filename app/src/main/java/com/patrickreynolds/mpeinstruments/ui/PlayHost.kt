package com.patrickreynolds.mpeinstruments.ui

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowLeft
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowRight
import androidx.compose.material.icons.filled.Close
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.key
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.GyroSettings
import com.patrickreynolds.mpeinstruments.PadSettings
import com.patrickreynolds.mpeinstruments.Part
import com.patrickreynolds.mpeinstruments.PhoneMods
import com.patrickreynolds.mpeinstruments.controllers.Controllers
import com.patrickreynolds.mpeinstruments.controllers.Option
import com.patrickreynolds.mpeinstruments.controllers.TouchPad
import com.patrickreynolds.mpeinstruments.releaseName

/** The rail's colours: the touch pad's look while it is in use, else the default one. */
fun hostTheme() = if (AppState.controller === TouchPad) padTheme(TouchPad.pad) else padTheme(PadSettings())

/**
 * The play screen, the same for every controller: the rail at the left (the controller to play
 * with, its own options, then what every controller shares), the controller's surface filling the
 * rest, and the loop buttons over it.
 */
@Composable
fun PlayHost(onClose: () -> Unit) {
    var panel by remember { mutableStateOf<String?>(null) }
    val c = AppState.controller
    val part = AppState.part
    val theme = hostTheme()
    LoopTicker()
    // The controller's options and the shared ones, built once for the rail and the open panel.
    val own = c.options(part)
    val shared = sharedOptions(part)
    val pickController = Option("Controller", c.name, perPart = false) { ControllerPicker { panel = null } }
    Box(Modifier.fillMaxSize().background(theme.bottom)) {
        Row(Modifier.fillMaxSize()) {
            if (!AppState.railHidden) Rail(pickController, own, shared, theme, panel, { panel = if (panel == it) null else it }, onClose)
            Box(Modifier.weight(1f).fillMaxHeight()) {
                // A new surface for each controller: the old one lets go of its notes as it goes.
                key(c.id) { c.Surface(part) }
                if (AppState.loopDock) LoopDock(theme, Modifier.align(Alignment.TopEnd).padding(6.dp))
            }
        }
        if (!AppState.railHidden) panel?.let { label ->
            (listOf(pickController) + own + shared).firstOrNull { it.label == label && it.panel != null }?.let { o ->
                SettingPanel(o, part, theme) { panel = null }
            }
        }
        // The rail tucked away: one small button in the corner brings it back.
        else Box(Modifier.padding(6.dp).size(44.dp)
                     .background(if (theme.flat) Color.White else theme.rail, RoundedCornerShape(12.dp))
                     .border(BorderStroke(if (theme.flat) 2.dp else 1.dp, if (theme.flat) Color.Black else theme.railDim), RoundedCornerShape(12.dp))
                     .clickable { AppState.changeRailHidden(false) },
                 contentAlignment = Alignment.Center) {
            Icon(Icons.AutoMirrored.Filled.KeyboardArrowRight, "Show the settings", tint = theme.railText)
        }
    }
}

/** What every controller shares: presets, the sound and its effects, release, loops, and the phone's gyro and breath. */
@Composable
private fun sharedOptions(k: Int): List<Option> {
    val part = AppState.perf.part(k)
    val accent = if (Eink) Color.Black else PartColors[k]
    val two = AppState.twoParts
    return listOfNotNull(
        Option("Presets", AppState.presetName.ifEmpty { "None" }, perPart = false) {
            Column(Modifier.verticalScroll(rememberScrollState())) { PresetLibrary(accent) }
        },
        Option("Sound", AppState.instrument(k)?.name ?: "") {
            SoundControls(k, accent)
            LazyColumn {
                for ((group, list) in AppState.instruments.groupBy { it.group }) {
                    item { Text(group.uppercase(), color = TextDim, fontSize = 11.sp, modifier = Modifier.padding(top = 8.dp)) }
                    for (i in list) item {
                        val chosen = i.id == AppState.perf.part(k).instrument
                        Text(i.name, fontSize = 16.sp, color = if (chosen) accent else Color.Unspecified,
                             modifier = Modifier.clickable { AppState.updatePart(k) { it.copy(instrument = i.id) } }
                                 .fillMaxWidth().padding(vertical = 7.dp))
                    }
                }
            }
        },
        Option("Effects", effectsSummary(part)) {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                PartEffects(k, accent)
                LabeledSlider("Level", part.volume, 0f..1.5f, pct(part.volume), accent = accent) { v ->
                    AppState.updatePart(k) { it.copy(volume = v) } }
                Text(if (two) "Reverb (both sounds)" else "Reverb", fontSize = 15.sp, fontWeight = FontWeight.SemiBold,
                     modifier = Modifier.padding(top = 12.dp))
                ReverbControls()
                Text(if (two) "Delay (both sounds)" else "Delay", fontSize = 15.sp, fontWeight = FontWeight.SemiBold,
                     modifier = Modifier.padding(top = 12.dp))
                DelayControls()
            }
        },
        Option("Release", releaseName(part.release)) {
            ReleaseSlider(k, accent)
            Text(if (part.release < 0.01f) "A note ends the instrument's own way when you lift your finger."
                 else "When you lift your finger the note plays on as it was and fades out over this time, " +
                      "like holding a piano's sustain pedal. A new note on the same finger cuts it off.",
                 color = TextDim, fontSize = 12.sp)
        },
        Option("Loop", loopSummary(), perPart = false) {
            Column(Modifier.verticalScroll(rememberScrollState())) { LoopControls(accent) }
        },
        // Experimental: on the rail once turned on in Settings.
        if (AppState.mods.gyroOn) Option("Gyro", gyroSummary(AppState.mods), perPart = false) {
            // The gyro's tabs and meter stay at the top; the rest scrolls under them.
            Column { GyroPanel(liveModReading(), scroll = true) }
        } else null,
        if (AppState.mods.breath != 0) Option("Breath", PhoneMods.BREATH[AppState.mods.breath], perPart = false) {
            Column(Modifier.verticalScroll(rememberScrollState())) { BreathControls(liveModReading()) }
        } else null,
    )
}

/** The controllers there are, to play with another. */
@Composable
private fun ControllerPicker(done: () -> Unit) {
    Column(Modifier.verticalScroll(rememberScrollState())) {
        for (c in Controllers.all) {
            val on = c === AppState.controller
            Column(Modifier.fillMaxWidth().clickable { AppState.switchController(c); done() }.padding(vertical = 8.dp)) {
                Text(c.name, fontSize = 17.sp, fontWeight = if (on) FontWeight.SemiBold else FontWeight.Normal,
                     color = if (on) PartColors[0] else Color.Unspecified)
                Text(c.description, color = TextDim, fontSize = 12.sp)
            }
        }
        Text("Each controller keeps its own settings. The sound, effects, loops and presets stay as they are when you switch.",
             color = TextDim, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp))
    }
}

/**
 * The rail at the left edge, Etherpad style: tap a setting to change it. At the top the
 * controller; with two sounds (a split, or a layer) a toggle for the one the settings below change.
 */
@Composable
private fun Rail(pick: Option, own: List<Option>, shared: List<Option>, t: PadTheme, open: String?,
                 tap: (String) -> Unit, onClose: () -> Unit) {
    val accent = t.parts[AppState.part]
    Column(Modifier.fillMaxHeight().width(92.dp).background(t.rail)
               .then(if (t.flat) Modifier.border(BorderStroke(2.dp, Color.Black)) else Modifier)
               .verticalScroll(rememberScrollState()).padding(vertical = 4.dp),
           horizontalAlignment = Alignment.CenterHorizontally, verticalArrangement = Arrangement.spacedBy(6.dp)) {
        Row {
            IconButton(onClick = onClose, Modifier.size(44.dp)) { Icon(Icons.Filled.Close, "Close", tint = t.railDim) }
            // Tuck the rail away for the whole width to play on; the corner button brings it back.
            IconButton(onClick = { AppState.changeRailHidden(true) }, Modifier.size(44.dp)) {
                Icon(Icons.AutoMirrored.Filled.KeyboardArrowLeft, "Hide the settings", tint = t.railDim) }
        }
        RailItem(pick, open == pick.label, t, accent, tap)
        if (AppState.twoParts) PartToggle(AppState.controller.splitNames ?: listOf("Main", "Layer"), t)
        for (o in own) RailItem(o, open == o.label, t, accent, tap)
        Box(Modifier.padding(horizontal = 14.dp).fillMaxWidth().height(1.dp).background(t.railDim.copy(alpha = 0.4f)))
        for (o in shared) RailItem(o, open == o.label, t, accent, tap)
    }
}

@Composable
private fun RailItem(o: Option, open: Boolean, t: PadTheme, accent: Color, tap: (String) -> Unit) {
    val on = open || o.on
    Column(Modifier.clickable { o.tap?.invoke() ?: tap(o.label) }
               .then(if (on && t.flat) Modifier.background(Color.Black) else Modifier)
               .padding(horizontal = 4.dp, vertical = 2.dp),
           horizontalAlignment = Alignment.CenterHorizontally) {
        Text(o.label, fontSize = 12.sp, fontWeight = FontWeight.SemiBold,
             color = when { on && t.flat -> Color.White; on -> accent; else -> t.railText })
        Text(o.value, fontSize = 10.sp, color = if (on && t.flat) Color.White else t.railDim, maxLines = 1)
    }
}

/** Which of two sounds the rail changes: the left or right pad of a split, or the main sound or its layer. */
@Composable
private fun PartToggle(names: List<String>, t: PadTheme) {
    Row(Modifier.padding(horizontal = 6.dp).fillMaxWidth().height(34.dp)
            .border(BorderStroke(if (t.flat) 2.dp else 1.dp, if (t.flat) Color.Black else t.railDim), RoundedCornerShape(8.dp))) {
        for (k in 0..1) {
            val on = k == AppState.part
            Box(Modifier.weight(1f).fillMaxHeight()
                    .background(when { !on -> Color.Transparent; t.flat -> Color.Black; else -> t.parts[k].copy(alpha = 0.3f) },
                                RoundedCornerShape(8.dp))
                    .clickable { AppState.side = k },
                contentAlignment = Alignment.Center) {
                Text(names[k], fontSize = 12.sp, fontWeight = if (on) FontWeight.Bold else FontWeight.Normal,
                     color = when { on && t.flat -> Color.White; on -> t.parts[k]; else -> t.railText })
            }
        }
    }
}

@Composable
private fun SettingPanel(o: Option, part: Int, t: PadTheme, close: () -> Unit) {
    val names = AppState.controller.splitNames ?: listOf("main", "layer")
    Surface(Modifier.padding(start = 100.dp, top = 12.dp, bottom = 12.dp).widthIn(max = 380.dp),
            shape = RoundedCornerShape(18.dp), color = if (t.flat) Color.White else Panel.copy(alpha = 0.96f), contentColor = Fg,
            border = if (t.flat) BorderStroke(Edge, Color.Black) else null) {
        Column(Modifier.padding(14.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(if (AppState.twoParts && o.perPart) "${o.label} · ${names[part].lowercase()}" else o.label,
                     fontSize = 17.sp, fontWeight = FontWeight.SemiBold, modifier = Modifier.weight(1f))
                TextButton(onClick = close) { Text("Done") }
            }
            o.panel?.invoke()
        }
    }
}

/** A word or two for the rail: the effects that are on for this part. */
private fun effectsSummary(p: Part): String {
    val on = listOfNotNull("Verb".takeIf { p.reverb > 0.01f }, "Delay".takeIf { p.delay > 0.01f },
                           "Drive".takeIf { p.drive > 0.01f }, "Tone".takeIf { p.brightness < 0.999f })
    return when { on.isEmpty() -> "Dry"; on.size <= 2 -> on.joinToString(" + "); else -> "${on.size} on" }
}

/** A word or two for the rail: what the gyro's sources do ("Off", "Twist: Bend", "2 on"). */
internal fun gyroSummary(m: PhoneMods): String {
    val on = m.gyro.withIndex().filter { it.value.target != 0 }
    return when (on.size) {
        0 -> "Off"
        1 -> "${PhoneMods.SOURCES[on[0].index]}: ${GyroSettings.TARGETS[on[0].value.target]}"
        else -> "${on.size} on"
    }
}
