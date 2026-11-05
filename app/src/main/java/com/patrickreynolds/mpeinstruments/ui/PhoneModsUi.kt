package com.patrickreynolds.mpeinstruments.ui

import android.Manifest
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.layout.width
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.rotate
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.patrickreynolds.mpeinstruments.AppState
import com.patrickreynolds.mpeinstruments.GyroSettings
import com.patrickreynolds.mpeinstruments.GyroSource
import com.patrickreynolds.mpeinstruments.ModReading
import com.patrickreynolds.mpeinstruments.Motion
import com.patrickreynolds.mpeinstruments.PhoneControl
import com.patrickreynolds.mpeinstruments.PhoneMods
import kotlinx.coroutines.delay
import kotlin.math.abs
import kotlin.math.cos
import kotlin.math.roundToInt
import kotlin.math.sin

// ---- what the screen shows of tilt and breath --------------------------------------------------

/** How far a source's meter goes: a vibrato is a few degrees, other turns tens, a shake 0..1. */
fun gyroSpan(source: Int, target: Int) = when { source == 3 -> 1f; target == GyroSettings.VIBRATO -> 6f; else -> 36f }

/** What the gyro is doing, in words: "+1.0 semitone · volume 80%". */
fun gyroEffect(m: PhoneMods, r: ModReading): String {
    val targets = m.gyro.map { it.target }.filter { it != 0 }.distinct()
    return targets.mapNotNull { t -> when (t) {
        GyroSettings.BEND -> "%+.1f semitone%s".format(r.bend, if (abs(r.bend) >= 1.05f) "s" else "")
        GyroSettings.VIBRATO -> if (GyroSettings.BEND in targets) null else "%+d cents".format((r.bend * 100).roundToInt())
        GyroSettings.VOLUME -> "volume ${(r.gain * 100).roundToInt()}%"
        GyroSettings.TONE -> "tone %+d%%".format((r.slide * 100).roundToInt())
        GyroSettings.BRIGHTNESS -> "brightness ${(r.bright * 100).roundToInt()}%"
        GyroSettings.RELEASE -> "release +%.1f s".format(r.release)
        GyroSettings.REVERB -> "reverb %+d%%".format((r.reverb * 100).roundToInt())
        GyroSettings.DELAY -> "delay %+d%%".format((r.delay * 100).roundToInt())
        else -> null
    } }.joinToString(" · ")
}

/** The same reading in coarse steps: on e-ink the screen only redraws when one of these changes. */
fun ModReading.coarse() = copy(
    tilt = (tilt / 2).roundToInt() * 2f, axis = axis.map { (it / 2).roundToInt() * 2f }, gain = (gain * 10).roundToInt() / 10f,
    bright = (bright * 10).roundToInt() / 10f, release = (release * 10).roundToInt() / 10f,
    reverb = (reverb * 10).roundToInt() / 10f, delay = (delay * 10).roundToInt() / 10f, bend = (bend * 10).roundToInt() / 10f, slide = (slide * 10).roundToInt() / 10f,
    breath = (breath * 10).roundToInt() / 10f, micDb = (micDb / 5).roundToInt() * 5f, floorDb = (floorDb / 5).roundToInt() * 5f,
    onDb = (onDb / 5).roundToInt() * 5f, fullDb = (fullDb / 5).roundToInt() * 5f, micLevel = (micLevel * 10).roundToInt() / 10f,
    echoDb = (echoDb / 5).roundToInt() * 5f)

/** What the gyro and breath are doing, read every frame while one is on (in coarse steps on e-ink). */
@Composable
fun liveModReading(): ModReading {
    var r by remember { mutableStateOf(ModReading()) }
    LaunchedEffect(Unit) {
        while (true) {
            if (!AppState.mods.gyroOn && AppState.mods.breath == 0) delay(250)    // nothing moves: no need to draw
            else withFrameNanos { }
            val now = ModReading.read()
            r = if (Eink) now.coarse() else now
        }
    }
    return r
}

