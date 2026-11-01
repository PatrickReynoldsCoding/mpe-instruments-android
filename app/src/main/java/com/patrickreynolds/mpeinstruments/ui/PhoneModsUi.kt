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

/** How far the gauge's needle goes for this mode: vibrato is a few degrees, a bend tens. */
fun tiltSpan(mode: Int) = if (mode == 1) 6f else 36f
/** The angle under which a turn does nothing (as in Engine::updateModifiers). */
fun tiltDeadZone(mode: Int) = when (mode) { 1 -> 0.2f; 2 -> 2f; 3 -> 1.5f; else -> 0f }

/** The effect in words: "+1.0 semitone", "-22 cents", "tone +25%". */
fun tiltEffect(mode: Int, r: ModReading) = when (mode) {
    1 -> "%+d cents".format((r.bend * 100).roundToInt())
    2 -> "%+.1f semitone%s".format(r.bend, if (abs(r.bend) >= 1.05f) "s" else "")
    3 -> "tone %+d%%".format((r.slide * 100).roundToInt())
    else -> ""
}

/** The same reading in coarse steps: on e-ink the screen only redraws when one of these changes. */
fun ModReading.coarse() = copy(
    tilt = (tilt / 2).roundToInt() * 2f, bend = (bend * 10).roundToInt() / 10f, slide = (slide * 10).roundToInt() / 10f,
    breath = (breath * 10).roundToInt() / 10f, micDb = (micDb / 5).roundToInt() * 5f, floorDb = (floorDb / 5).roundToInt() * 5f,
    onDb = (onDb / 5).roundToInt() * 5f, fullDb = (fullDb / 5).roundToInt() * 5f, micLevel = (micLevel * 10).roundToInt() / 10f,
    echoDb = (echoDb / 5).roundToInt() * 5f)

/** What tilt and breath are doing, read every frame while one is on (in coarse steps on e-ink). */
@Composable
fun liveModReading(): ModReading {
    var r by remember { mutableStateOf(ModReading()) }
    LaunchedEffect(Unit) {
        while (true) {
            if (AppState.mods.tilt == 0 && AppState.mods.breath == 0) delay(250)    // nothing moves: no need to draw
            else withFrameNanos { }
            val now = ModReading.read()
            r = if (Eink) now.coarse() else now
        }
    }
    return r
}

/**
 * The tilt gauge: a little phone that turns as the phone does, on an arc whose ends are the
 * full effect. The shaded wedge in the middle is the dead zone. Dim while no finger is down:
 * then the zero is still following the phone.
 */
@Composable
fun TiltGauge(r: ModReading, mode: Int, modifier: Modifier = Modifier) {
    val accent = PartColors[0]
    Column(modifier.fillMaxWidth()) {
        Canvas(Modifier.fillMaxWidth().height(92.dp)) { tiltDial(r, mode, accent, if (Eink) Color.Black else TextDim, Eink) }
        Row(Modifier.fillMaxWidth()) {
            Text(if (r.held) "Zero set: turn to play" else "Zero follows the phone until you play",
                 color = TextDim, fontSize = 12.sp, modifier = Modifier.weight(1f))
            Text(tiltEffect(mode, r), fontSize = 14.sp, fontWeight = FontWeight.SemiBold, color = if (r.held) accent else TextDim)
        }
    }
}

