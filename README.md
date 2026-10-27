# mpe-instruments-android

An Android app that plays the Faust instruments from `mpe-instruments` from a
Seaboard BLOCK (or any MPE controller) over USB or Bluetooth LE. It also keeps
the MIDI stream clean, so a lost link never leaves a stuck note (the logic from
`seaboard-mpe`).

```
Seaboard -> android.media.midi -> core (clean) -> voice engine (Faust) -> Oboe -> speaker
```

Status: the engine (`engine/`) is built and tested on the desktop. The Android app
(`app/`) is written but has **never been compiled or run**: it was written on a machine without
the Android SDK. Expect a first build to need small fixes. See [LATENCY.md](LATENCY.md) for the
low-latency design and what still has to be measured on a real phone.

## Build and test

Engine, on the desktop (needs `cmake`, a C++17 compiler, and `faust`):

    git clone --recurse-submodules https://github.com/PatrickReynoldsCoding/mpe-instruments-android
    cd mpe-instruments-android/engine
    cmake -S . -B build && cmake --build build -j && (cd build && ctest --output-on-failure)
    ./build/tools/bench 8 96 3        # cost of 8 fingers, 96-frame callbacks

Android app (needs Android Studio or the command-line SDK with NDK, JDK 17, and `faust` on the
machine running Gradle, because the build turns each `.dsp` into C++):

    gradle wrapper --gradle-version 8.9      # once; the wrapper is not committed
    ./gradlew :app:assembleDebug

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
| App on Android (JNI, service, UI, audio output starts) | Android emulator (x86_64, KVM). A debug build has a virtual MIDI device and an `adb` command that injects MIDI bytes. Audio is dumped to a file so a test can check it | emulator |
| USB permission, real Bluetooth, real latency | Real phone with the Seaboard | phone |

Emulator limits, so nothing surprises us: it cannot see the Seaboard over USB or
Bluetooth, and its audio latency says nothing about a phone's.
