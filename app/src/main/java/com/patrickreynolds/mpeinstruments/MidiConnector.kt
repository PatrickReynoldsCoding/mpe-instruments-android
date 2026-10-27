package com.patrickreynolds.mpeinstruments

import android.annotation.SuppressLint
import android.bluetooth.BluetoothDevice
import android.content.Context
import android.media.midi.MidiDevice
import android.media.midi.MidiDeviceInfo
import android.media.midi.MidiManager
import android.os.Handler
import android.os.Looper
import android.util.Log

/**
 * Finds MIDI controllers (USB, paired or Bluetooth LE), opens one, and hands it to the C++ side.
 * After that no MIDI byte passes through Kotlin: the audio callback reads the port itself.
 */
@SuppressLint("MissingPermission")
object MidiConnector {
    private const val TAG = "mpe"
    private const val AUTO_BLE_WATCHDOG_MS = 1500
    private val autoNames = listOf("seaboard", "roli", "block", "lightpad")

    private lateinit var manager: MidiManager
    private lateinit var appContext: Context
    private val handler = Handler(Looper.getMainLooper())
    private var current: MidiDevice? = null
    private var currentId = -1
    private var currentIsBle = false
    private var started = false
    // Bluetooth MIDI devices only exist once an app opens them, so after a dropout nothing
    // brings them back by itself. Remember the last one and scan for it until it returns.
    private var reconnectScanner: BleScanner? = null
    private val prefs by lazy { appContext.getSharedPreferences("midi", Context.MODE_PRIVATE) }

    /**
     * Stuck-note watchdog setting in ms; -1 = auto. Auto is on for Bluetooth only: a lost BLE
     * link can take Android 30 s to report (measured on the emulator), a pulled USB cable is
     * reported at once.
     */
    var watchdogSetting: Int
        get() = appContext.getSharedPreferences("mpe", Context.MODE_PRIVATE).getInt("watchdog", -1)
        set(v) {
            appContext.getSharedPreferences("mpe", Context.MODE_PRIVATE).edit().putInt("watchdog", v).apply()
            applyWatchdog()
        }

    private fun applyWatchdog() {
        val v = watchdogSetting
        Native.nativeSetWatchdogMs(if (v >= 0) v else if (currentIsBle) AUTO_BLE_WATCHDOG_MS else 0)
    }

    /** MidiDeviceInfo.id of the connected device, -1 when none. */
    val connectedId get() = currentId

    /** Name shown in the UI, "" when nothing is connected. */
    @Volatile var connectedName: String = ""
        private set
    /** Called on the main thread when devices come, go or connect. */
    var onChange: (() -> Unit)? = null

    fun init(context: Context) {
        if (started) return
        started = true
        appContext = context.applicationContext
        manager = appContext.getSystemService(Context.MIDI_SERVICE) as MidiManager
        applyWatchdog()
        manager.registerDeviceCallback(object : MidiManager.DeviceCallback() {
            override fun onDeviceAdded(info: MidiDeviceInfo) {
                // A Bluetooth device only appears here because something just opened it (we did, in
                // connectBluetooth): opening it again would race that and replace it.
                if (current == null && info.type != MidiDeviceInfo.TYPE_BLUETOOTH && info.outputPortCount > 0 &&
                    looksLikeController(info)) connect(info)
                onChange?.invoke()
            }
            override fun onDeviceRemoved(info: MidiDeviceInfo) {
                if (info.id == currentId) {
                    val wasBle = currentIsBle
                    detach()
                    if (wasBle) startBleReconnect()          // the link dropped; the user did not ask to disconnect
                }
                onChange?.invoke()
            }
        }, handler)
        // A controller that is already plugged in, else the Bluetooth one used last time.
        val plugged = devices().firstOrNull { it.type != MidiDeviceInfo.TYPE_BLUETOOTH && looksLikeController(it) }
        if (plugged != null) connect(plugged) else startBleReconnect()
    }