private fun DrawScope.tiltDial(r: ModReading, mode: Int, accent: Color, dim: Color, flat: Boolean) {
    val c = Offset(size.width / 2, size.height * 0.96f)
    val rad = minOf(size.width * 0.45f, size.height * 0.9f)
    val span = tiltSpan(mode)
    val sweep = 70f                                            // degrees of arc each side
    fun vis(deg: Float) = (deg / span).coerceIn(-1f, 1f) * sweep
    fun at(a: Float, k: Float) = Offset(c.x + rad * k * sin(Math.toRadians(a.toDouble())).toFloat(),
                                        c.y - rad * k * cos(Math.toRadians(a.toDouble())).toFloat())
    val track = if (flat) Color.Black else dim.copy(alpha = 0.5f)
    // The arc, its ends (the full effect) and the dead zone.
    drawArc(track, startAngle = -90f - sweep, sweepAngle = 2 * sweep, useCenter = false,
            topLeft = Offset(c.x - rad, c.y - rad), size = Size(2 * rad, 2 * rad), style = Stroke(width = if (flat) 4f else 3f))
    val dz = vis(tiltDeadZone(mode))
    drawArc(if (flat) Color(0xFFAAAAAA) else dim.copy(alpha = 0.25f), startAngle = -90f - dz, sweepAngle = 2 * dz, useCenter = true,
            topLeft = Offset(c.x - rad, c.y - rad), size = Size(2 * rad, 2 * rad))
    val full = if (mode == 1) 3.5f else 30f
    for (s in listOf(-1f, 1f)) drawLine(track, at(vis(s * full), 0.88f), at(vis(s * full), 1.08f), strokeWidth = 4f)
    drawLine(track, at(0f, 0.9f), at(0f, 1.1f), strokeWidth = 2f)
    // The needle and the little phone, turned by the reading.
    val a = vis(r.tilt)
    val col = if (flat) Color.Black else if (r.held) accent else dim
    drawLine(col, c, at(a, 1f), strokeWidth = if (flat) 5f else 4f, cap = StrokeCap.Round)
    val pw = rad * 0.5f
    val ph = pw * 0.48f
    val pc = at(a, 0.42f)
    rotate(a, pivot = pc) {
        drawRoundRect(if (flat) Color.White else Color.Black.copy(alpha = 0.35f), topLeft = Offset(pc.x - pw / 2, pc.y - ph / 2),
                      size = Size(pw, ph), cornerRadius = CornerRadius(ph * 0.25f))
        drawRoundRect(col, topLeft = Offset(pc.x - pw / 2, pc.y - ph / 2), size = Size(pw, ph),
                      cornerRadius = CornerRadius(ph * 0.25f), style = Stroke(width = if (flat) 5f else 3f))
    }
    drawCircle(col, radius = 6f, center = c)
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

@Composable
fun TiltControls(r: ModReading) {
    val m = AppState.mods
    Chips(PhoneMods.TILT, m.tilt) { k -> AppState.updateMods { it.copy(tilt = k) } }
    Text(when (m.tilt) {
        1 -> "Wobble the phone for vibrato. Holding it at a slant does nothing, so it stays in tune."
        2 -> "Turn clockwise to bend up, anticlockwise to bend down. Each note starts in tune, wherever the phone is."
        3 -> "Turn to change each instrument's tone: the same as sliding a finger up or down a Seaboard key."
        else -> "Turn the phone like a steering wheel to add vibrato, bend the notes or change the tone."
    }, color = TextDim, fontSize = 12.sp, modifier = Modifier.padding(top = 4.dp))
    when (m.tilt) {
        1 -> LabeledSlider("Depth at a firm wobble", m.vibratoCents, 10f..100f, "${m.vibratoCents.roundToInt()} cents") { v ->
            AppState.updateMods { it.copy(vibratoCents = v) } }
        2 -> {
            Text("Bend at a 30° turn", fontSize = 14.sp, modifier = Modifier.padding(top = 8.dp, bottom = 4.dp))
            val i = PhoneMods.BEND_RANGES.indexOfFirst { abs(it - m.bendSemis) < 0.01f }
            Chips(PhoneMods.BEND_RANGES.map { "±${it.roundToInt()}" }, i) { k ->
                AppState.updateMods { it.copy(bendSemis = PhoneMods.BEND_RANGES[k]) } }
        }
        3 -> LabeledSlider("Change at a 30° turn", m.toneRange, 0.1f..1f, pct(m.toneRange)) { v -> AppState.updateMods { it.copy(toneRange = v) } }
    }
    if (m.tilt != 0) {
        if (Motion.available == false) Text("This phone has no gyroscope, so tilt cannot work here.", color = Warn, fontSize = 13.sp)
        else TiltGauge(r, m.tilt, Modifier.padding(top = 6.dp))
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
 * Setup tab, Experimental: tilt and breath, with what they are doing. New, and tried on an
 * emulator but not yet on many phones. Once one is on, it is also on the touch pad's rail.
 */
@Composable
fun ExperimentalCard() {
    val r = liveModReading()
    SectionCard("Experimental", accent = Warn) {
        Text("New and still being tuned: try them and see how they feel. They work on every instrument, from the touch pad " +
             "or a controller. Once one is on, it also appears on the touch pad's side rail.", color = TextDim, fontSize = 12.sp)
        Spacer(Modifier.height(12.dp))
        Text("Tilt (the gyro)", fontSize = 15.sp, fontWeight = FontWeight.SemiBold)
        Spacer(Modifier.height(4.dp))
        TiltControls(r)
        HorizontalDivider(color = Line, modifier = Modifier.padding(vertical = 12.dp))
        Text("Breath (the microphone)", fontSize = 15.sp, fontWeight = FontWeight.SemiBold)
        Spacer(Modifier.height(4.dp))
        BreathControls(r)
    }
}
