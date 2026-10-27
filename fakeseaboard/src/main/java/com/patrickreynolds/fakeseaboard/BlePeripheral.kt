package com.patrickreynolds.fakeseaboard

import android.annotation.SuppressLint
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothGattServer
import android.bluetooth.BluetoothGattServerCallback
import android.bluetooth.BluetoothGattService
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.bluetooth.le.AdvertiseCallback
import android.bluetooth.le.AdvertiseData
import android.bluetooth.le.AdvertiseSettings
import android.content.Context
import android.os.ParcelUuid
import android.util.Log
import java.util.UUID

/**
 * The fake Seaboard as a Bluetooth LE MIDI device (the BLE-MIDI 1.0 spec): a GATT server with
 * the MIDI service and its one characteristic, advertised so a phone can find it. Messages are
 * sent as notifications in the spec's packet format.
 */
@SuppressLint("MissingPermission")
object BlePeripheral {
    private const val TAG = FakeSeaboardService.TAG
    val SERVICE: UUID = UUID.fromString("03B80E5A-EDE8-4B33-A751-6CE34EC4C700")
    val CHARACTERISTIC: UUID = UUID.fromString("7772E5DB-3868-4112-A1A9-F2669D106BF3")
    private val CCCD: UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

    private var server: BluetoothGattServer? = null
    private var characteristic: BluetoothGattCharacteristic? = null
    private val subscribed = java.util.concurrent.CopyOnWriteArraySet<BluetoothDevice>()
    private var manager: BluetoothManager? = null
    @Volatile private var mtu = 23

    val connected get() = subscribed.isNotEmpty()

    private val advertiseCallback = object : AdvertiseCallback() {
        override fun onStartSuccess(settingsInEffect: AdvertiseSettings) { Log.i(TAG, "BLE advertising") }
        override fun onStartFailure(errorCode: Int) { Log.w(TAG, "BLE advertising failed: $errorCode") }
    }

    fun start(context: Context) {
        // Always start fresh: after Bluetooth is switched off and on, an old server is dead.
        if (server != null) stop()
        val m = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        manager = m
        val adapter = m.adapter ?: run { Log.w(TAG, "no Bluetooth adapter"); return }
        adapter.name = "Seaboard BLOCK (emulated BLE)"
        val c = BluetoothGattCharacteristic(CHARACTERISTIC,
            BluetoothGattCharacteristic.PROPERTY_READ or BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE or
                BluetoothGattCharacteristic.PROPERTY_NOTIFY,
            BluetoothGattCharacteristic.PERMISSION_READ or BluetoothGattCharacteristic.PERMISSION_WRITE)
        c.addDescriptor(BluetoothGattDescriptor(CCCD, BluetoothGattDescriptor.PERMISSION_READ or BluetoothGattDescriptor.PERMISSION_WRITE))
        val svc = BluetoothGattService(SERVICE, BluetoothGattService.SERVICE_TYPE_PRIMARY)
        svc.addCharacteristic(c)
        characteristic = c
        server = m.openGattServer(context, callback)?.also { it.addService(svc) }
        adapter.bluetoothLeAdvertiser?.startAdvertising(
            AdvertiseSettings.Builder().setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY)
                .setConnectable(true).setTxPowerLevel(AdvertiseSettings.ADVERTISE_TX_POWER_HIGH).build(),
            AdvertiseData.Builder().addServiceUuid(ParcelUuid(SERVICE)).build(),
            AdvertiseData.Builder().setIncludeDeviceName(true).build(),
            advertiseCallback)
    }

    /** Drop the link, like walking out of range. */
    fun stop() {
        manager?.adapter?.bluetoothLeAdvertiser?.stopAdvertising(advertiseCallback)
        server?.let { s -> subscribed.forEach { s.cancelConnection(it) }; s.close() }
        server = null
        subscribed.clear()
        Log.i(TAG, "BLE stopped")
    }

    private val callback = object : BluetoothGattServerCallback() {
        override fun onConnectionStateChange(device: BluetoothDevice, status: Int, newState: Int) {
            Log.i(TAG, "BLE ${if (newState == BluetoothProfile.STATE_CONNECTED) "connected" else "disconnected"}: ${device.address}")
            if (newState != BluetoothProfile.STATE_CONNECTED) subscribed.remove(device)
        }
        override fun onMtuChanged(device: BluetoothDevice, newMtu: Int) { mtu = newMtu; Log.i(TAG, "BLE mtu $newMtu") }
        override fun onCharacteristicReadRequest(device: BluetoothDevice, requestId: Int, offset: Int, ch: BluetoothGattCharacteristic) {
            server?.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, 0, ByteArray(0))   // spec: read returns no payload
        }
        override fun onCharacteristicWriteRequest(device: BluetoothDevice, requestId: Int, ch: BluetoothGattCharacteristic,
                                                  preparedWrite: Boolean, responseNeeded: Boolean, offset: Int, value: ByteArray) {
            if (responseNeeded) server?.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, 0, null)
        }
        override fun onDescriptorReadRequest(device: BluetoothDevice, requestId: Int, offset: Int, d: BluetoothGattDescriptor) {
            server?.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, 0,
                if (device in subscribed) BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE else BluetoothGattDescriptor.DISABLE_NOTIFICATION_VALUE)
        }
        override fun onDescriptorWriteRequest(device: BluetoothDevice, requestId: Int, d: BluetoothGattDescriptor,
                                              preparedWrite: Boolean, responseNeeded: Boolean, offset: Int, value: ByteArray) {
            if (d.uuid == CCCD) {
                if (value.contentEquals(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)) subscribed.add(device) else subscribed.remove(device)
                Log.i(TAG, "BLE notifications ${if (device in subscribed) "on" else "off"} for ${device.address}")
            }
            if (responseNeeded) server?.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, 0, null)
        }
    }

    /**
     * One BLE-MIDI packet per message: header (1, 0, timestamp bits 12..7), then
     * timestamp low (1, bits 6..0), then the full MIDI message. Timestamp = ms, 13 bits.
     */
    fun send(bytes: ByteArray) {
        val s = server ?: return
        val c = characteristic ?: return
        if (subscribed.isEmpty()) return
        val ms = (System.nanoTime() / 1_000_000L).toInt() and 0x1FFF
        val packet = ByteArray(2 + bytes.size)
        packet[0] = (0x80 or ((ms shr 7) and 0x3F)).toByte()
        packet[1] = (0x80 or (ms and 0x7F)).toByte()
        System.arraycopy(bytes, 0, packet, 2, bytes.size)
        for (d in subscribed) s.notifyCharacteristicChanged(d, c, false, packet)
    }
}