/**
 * One gyro source's meter, on one line. A turn: a needle from the middle, the full effect at the
 * ends and the dead zone shaded; dim while no finger is down (then the zero still follows the
 * phone). A shake: a bar filling from the left. Dim and empty while the source is off.
 */
@Composable
fun GyroMeter(r: ModReading, source: Int, target: Int, modifier: Modifier = Modifier) {
    val accent = PartColors[0]
    val v = if (target == 0) 0f else r.axis.getOrElse(source) { 0f }
    val live = target != 0 && (r.held || source == 3)
    Row(modifier.fillMaxWidth().height(28.dp), verticalAlignment = androidx.compose.ui.Alignment.CenterVertically) {
        Canvas(Modifier.weight(1f).height(22.dp)) { gyroBar(v, source, target, live, accent, Eink) }
        Text(when {
            target == 0 -> "off"
            source == 3 -> "${(v * 100).roundToInt()}%"
            else -> v.roundToInt().let { if (it == 0) "0°" else "%+d°".format(it) }
        }, fontSize = 13.sp, fontWeight = FontWeight.SemiBold, color = if (live) accent else TextDim,
           textAlign = androidx.compose.ui.text.style.TextAlign.End, modifier = Modifier.width(48.dp))
    }
}

private fun DrawScope.gyroBar(v: Float, source: Int, target: Int, live: Boolean, accent: Color, flat: Boolean) {
    val h = size.height; val w = size.width
    val track = if (flat) Color.White else Line
    drawRoundRect(track, topLeft = Offset(0f, h * 0.3f), size = Size(w, h * 0.4f), cornerRadius = CornerRadius(h * 0.2f))
    if (flat) drawRoundRect(Color.Black, topLeft = Offset(0f, h * 0.3f), size = Size(w, h * 0.4f), cornerRadius = CornerRadius(h * 0.2f),
                            style = Stroke(width = 3f))
    val col = if (flat) Color.Black else if (live) accent else TextDim
    if (source == 3) {
        drawRoundRect(col, topLeft = Offset(0f, h * 0.3f), size = Size(w * v.coerceIn(0f, 1f), h * 0.4f), cornerRadius = CornerRadius(h * 0.2f))
        return
    }
    val span = gyroSpan(source, target)
    fun x(deg: Float) = w / 2 + (deg / span).coerceIn(-1f, 1f) * w / 2
    val dz = if (target == GyroSettings.VIBRATO) 0.2f else if (target == GyroSettings.BEND) 2f else 1.5f
    drawRect(if (flat) Color(0xFFAAAAAA) else TextDim.copy(alpha = 0.3f), Offset(x(-dz), h * 0.3f), Size(x(dz) - x(-dz), h * 0.4f))
    val full = if (target == GyroSettings.VIBRATO) 3.5f else 30f
    for (s in listOf(-1f, 1f)) drawLine(if (flat) Color.Black else TextDim, Offset(x(s * full), h * 0.1f), Offset(x(s * full), h * 0.9f), strokeWidth = 3f)
    drawLine(col, Offset(w / 2, h / 2), Offset(x(v), h / 2), strokeWidth = h * 0.4f)
    drawCircle(col, radius = h * 0.32f, center = Offset(x(v), h / 2))
}

/**
 * The breath meter, left to right from silence to full scale. The shaded stretch is where breath
 * plays, from "starts" to "full"; the thin bar is what the microphone hears now; the thick bar
 * the breath as applied. A red tick: the app's own sound from the speaker, which breath must beat.
 */
