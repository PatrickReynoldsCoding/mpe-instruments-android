package com.patrickreynolds.mpeinstruments

import android.Manifest
import android.app.Activity
import android.bluetooth.BluetoothDevice
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Typeface
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.View
import android.view.WindowInsets
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.SeekBar
import android.widget.Spinner
import android.widget.TextView

/** One plain screen: connect a controller, pick an instrument, see the latency numbers. */
class MainActivity : Activity() {
    private val ui = Handler(Looper.getMainLooper())
    private lateinit var prefs: android.content.SharedPreferences
    private lateinit var status: TextView
    private lateinit var stats: TextView
    private lateinit var deviceList: LinearLayout
    private lateinit var transposeLabel: TextView
    private var transpose = 0
    private var ble: BleScanner? = null
    private val bleSeen = LinkedHashMap<String, BluetoothDevice>()
    private var instruments: List<Native.Instrument> = emptyList()

    private val refresh = object : Runnable {
        override fun run() {
            updateStats()
            ui.postDelayed(this, 250)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        prefs = getSharedPreferences("mpe", Context.MODE_PRIVATE)
        requestNeededPermissions()
        startForegroundService(Intent(this, SynthService::class.java))
        MidiConnector.init(applicationContext)
        MidiConnector.onChange = { runOnUiThread { showDevices() } }
        instruments = Native.instruments()
        setContentView(buildUi())
        applySavedSettings()
        showDevices()
    }

    override fun onResume() { super.onResume(); ui.post(refresh) }
    override fun onPause() { ui.removeCallbacks(refresh); ble?.stop(); super.onPause() }

    // ---- permissions -----------------------------------------------------------------------

    private fun requestNeededPermissions() {
        val want = mutableListOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
        if (Build.VERSION.SDK_INT >= 33) want += Manifest.permission.POST_NOTIFICATIONS
        val missing = want.filter { checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED }
        if (missing.isNotEmpty()) requestPermissions(missing.toTypedArray(), 1)
    }

    private fun hasBlePermissions() =
        checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED &&
        checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED

    // ---- UI ----------------------------------------------------------------------------------

    private fun dp(v: Int) = (v * resources.displayMetrics.density).toInt()

    private fun label(text: String, bold: Boolean = false) = TextView(this).apply {
        this.text = text
        textSize = 16f
        if (bold) setTypeface(typeface, Typeface.BOLD)
        setPadding(0, dp(12), 0, dp(4))
    }

    private fun button(text: String, onClick: () -> Unit) = Button(this).apply {
        this.text = text
        isAllCaps = false
        setOnClickListener { onClick() }
    }

    private fun spinner(items: List<String>, selected: Int, onPick: (Int) -> Unit) = Spinner(this).apply {
        adapter = ArrayAdapter(this@MainActivity, android.R.layout.simple_spinner_dropdown_item, items)
        setSelection(selected)
        var first = true                                   // the spinner reports its starting value once
        onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(p: AdapterView<*>?, v: View?, pos: Int, id: Long) {
                if (first) { first = false; return }
                onPick(pos)
            }
            override fun onNothingSelected(p: AdapterView<*>?) {}
        }
    }

    private fun buildUi(): View {
        val root = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL; setPadding(dp(16), dp(16), dp(16), dp(24)) }

        status = TextView(this).apply { textSize = 18f; setTypeface(typeface, Typeface.BOLD) }
        root.addView(status)

        root.addView(label("Controller", bold = true))
        deviceList = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        root.addView(deviceList)
        root.addView(button("Scan for Bluetooth LE controllers") { scanBle() })

        root.addView(label("Instrument", bold = true))
        val names = instruments.map { "${it.group} / ${it.name}" }
        val savedInst = prefs.getInt("instrument", instruments.indexOfFirst { it.id == "strings/cello" }.coerceAtLeast(0))
        root.addView(spinner(names, savedInst) { i -> Native.nativeSetInstrument(i); prefs.edit().putInt("instrument", i).apply() })

