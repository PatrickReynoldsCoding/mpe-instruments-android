package com.patrickreynolds.fakeseaboard

import android.content.BroadcastReceiver
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.util.Log

/**
 * Commands from the computer, e.g.
 *   adb shell am broadcast -n com.patrickreynolds.fakeseaboard/.CommandReceiver -a PLAY --es file seaboard-session.mid --ef speed 1
 *   ... -a NOTE --ei channel 1 --ei note 57 --ef seconds 2 --ef vibrato 0
 *   ... -a HOLD --ei channel 1 --ei note 57      (note on that is never released by the device)
 *   ... -a CHORD --es notes 1:48,2:52,3:55 --ef seconds 3     (several fingers at once)
 *   ... -a STOP            stop and lift every finger
 *   ... -a SILENCE         stop mid-note with no note-off (a dropped Bluetooth link)
 *   ... -a UNPLUG / PLUG   remove / add the device, like pulling the USB cable
 *   ... -a BLE_START       also be a Bluetooth LE MIDI device (advertise; for a second emulator or phone)
 *   ... -a BLE_STOP        drop the Bluetooth link, like walking out of range
 */
class CommandReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        val tag = FakeSeaboardService.TAG
        Log.i(tag, "command ${intent.action}")
        when (intent.action) {
            "PLAY" -> {
                val file = intent.getStringExtra("file") ?: "seaboard-session.mid"
                val speed = intent.getFloatExtra("speed", 1f).toDouble()
                val seconds = intent.getFloatExtra("seconds", 0f).toDouble()       // 0 = all of it
                var timeline = context.assets.open(file).use { MidiFile.read(it) }
                if (seconds > 0) timeline = timeline.filter { it.seconds <= seconds }
                Player.play(timeline, speed, file)
            }
            "NOTE", "HOLD" -> Player.play(
                Player.gesture(intent.getIntExtra("channel", 1), intent.getIntExtra("note", 57),
                               intent.getFloatExtra("seconds", 2f).toDouble(),
                               intent.getFloatExtra("vibrato", 0f).toDouble(),
                               holdForever = intent.action == "HOLD"),
                1.0, "${intent.action} ${intent.getIntExtra("note", 57)}")
            "CHORD" -> {
                // --es notes 1:48,2:52,3:55  one finger per channel, all for --ef seconds, merged into one timeline
                val fingers = (intent.getStringExtra("notes") ?: "1:48,2:52,3:55").split(",").map { it.split(":") }
                val seconds = intent.getFloatExtra("seconds", 2f).toDouble()
                val vibrato = intent.getFloatExtra("vibrato", 0f).toDouble()
                val timeline = fingers.flatMap { (ch, note) -> Player.gesture(ch.trim().toInt(), note.trim().toInt(), seconds, vibrato, false) }
                    .sortedBy { it.seconds }
                Player.play(timeline, 1.0, "CHORD ${fingers.size} notes")
            }
            "STOP" -> Player.stop(releaseNotes = true)
            "SILENCE" -> Player.stop(releaseNotes = false)
            "UNPLUG", "PLUG" -> {
                // The MIDI service drops a device whose service is disabled, and adds it back when enabled.
                if (intent.action == "UNPLUG") Player.stop(releaseNotes = false)
                val state = if (intent.action == "UNPLUG") PackageManager.COMPONENT_ENABLED_STATE_DISABLED
                            else PackageManager.COMPONENT_ENABLED_STATE_ENABLED
                context.packageManager.setComponentEnabledSetting(
                    ComponentName(context, FakeSeaboardService::class.java), state, PackageManager.DONT_KILL_APP)
            }
            "BLE_START" -> BlePeripheral.start(context.applicationContext)
            "BLE_STOP" -> { Player.stop(releaseNotes = false); BlePeripheral.stop() }
            else -> Log.w(tag, "unknown command ${intent.action}")
        }
    }
}