@Composable
fun BreathMeter(r: ModReading, mode: Int, micState: Int, modifier: Modifier = Modifier) {
    val accent = PartColors[1]
    Column(modifier.fillMaxWidth()) {
        Canvas(Modifier.fillMaxWidth().height(30.dp)) { breathBar(r, accent, Eink) }
        Row(Modifier.fillMaxWidth()) {
            val hint = when {
                micState == -1 -> "The microphone could not be opened (another app may be using it). Notes play as if breath were off."
                micState == 0 -> "Starting the microphone…"
                r.breath > 0.02f -> "Breath %d%%".format((r.breath * 100).roundToInt())
                mode == 1 && r.held -> "Blow to sound the notes"
                else -> "Blow across the bottom edge of the phone"
            }
            Text(hint, fontSize = 13.sp, fontWeight = if (r.breath > 0.02f) FontWeight.SemiBold else FontWeight.Normal,
                 color = if (micState == -1) Warn else if (r.breath > 0.02f) accent else TextDim, modifier = Modifier.weight(1f))
            if (micState != -1) Text("room %d dB · starts %d dB".format(r.floorDb.roundToInt(), r.onDb.roundToInt()),
                                     color = TextDim, fontSize = 11.sp)
        }
    }
}

private const val METER_LOW_DB = -100f

private fun DrawScope.breathBar(r: ModReading, accent: Color, flat: Boolean) {
    fun x(db: Float) = ((db - METER_LOW_DB) / -METER_LOW_DB).coerceIn(0f, 1f) * size.width
    val h = size.height
    val track = if (flat) Color.White else Line
    drawRoundRect(track, topLeft = Offset(0f, h * 0.2f), size = Size(size.width, h * 0.6f), cornerRadius = CornerRadius(h * 0.3f))
    if (flat) drawRoundRect(Color.Black, topLeft = Offset(0f, h * 0.2f), size = Size(size.width, h * 0.6f),
                            cornerRadius = CornerRadius(h * 0.3f), style = Stroke(width = 3f))
    val on = x(r.onDb); val full = x(r.fullDb)
    drawRect(if (flat) Color(0xFFCCCCCC) else accent.copy(alpha = 0.18f), Offset(on, h * 0.2f), Size(full - on, h * 0.6f))
    // The breath as applied, from where it starts.
    if (r.breath > 0.005f) drawRect(if (flat) Color.Black else accent, Offset(on, h * 0.2f), Size((full - on) * r.breath, h * 0.6f))
    // What the microphone hears now.
    drawRect(if (flat) Color.Black else Fg.copy(alpha = 0.7f), Offset(0f, h * 0.45f), Size(x(r.micDb), h * 0.1f))
    drawLine(if (flat) Color.Black else accent, Offset(on, 0f), Offset(on, h), strokeWidth = 4f)
    drawLine(if (flat) Color.Black else TextDim, Offset(x(r.floorDb), h * 0.1f), Offset(x(r.floorDb), h * 0.9f), strokeWidth = 2f)
    if (r.echoDb > METER_LOW_DB + 5) drawLine(if (flat) Color.Black else Danger, Offset(x(r.echoDb), 0f), Offset(x(r.echoDb), h), strokeWidth = 3f)
}

// ---- the settings ----------------------------------------------------------------------------

/** What each source does, in words. */
private val SOURCE_HELP = listOf(
    "Turn the phone like a steering wheel. Clockwise is up (or more).",
    "Tip the top edge away from you for up (or more), towards you for down.",
    "Turn the right edge away from you for up (or more), the left edge for down.",
    "Shake the phone: the harder, the more. Volume and brightness are lowered at rest and come back as you shake.",
)
private val TARGET_HELP = mapOf(
    GyroSettings.BEND to "Bends every note. Each note starts in tune, wherever the phone is.",
    GyroSettings.VIBRATO to "A quick wobble adds vibrato; holding the phone at a slant does nothing, so it stays in tune.",
    GyroSettings.VOLUME to "Louder one way, quieter the other.",
    GyroSettings.TONE to "Each instrument's own tone control, like sliding a finger up or down a Seaboard key.",
    GyroSettings.BRIGHTNESS to "Darker the other way: a filter closes over the sound.",
    GyroSettings.RELEASE to "Notes ring on after you lift your finger, longer the further you move it.",
    GyroSettings.REVERB to "More of the sound goes into the reverb.",
    GyroSettings.DELAY to "More of the sound goes into the delay.",
)

/**
 * The gyro: pick a source (twist, tip, turn, shake), then what it does and how much. The source
 * tabs and the source's meter stay at the top; with [scroll], only what is below them scrolls.
 */
