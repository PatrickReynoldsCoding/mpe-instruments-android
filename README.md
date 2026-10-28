# mpe-instruments-android

An Android app that plays the Faust instruments from `mpe-instruments` from a
Seaboard BLOCK (or any MPE controller) over USB or Bluetooth LE, with low latency: about
25 ms from note to sound on a POCO F2 Pro, measured ([LATENCY.md](LATENCY.md)).

```
Seaboard -> android.media.midi -> core (clean) -> parts (play / arp) -> Faust voices -> effects -> Oboe
```

What it does:

- **14 physical-model and synth instruments**, each finger its own voice (MPE): pitch glides,
  pressure swells, slide shapes the tone.
- **Split and layer**: two instruments, left and right of a split point (or "learn" it from a
  key), or both at once with their own octave and level.
- **An MPE arpeggiator** per part: up, down, up-down, as played, random, chord; rate, gate,
  octaves, latch. Each arpeggiated note keeps following the finger that holds it, so pressing
  swells the arpeggio and sliding changes its tone. Arpeggiate the left hand, play the right.
- **Pitch assist**: a finger that comes to rest is drawn onto the nearest note of a scale;
  glides and vibrato are left alone.
- **Sound**: per part brightness, drive, touch response, reverb and delay sends; a Zita reverb
  and a tempo-synced ping-pong delay. Tap tempo.
- **Presets**: ten built in, save your own.
- **A fullscreen touch pad**: play with your fingers, no controller needed. Columns of notes in
  a key and scale, slide to glide, higher up for louder and brighter; each finger its own MPE
  voice, so the arp, split and effects apply. A Seaboard played at the same time shows there too.
- **An octave warning** when the controller's notes are far from the instrument's range.
- Bluetooth reconnects by itself (can be turned off); Stop turns everything off.

Status: the engine is tested on the desktop (sanitizers included), the app on an emulator with
an emulated Seaboard over USB and Bluetooth LE, and on a real phone with the real Seaboard over
USB. Bluetooth with the real Seaboard is not tested yet.

## Install

Download `MPE-Instruments-<version>.apk` from the [Releases](../../releases) page on the phone and
open it (allow installing from your browser or file manager when Android asks). Android 12 or
newer.

## Build and test

Engine, on the desktop (needs `cmake`, a C++17 compiler, and `faust`):

    git clone --recurse-submodules https://github.com/PatrickReynoldsCoding/mpe-instruments-android
    cd mpe-instruments-android/engine
    cmake -S . -B build && cmake --build build -j && (cd build && ctest --output-on-failure)
    ./build/tools/bench 8 96 3        # cost of 8 fingers, 96-frame callbacks

Android app (needs Android Studio or the command-line SDK with NDK, JDK 17, and `faust` on the
machine running Gradle, because the build turns each `.dsp` into C++):

    source tools/env.sh                      # paths of the local toolchain; edit for your machine
    gradle assembleDebug                     # the app, and the fake Seaboard (fakeseaboard/)
    gradle assembleRelease                   # signed if a key is set up (MPE_SIGNING, see app/build.gradle.kts)

End to end on an emulator (boot one first, for example
`emulator -avd <name> -no-window -no-audio`):

    python3 -I tools/emulator_test.py

Bluetooth LE MIDI, with a second emulator as the Seaboard (emulators on one machine share a
virtual Bluetooth radio):

    emulator -avd <name2> -port 5556 -no-window -no-audio &
    python3 -I tools/emulator_test.py --serial emulator-5554 --ble-peer emulator-5556

`fakeseaboard/` is a small debug app that registers a virtual MIDI device called "Seaboard BLOCK
(emulated)". The app connects to it exactly as it would to the real controller (MidiManager,
then AMidi in the audio callback). The test drives it over adb: known notes on several
instruments (the app records its own output and the pitch is measured), 30 s of the recorded
session, a Bluetooth-style drop with the watchdog on and off, unplug and replug mid-note,
instrument switching, screen off and Doze, the audio server dying, a stress run, and an app
restart. In `--ble-peer` mode the same app is a Bluetooth LE MIDI device (the BLE-MIDI GATT
service) on the other emulator: scan and connect, notes and the session over Bluetooth, the link
lost mid-note, and automatic reconnection. The debug build of the app has a `DebugReceiver` for
this; release builds do not include it.

Minimum Android 12 (API 31). Instruments are compiled in from the `third_party/mpe-instruments`
submodule: add a `.dsp` there and rebuild.

## Layout

```
engine/    C++: clean-up core + voice engine + generated Faust instruments. No Android code.
app/       Kotlin: MIDI devices, service, UI. Calls engine through JNI.
tests/     host tests for engine; emulator tests for app
captures/  recorded Seaboard MIDI to replay in tests
```

## Test plan: what runs where

The aim is that the phone is only needed for the last two rows.

| Layer | How it is tested | Needs |
|---|---|---|
| Clean-up logic (held notes, panic, silence watchdog) | Unit tests on the desktop; engine is plain C++ | nothing |
| Voice engine and instruments | Desktop build renders audio to a file; test checks pitch, level, no clicks, no hanging notes (as `tests/test_instruments.py` does in `mpe-instruments`) | nothing |
| Real Seaboard behaviour | Replay a recorded capture (from `aseqdump`) through the engine; same file every run | one capture |
| App on Android (MIDI connection, JNI, service, audio, safety) | `tools/emulator_test.py` with the fake Seaboard; the app records its own output and the test measures it | emulator |
| USB permission, real Bluetooth, real latency | Real phone with the Seaboard | phone |

Emulator limits, so nothing surprises us: it cannot see the Seaboard over USB or
Bluetooth, and its audio latency says nothing about a phone's.
