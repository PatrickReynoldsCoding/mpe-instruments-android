package com.patrickreynolds.mpeinstruments.ui

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalLifecycleOwner
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import com.patrickreynolds.mpeinstruments.BuildConfig
import com.patrickreynolds.mpeinstruments.Updater
import com.patrickreynolds.mpeinstruments.Updater.State
import java.text.DateFormat
import java.util.Date

/** Setup tab: the version, a check for a newer one, and installing it. */
@Composable
fun UpdatesCard() {
    val context = LocalContext.current
    val s = Updater.state
    // Back from Android's "install unknown apps" page: carry on with the install.
    val owner = LocalLifecycleOwner.current
    DisposableEffect(owner) {
        val obs = LifecycleEventObserver { _, e -> if (e == Lifecycle.Event.ON_RESUME) Updater.continueInstall(context) }
        owner.lifecycle.addObserver(obs)
        onDispose { owner.lifecycle.removeObserver(obs) }
    }
    SectionCard("Updates") {
        Text("This is version ${BuildConfig.VERSION_NAME}", fontSize = 15.sp)
        Spacer(Modifier.height(6.dp))
        when (s) {
            is State.Idle -> {}
            is State.Checking -> Text("Checking GitHub…", color = TextDim, fontSize = 13.sp)
            is State.UpToDate -> Text("Up to date: ${s.latest} is the latest.", color = TextDim, fontSize = 13.sp)
            is State.Available -> ReleaseInfo(s.release)
            is State.Downloading -> {
                Text("Downloading ${s.release.version}… ${(s.progress * 100).toInt()}%", fontSize = 13.sp)
                LinearProgressIndicator(progress = { s.progress }, modifier = Modifier.fillMaxWidth().padding(top = 6.dp), color = PartColors[1])
            }
            is State.NeedsPermission -> {
                Text("${s.release.version} is downloaded. Android needs your OK for MPE Instruments to install its updates: " +
                     "turn on \"Allow from this source\", then come back.", fontSize = 13.sp)
                Button(onClick = { context.startActivity(Updater.permissionSettings(context)) }, modifier = Modifier.padding(top = 8.dp),
                       colors = ButtonDefaults.buttonColors(containerColor = PartColors[1], contentColor = Ink)) {
                    Text("Open Android's setting")
                }
            }
            is State.Installing -> Text("Installing ${s.release.version}… The app closes while Android updates it; open it again afterwards.",
                                        fontSize = 13.sp)
            is State.Failed -> Text(s.message, color = Warn, fontSize = 13.sp)
        }
        val busy = s is State.Checking || s is State.Downloading || s is State.Installing
        val release = (s as? State.Available)?.release ?: (s as? State.Failed)?.release
        Row(Modifier.padding(top = 8.dp), verticalAlignment = Alignment.CenterVertically) {
            if (release != null && !busy) {
                Button(onClick = { Updater.downloadAndInstall(context, release) },
                       colors = ButtonDefaults.buttonColors(containerColor = PartColors[1], contentColor = Ink)) {
                    Text(if (s is State.Failed) "Try again" else "Update to ${release.version}")
                }
                Spacer(Modifier.width(8.dp))
            }
            if (s !is State.NeedsPermission) OutlinedButton(onClick = { Updater.check(context) }, enabled = !busy) { Text("Check now") }
        }
        SwitchRow("Check when the app opens", Updater.autoCheck,
                  "At most once a day. The app only goes online for this, to GitHub.", accent = PartColors[1]) {
            Updater.changeAutoCheck(it)
        }
        if (Updater.lastCheckMs > 0) Text("Last checked ${DateFormat.getDateTimeInstance(DateFormat.MEDIUM, DateFormat.SHORT).format(Date(Updater.lastCheckMs))}",
                                         color = TextDim, fontSize = 11.sp)
    }
}

@Composable
private fun ReleaseInfo(r: Updater.Release) {
    Text("Version ${r.version} is available", fontSize = 16.sp, fontWeight = FontWeight.SemiBold, color = PartColors[1])
    // The first lines of the release notes, without Markdown marks.
    val notes = r.notes.lines().map { it.replace(Regex("[#*`]"), "").trim() }.filter { it.isNotEmpty() }
        .filterNot { it.startsWith("MPE Instruments") }.take(3).joinToString("\n")
    if (notes.isNotEmpty()) Text(notes, color = TextDim, fontSize = 12.sp, maxLines = 6, modifier = Modifier.padding(top = 4.dp))
    val mb = if (r.size > 0) " (%.1f MB)".format(r.size / 1e6) else ""
    Text("Downloads ${r.apkName}$mb from GitHub. Your settings and presets are kept.", color = TextDim, fontSize = 11.sp,
         modifier = Modifier.padding(top = 4.dp))
}

/** Play tab: a newer version is out. Tapping goes to Setup, where it is installed. */
@Composable
fun UpdateBanner(openSetup: () -> Unit) {
    val r = (Updater.state as? State.Available)?.release ?: return
    Card(Modifier.fillMaxWidth().padding(horizontal = 16.dp, vertical = 6.dp).clickable(onClick = openSetup),
         shape = RoundedCornerShape(18.dp), colors = CardDefaults.cardColors(containerColor = PartColors[1].copy(alpha = if (Eink) 0f else 0.14f)),
         border = if (Eink) androidx.compose.foundation.BorderStroke(Edge, Line) else null) {
        Row(Modifier.padding(16.dp), verticalAlignment = Alignment.CenterVertically) {
            Column(Modifier.weight(1f)) {
                Text("Version ${r.version} is available", color = PartColors[1], fontWeight = FontWeight.SemiBold)
                Text("Update from Setup → Updates", color = TextDim, fontSize = 13.sp)
            }
            Text("Update", color = PartColors[1], fontWeight = FontWeight.SemiBold)
        }
    }
}
