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
    private val autoNames = listOf("seaboard", "roli", "block", "lightpad")

    private lateinit var manager: MidiManager
    private val handler = Handler(Looper.getMainLooper())
    private var current: MidiDevice? = null
    private var currentId = -1
    private var started = false

    /** Name shown in the UI, "" when nothing is connected. */
    @Volatile var connectedName: String = ""
        private set
    /** Called on the main thread when devices come, go or connect. */
    var onChange: (() -> Unit)? = null

    fun init(context: Context) {
        if (started) return
        started = true
        manager = context.applicationContext.getSystemService(Context.MIDI_SERVICE) as MidiManager
        manager.registerDeviceCallback(object : MidiManager.DeviceCallback() {
            override fun onDeviceAdded(info: MidiDeviceInfo) {
                if (current == null && info.outputPortCount > 0 && looksLikeController(info)) connect(info)
                onChange?.invoke()
            }
            override fun onDeviceRemoved(info: MidiDeviceInfo) {
                if (info.id == currentId) disconnect()
                onChange?.invoke()
            }
        }, handler)
        // A controller that is already plugged in.
        devices().firstOrNull { looksLikeController(it) }?.let { connect(it) }
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
        manager.openBluetoothDevice(device, { midi ->
            if (midi == null) { Log.w(TAG, "could not open Bluetooth MIDI device"); return@openBluetoothDevice }
            val info = midi.info
            val port = info.ports.firstOrNull { it.type == MidiDeviceInfo.PortInfo.TYPE_OUTPUT }
            if (port == null) { midi.close(); return@openBluetoothDevice }
            attach(midi, info, port.portNumber)
        }, handler)
    }

    private fun attach(device: MidiDevice, info: MidiDeviceInfo, portNumber: Int) {
        disconnect()
        if (Native.nativeAttachMidi(device, portNumber)) {
            current = device
            currentId = info.id
            connectedName = name(info)
        } else {
            device.close()
        }
        onChange?.invoke()
    }

    fun disconnect() {
        Native.nativeDetachMidi()          // releases held notes, then closes the native port
        try { current?.close() } catch (_: Exception) {}
        current = null
        currentId = -1
        connectedName = ""
        onChange?.invoke()
    }
}