    /** "Waiting for <name>" while a Bluetooth controller is being looked for, else "". */
    val reconnecting get() = if (reconnectScanner != null) prefs.getString("ble_name", "") ?: "" else ""

    private fun startBleReconnect() {
        val address = prefs.getString("ble_address", null) ?: return
        val btName = prefs.getString("ble_bt_name", null)
        if (reconnectScanner != null || current != null) return
        val granted = appContext.checkSelfPermission(android.Manifest.permission.BLUETOOTH_SCAN) ==
            android.content.pm.PackageManager.PERMISSION_GRANTED
        if (!granted) return
        val scanner = BleScanner(appContext) { device ->
            // Match the address, or the name: many devices use a random address that changes when
            // their Bluetooth restarts.
            val sameName = btName != null && device.name == btName
            if (device.address == address || sameName) handler.post {
                stopBleReconnect()
                Log.i(TAG, "Bluetooth controller is back, reconnecting")
                connectBluetooth(device)
            }
        }
        if (!scanner.available) return
        reconnectScanner = scanner
        scanner.start()
        Log.i(TAG, "waiting for Bluetooth controller $address")
        onChange?.invoke()
    }

    private fun stopBleReconnect() {
        reconnectScanner?.stop()
        reconnectScanner = null
    }

    fun name(info: MidiDeviceInfo): String =
        info.properties.getString(MidiDeviceInfo.PROPERTY_NAME) ?: "MIDI device ${info.id}"

    private fun looksLikeController(info: MidiDeviceInfo): Boolean {
        val n = name(info).lowercase()
        return autoNames.any { n.contains(it) }
    }

    /** Devices that send us MIDI (they have an output port from the app's point of view). */
    fun devices(): List<MidiDeviceInfo> = manager.devices.filter { it.outputPortCount > 0 }

    fun connect(info: MidiDeviceInfo) {
        val port = info.ports.firstOrNull { it.type == MidiDeviceInfo.PortInfo.TYPE_OUTPUT } ?: return
        manager.openDevice(info, { device ->
            if (device == null) { Log.w(TAG, "could not open ${name(info)}"); return@openDevice }
            attach(device, info, port.portNumber)
        }, handler)
    }

    fun connectBluetooth(device: BluetoothDevice) {
        stopBleReconnect()
        manager.openBluetoothDevice(device, { midi ->
            if (midi == null) { Log.w(TAG, "could not open Bluetooth MIDI device"); return@openBluetoothDevice }
            val info = midi.info
            val port = info.ports.firstOrNull { it.type == MidiDeviceInfo.PortInfo.TYPE_OUTPUT }
            if (port == null) { midi.close(); return@openBluetoothDevice }
            attach(midi, info, port.portNumber)
            if (current === midi)
                prefs.edit().putString("ble_address", device.address).putString("ble_name", connectedName)
                    .putString("ble_bt_name", device.name).apply()
        }, handler)
    }

    private fun attach(device: MidiDevice, info: MidiDeviceInfo, portNumber: Int) {
        detach()
        stopBleReconnect()
        if (Native.nativeAttachMidi(device, portNumber)) {
            current = device
            currentId = info.id
            currentIsBle = info.type == MidiDeviceInfo.TYPE_BLUETOOTH
            connectedName = name(info)
            applyWatchdog()
        } else {
            device.close()
        }
        onChange?.invoke()
    }

    /** The user pressed Disconnect: also forget the Bluetooth controller, so it is not reconnected. */
    fun disconnect() {
        if (currentIsBle) prefs.edit().remove("ble_address").remove("ble_name").remove("ble_bt_name").apply()
        stopBleReconnect()
        detach()
    }

    /** The app is shutting down: close the device but remember it for next time. */
    fun release() {
        stopBleReconnect()
        detach()
    }

    private fun detach() {
        Native.nativeDetachMidi()          // releases held notes, then closes the native port
        try { current?.close() } catch (_: Exception) {}
        current = null
        currentId = -1
        currentIsBle = false
        connectedName = ""
        onChange?.invoke()
    }
}