        root.addView(label("Volume"))
        root.addView(SeekBar(this).apply {
            max = 100
            progress = (prefs.getFloat("volume", 0.8f) * 100).toInt()
            setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(s: SeekBar?, p: Int, fromUser: Boolean) {
                    Native.nativeSetVolume(p / 100f); prefs.edit().putFloat("volume", p / 100f).apply()
                }
                override fun onStartTrackingTouch(s: SeekBar?) {}
                override fun onStopTrackingTouch(s: SeekBar?) {}
            })
        })

        root.addView(label("Transpose"))
        transposeLabel = TextView(this).apply { textSize = 16f }
        val row = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        for ((text, delta) in listOf("-12" to -12, "-1" to -1, "+1" to 1, "+12" to 12))
            row.addView(button(text) { setTranspose(transpose + delta) }, LinearLayout.LayoutParams(0, -2, 1f))
        root.addView(row)
        root.addView(transposeLabel)

        root.addView(label("Bend range (match the controller; the Block sends ±48)"))
        val ranges = listOf(48f, 24f, 12f, 2f)
        root.addView(spinner(ranges.map { "±${it.toInt()} semitones" }, ranges.indexOf(prefs.getFloat("bend", 48f)).coerceAtLeast(0)) { i ->
            Native.nativeSetBendRange(ranges[i]); prefs.edit().putFloat("bend", ranges[i]).apply()
        })

        root.addView(label("Stuck-note watchdog (release notes if the link goes silent; use for Bluetooth)"))
        val dogs = listOf(0, 500, 1500, 3000)
        root.addView(spinner(dogs.map { if (it == 0) "off" else "$it ms" }, dogs.indexOf(prefs.getInt("watchdog", 0)).coerceAtLeast(0)) { i ->
            Native.nativeSetWatchdogMs(dogs[i]); prefs.edit().putInt("watchdog", dogs[i]).apply()
        })

        val actions = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        actions.addView(button("Panic") { Native.nativePanic() }, LinearLayout.LayoutParams(0, -2, 1f))
        actions.addView(button("Reset latency tuner") { Native.nativeResetLatencyTuner() }, LinearLayout.LayoutParams(0, -2, 1f))
        root.addView(actions)

        root.addView(label("Latency and load", bold = true))
        stats = TextView(this).apply { typeface = Typeface.MONOSPACE; textSize = 13f }
        root.addView(stats)

        // Android 15 draws apps edge to edge: keep the content clear of the bars and the cutout.
        val scroll = ScrollView(this).apply { addView(root) }
        scroll.setOnApplyWindowInsetsListener { v, insets ->
            val bars = insets.getInsets(WindowInsets.Type.systemBars() or WindowInsets.Type.displayCutout())
            v.setPadding(bars.left, bars.top + actionBarHeight(), bars.right, bars.bottom)
            insets
        }
        return scroll
    }

    private fun actionBarHeight(): Int {
        val tv = android.util.TypedValue()
        return if (actionBar?.isShowing == true && theme.resolveAttribute(android.R.attr.actionBarSize, tv, true))
            android.util.TypedValue.complexToDimensionPixelSize(tv.data, resources.displayMetrics) else 0
    }

    private fun applySavedSettings() {
        Native.nativeSetInstrument(prefs.getInt("instrument", instruments.indexOfFirst { it.id == "strings/cello" }.coerceAtLeast(0)))
        Native.nativeSetVolume(prefs.getFloat("volume", 0.8f))
        Native.nativeSetBendRange(prefs.getFloat("bend", 48f))
        Native.nativeSetWatchdogMs(prefs.getInt("watchdog", 0))
        setTranspose(prefs.getInt("transpose", 0))
    }

    private fun setTranspose(v: Int) {
        transpose = v.coerceIn(-48, 48)
        Native.nativeSetTranspose(transpose)
        prefs.edit().putInt("transpose", transpose).apply()
        transposeLabel.text = "$transpose semitones"
    }

    private fun showDevices() {
        deviceList.removeAllViews()
        val connected = MidiConnector.connectedName
        status.text = if (connected.isEmpty()) "No controller connected" else "Connected: $connected"
        if (connected.isNotEmpty()) deviceList.addView(button("Disconnect $connected") { MidiConnector.disconnect() })
        for (info in MidiConnector.devices().filter { it.id != MidiConnector.connectedId })
            deviceList.addView(button("Connect: ${MidiConnector.name(info)}") { MidiConnector.connect(info) })
        for (d in bleSeen.values)
            deviceList.addView(button("Connect (Bluetooth): ${d.name ?: d.address}") { MidiConnector.connectBluetooth(d) })
    }

    private fun scanBle() {
        if (!hasBlePermissions()) { requestNeededPermissions(); status.text = "Allow Bluetooth access, then scan again"; return }
        val scanner = ble ?: BleScanner(this) { d -> runOnUiThread { bleSeen[d.address] = d; showDevices() } }.also { ble = it }
        if (!scanner.available) { status.text = "Turn Bluetooth on first"; return }
        scanner.start()
        status.text = "Scanning for Bluetooth LE MIDI controllers..."
    }

    private fun updateStats() {
        val s = Stats.from(Native.nativeStats())
        val ms = { v: Double -> if (v.isNaN() || v < 0) "  n/a" else "%5.1f".format(v) }
        stats.text = buildString {
            appendLine("audio path        ${s.pathName}${if (s.exclusive) ", exclusive" else ", shared"}")
            appendLine("sample rate       ${s.sampleRate} Hz")
            appendLine("burst / buffer    ${s.burstFrames} / ${s.bufferFrames} frames  (${ms(1000.0 * s.bufferFrames / s.sampleRate.coerceAtLeast(1))} ms)")
            appendLine("underruns         ${s.xruns}   stream restarts ${s.streamRestarts}")
            appendLine()
            val played = s.events > 0                // no MIDI yet: the MIDI half is unknown, not zero
            appendLine("MIDI -> synth     avg ${if (played) ms(s.eventAgeAvgMs) else "  n/a"} ms   max ${if (played) ms(s.eventAgeMaxMs) else "  n/a"} ms")
            appendLine("synth -> speaker  ${ms(s.outputLatencyMs)} ms   (buffer + hardware)")
            appendLine("total             ${if (played) ms(s.totalMs) else "  n/a"} ms   (from Android receiving the note)")
            appendLine()
            appendLine("cpu load          avg ${"%.1f".format(s.loadAvg * 100)}%   peak ${"%.1f".format(s.loadPeak * 100)}%")
            appendLine("voices            ${s.voices}")
            appendLine("events            ${s.events}   dropped ${s.dropped}   panics ${s.panics}")
            append("midi bytes        ${s.midiBytes}")
        }
    }
}