@Composable
fun GyroPanel(r: ModReading, scroll: Boolean) {
    val m = AppState.mods
    var src by remember { mutableIntStateOf(m.gyro.indexOfFirst { it.target != 0 }.coerceAtLeast(0)) }
    Segmented(PhoneMods.SOURCES.mapIndexed { k, n -> if (m.source(k).target != 0) "$n •" else n }, src) { src = it }
    if (Motion.available == false && m.gyroOn) Text("This phone has no gyroscope, so the gyro cannot work here.", color = Warn, fontSize = 13.sp)
    else GyroMeter(r, src, m.source(src).target, Modifier.padding(top = 4.dp))
    HorizontalDivider(color = Line)
    Column(if (scroll) Modifier.verticalScroll(rememberScrollState()) else Modifier) { GyroControls(src) }
}

@Composable
private fun GyroControls(src: Int) {
    val m = AppState.mods
    val g = m.source(src)
    Text(SOURCE_HELP[src] + if (src < 3 && g.target != 0) " " + (if (g.invert) "Reversed: the other way is up." else "") else "",
         color = TextDim, fontSize = 12.sp, modifier = Modifier.padding(top = 6.dp))
    Text("Controls", color = TextDim, fontSize = 13.sp, modifier = Modifier.padding(top = 8.dp))
    Chips(GyroSettings.TARGETS, g.target) { t ->
        AppState.updateMods { it.withSource(src) { s -> GyroSource(t, GyroSettings.defaultAmount(t), s.invert) } } }
    if (g.target == 0) return
    TARGET_HELP[g.target]?.let { Text(it, color = TextDim, fontSize = 12.sp, modifier = Modifier.padding(top = 4.dp)) }
    if (src < 3) SwitchRow("Reverse", g.invert, when (src) {
        0 -> "Anticlockwise is up (or more)"
        1 -> "Tipping the top edge towards you is up (or more)"
        else -> "Turning the left edge away from you is up (or more)"
    }) { v -> AppState.updateMods { it.withSource(src) { s -> s.copy(invert = v) } } }
    val full = if (src == 3) "at a hard shake" else if (g.target == GyroSettings.VIBRATO) "at a firm wobble" else "at a 30° turn"
    if (g.target == GyroSettings.BEND) {
        Text("Bend $full", fontSize = 14.sp, modifier = Modifier.padding(top = 8.dp, bottom = 4.dp))
        val i = GyroSettings.BEND_RANGES.indexOfFirst { abs(it - g.amount) < 0.01f }
        Chips(GyroSettings.BEND_RANGES.map { "±${it.roundToInt()}" }, i) { k ->
            AppState.updateMods { it.withSource(src) { s -> s.copy(amount = GyroSettings.BEND_RANGES[k]) } } }
    } else {
        val range = GyroSettings.range(g.target)
        LabeledSlider("Amount $full", g.amount, range, GyroSettings.amountName(g.target, g.amount)) { v ->
            AppState.updateMods { it.withSource(src) { s -> s.copy(amount = v) } } }
    }
    // Bends: only with this source on Bend (the settings are shared by every source that bends).
    if (g.target == GyroSettings.BEND) {
        HorizontalDivider(color = Line, modifier = Modifier.padding(vertical = 10.dp))
        Text("Bends", fontSize = 14.sp, fontWeight = FontWeight.SemiBold)
        LabeledSlider("Snap to semitones", m.bendSnap, 0f..1f,
                      when { m.bendSnap < 0.01f -> "off"; m.bendSnap > 0.99f -> "at once"; else -> pct(m.bendSnap) }) { v ->
            AppState.updateMods { it.copy(bendSnap = v) } }
        Text(when {
            m.bendSnap < 0.01f -> "The bend follows the phone smoothly."
            m.bendSnap > 0.99f -> "The bend jumps from semitone to semitone, with nothing in between."
            else -> "The bend holds each semitone for a while, then moves on to the next. More snap holds it longer."
        }, color = TextDim, fontSize = 12.sp)
        SwitchRow("A new note resets the bend", m.bendReset,
                  "When you play another note, or a finger slides onto one, the bend starts again from where the phone is") { v ->
            AppState.updateMods { it.copy(bendReset = v) } }
    }
}

