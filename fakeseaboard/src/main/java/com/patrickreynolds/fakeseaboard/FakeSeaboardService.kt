package com.patrickreynolds.fakeseaboard

import android.media.midi.MidiDeviceService
import android.media.midi.MidiReceiver
import android.util.Log

/** The virtual device. Android binds it when an app opens "Seaboard BLOCK (emulated)". */
class FakeSeaboardService : MidiDeviceService() {
    override fun onCreate() {
        super.onCreate()
        instance = this
        Log.i(TAG, "device service created")
    }

    override fun onDestroy() {
        Player.stop(releaseNotes = false)
        instance = null
        super.onDestroy()
    }

    override fun onGetInputPortReceivers(): Array<MidiReceiver> = emptyArray()   // the controller only sends

    /** Sends one message to every app that has our output port open. */
    fun send(bytes: ByteArray, timestampNs: Long = System.nanoTime()) {
        val out = outputPortReceivers.firstOrNull() ?: return
        out.send(bytes, 0, bytes.size, timestampNs)
    }

    companion object {
        const val TAG = "fakeseaboard"
        @Volatile var instance: FakeSeaboardService? = null
    }
}
