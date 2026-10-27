package com.patrickreynolds.mpeinstruments

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothManager
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.ParcelUuid
import java.util.UUID

/** Looks for Bluetooth LE MIDI devices (the standard BLE MIDI service). Needs BLUETOOTH_SCAN. */
@SuppressLint("MissingPermission")
class BleScanner(context: Context, private val onFound: (BluetoothDevice) -> Unit) {
    private val adapter: BluetoothAdapter? =
        (context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager).adapter
    private var scanning = false

    private val callback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) { onFound(result.device) }
    }

    val available get() = adapter?.isEnabled == true

    fun start() {
        val scanner = adapter?.bluetoothLeScanner ?: return
        if (scanning) return
        val filter = ScanFilter.Builder().setServiceUuid(ParcelUuid(MIDI_SERVICE)).build()
        val settings = ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build()
        scanner.startScan(listOf(filter), settings, callback)
        scanning = true
    }

    fun stop() {
        if (!scanning) return
        adapter?.bluetoothLeScanner?.stopScan(callback)
        scanning = false
    }

    companion object {
        val MIDI_SERVICE: UUID = UUID.fromString("03B80E5A-EDE8-4B33-A751-6CE34EC4C700")
    }
}