@Composable
fun BreathControls(r: ModReading) {
    val m = AppState.mods
    val context = LocalContext.current
    var pending by remember { mutableIntStateOf(0) }
    var denied by remember { mutableStateOf(false) }
    val ask = rememberLauncherForActivityResult(ActivityResultContracts.RequestPermission()) { ok ->
        denied = !ok
        if (ok && m.breath == pending) { PhoneControl.apply(context, AppState.mods); com.patrickreynolds.mpeinstruments.SynthService.refresh(context) }
        else if (ok) AppState.updateMods { it.copy(breath = pending) }
    }
    Chips(PhoneMods.BREATH, m.breath) { k ->
        if (k == 0 || PhoneControl.forceBreath || PhoneControl.hasMicPermission(context)) AppState.updateMods { it.copy(breath = k) }
        else { pending = k; ask.launch(Manifest.permission.RECORD_AUDIO) }
    }
    Text(when (m.breath) {
        1 -> "Your breath is the loudness: no breath, no sound, so your fingers only pick the notes. Bowed, sung and blown instruments also play harder as you blow harder."
        2 -> "Notes play as usual; blowing opens the tone from dark to bright, like a wah."
        else -> "Blow across the bottom edge of the phone, where the microphone is, to shape every note."
    }, color = TextDim, fontSize = 12.sp, modifier = Modifier.padding(top = 4.dp))
    if (denied) Text("Breath needs the microphone. Allow it for MPE Instruments in Android's settings.", color = Warn, fontSize = 13.sp)
    // On, but the permission was taken away since (or never given): say so, and offer to ask again.
    if (m.breath != 0 && !PhoneControl.forceBreath && !PhoneControl.hasMicPermission(context)) {
        Text("Breath is on but cannot hear you: it needs the microphone. Notes play as if it were off.", color = Warn, fontSize = 13.sp,
             modifier = Modifier.padding(top = 6.dp))
        androidx.compose.material3.TextButton(onClick = { pending = m.breath; ask.launch(Manifest.permission.RECORD_AUDIO) }) {
            Text("Allow the microphone") }
        return
    }
    if (m.breath != 0) {
        LabeledSlider("Sensitivity", m.sensitivity, 0f..1f,
                      when { m.sensitivity < 0.34f -> "a hard blow"; m.sensitivity > 0.66f -> "a soft breath"; else -> "medium" }) { v ->
            AppState.updateMods { it.copy(sensitivity = v) } }
        BreathMeter(r, m.breath, PhoneControl.micState)
        Text("Only how loud the microphone is below 150 Hz is measured: nothing is recorded or kept. Through the " +
             "speaker the app ignores its own bass; headphones work best.", color = TextDim, fontSize = 11.sp,
             modifier = Modifier.padding(top = 6.dp))
    }
}

/**
 * Setup tab, Experimental: the gyro and breath, with what they are doing. New, and tried on an
 * emulator but not yet on many phones. Once one is on, it is also on the touch pad's rail.
 */
@Composable
fun ExperimentalCard() {
    val r = liveModReading()
    SectionCard("Experimental", accent = Warn) {
        Text("New and still being tuned: try them and see how they feel. They work on every instrument, from the touch pad " +
             "or a controller. Once one is on, it also appears on the touch pad's side rail.", color = TextDim, fontSize = 12.sp)
        Spacer(Modifier.height(12.dp))
        Text("Gyro (the phone's motion)", fontSize = 15.sp, fontWeight = FontWeight.SemiBold)
        Spacer(Modifier.height(4.dp))
        GyroPanel(r, scroll = false)
        HorizontalDivider(color = Line, modifier = Modifier.padding(vertical = 12.dp))
        Text("Breath (the microphone)", fontSize = 15.sp, fontWeight = FontWeight.SemiBold)
        Spacer(Modifier.height(4.dp))
        BreathControls(r)
    }
}
