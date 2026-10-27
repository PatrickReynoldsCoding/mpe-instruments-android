# mpe-instruments-android

An Android app that plays the Faust instruments from `mpe-instruments` from a
Seaboard BLOCK (or any MPE controller) over USB or Bluetooth LE. It also keeps
the MIDI stream clean, so a lost link never leaves a stuck note (the logic from
`seaboard-mpe`).

```
Seaboard -> android.media.midi -> core (clean) -> voice engine (Faust) -> Oboe -> speaker
```

Status: the engine is tested on the desktop, and the app builds and passes end-to-end tests on
an Android emulator with an emulated Seaboard. It has **not yet run on a real phone or with the
real controller**. See [LATENCY.md](LATENCY.md) for the low-latency design and what still has
to be measured on a phone.

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

End to end on an emulator (boot one first, for example
`emulator -avd <name> -no-window -no-audio`):

    python3 -I tools/emulator_test.py

`fakeseaboard/` is a small debug app that registers a virtual MIDI device called "Seaboard BLOCK
(emulated)". The app connects to it exactly as it would to the real controller (MidiManager,
then AMidi in the audio callback). The test drives it over adb: known notes on several
instruments (the app records its own output and the pitch is measured), 30 s of the recorded
session, a Bluetooth-style drop with the watchdog on and off, unplug and replug mid-note,
instrument switching and an app restart. The debug build of the app has a `DebugReceiver` for
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
