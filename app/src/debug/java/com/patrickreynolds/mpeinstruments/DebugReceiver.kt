package com.patrickreynolds.mpeinstruments

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.util.Log
import java.io.File

/**
 * Test hooks, debug builds only. Results go to logcat with the tag "mpe-test".
 *   adb shell am broadcast -n com.patrickreynolds.mpeinstruments/.DebugReceiver -a RECORD --ef seconds 5 --es name note
 *   ... -a STATS
 *   ... -a SET --es instrument strings/cello --ei watchdog 1500 --ei transpose 0 --ef volume 0.8
 *   ... -a PANIC
 *   ... -a BLE_CONNECT --ef seconds 15     scan for a BLE MIDI device, connect to the first found
 * Recordings are saved to /sdcard/Android/data/com.patrickreynolds.mpeinstruments/files/<name>.wav
 */
class DebugReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        when (intent.action) {
            "RECORD" -> {
                val seconds = intent.getFloatExtra("seconds", 5f)
                val name = intent.getStringExtra("name") ?: "capture"
                val dir = context.getExternalFilesDir(null) ?: context.filesDir
                if (!Native.nativeCaptureStart(seconds)) { Log.w(TAG, "RECORD refused: already recording"); return }
                Log.i(TAG, "RECORD started $name ${seconds}s")
                Thread {
                    val deadline = System.currentTimeMillis() + (seconds * 1000).toLong() + 10_000
                    while (Native.nativeCaptureProgress() < 1.0 && System.currentTimeMillis() < deadline) Thread.sleep(50)
                    val file = File(dir, "$name.wav")
                    val ok = Native.nativeCaptureSave(file.absolutePath)
                    Log.i(TAG, if (ok) "RECORD saved ${file.absolutePath}" else "RECORD failed $name (progress ${Native.nativeCaptureProgress()})")
                }.start()
            }
            "STATS" -> {
                val s = Stats.from(Native.nativeStats())
                Log.i(TAG, "STATS connected=\"${MidiConnector.connectedName}\" midiState=${s.midiState} midiBytes=${s.midiBytes} " +
                    "events=${s.events} dropped=${s.dropped} panics=${s.panics} voices=${s.voices} " +
                    "ageAvgMs=${s.eventAgeAvgMs} ageMaxMs=${s.eventAgeMaxMs} outLatencyMs=${s.outputLatencyMs} " +
                    "rate=${s.sampleRate} burst=${s.burstFrames} buffer=${s.bufferFrames} xruns=${s.xruns} " +
                    "exclusive=${s.exclusive} path=${s.pathName.replace(' ', '_')} loadAvg=${s.loadAvg} loadPeak=${s.loadPeak} restarts=${s.streamRestarts} " +
                    "arpSteps=${s.arpSteps} noteCenter=${s.noteCenter}")
            }
            "SET" -> {
                // Through AppState, so the screen and the saved settings agree with what is playing.
                AppState.init(context)
                intent.getStringExtra("instrument")?.let { id ->
                    if (AppState.instruments.any { it.id == id }) AppState.updatePart(0) { it.copy(instrument = id) }
                    else Log.w(TAG, "unknown instrument $id")
                }
                if (intent.hasExtra("watchdog")) Native.nativeSetWatchdogMs(intent.getIntExtra("watchdog", 0))
                if (intent.hasExtra("transpose")) AppState.changeTranspose(intent.getIntExtra("transpose", 0))
                if (intent.hasExtra("volume")) AppState.changeVolume(intent.getFloatExtra("volume", 0.8f))
                if (intent.hasExtra("bend")) AppState.changeBendRange(intent.getFloatExtra("bend", 48f))
                // --es perf '<Perf json>' sets the whole performance (split, arp, effects); --es preset <name> loads one.
                intent.getStringExtra("perf")?.let { j -> AppState.update { Perf.from(org.json.JSONObject(j)) } }
                intent.getStringExtra("preset")?.let { n -> AppState.allPresets.firstOrNull { it.name == n }?.let { AppState.load(it) } }
                Log.i(TAG, "SET done")
            }
            "PANIC" -> { Native.nativePanic(); Log.i(TAG, "PANIC done") }
            "BLE_CONNECT" -> {
                val seconds = intent.getFloatExtra("seconds", 15f)
                var done = false
                lateinit var scanner: BleScanner
                scanner = BleScanner(context.applicationContext) { device ->
                    if (done) return@BleScanner
                    done = true
                    scanner.stop()
                    Log.i(TAG, "BLE_CONNECT found ${device.address}, connecting")
                    MidiConnector.connectBluetooth(device)
                }
                if (!scanner.available) { Log.w(TAG, "BLE_CONNECT: Bluetooth is off"); return }
                scanner.start()
                Log.i(TAG, "BLE_CONNECT scanning")
                android.os.Handler(android.os.Looper.getMainLooper()).postDelayed({
                    if (!done) { scanner.stop(); Log.w(TAG, "BLE_CONNECT: nothing found in ${seconds}s") }
                }, (seconds * 1000).toLong())
            }
            else -> Log.w(TAG, "unknown command ${intent.action}")
        }
    }

    companion object { private const val TAG = "mpe-test" }
}
