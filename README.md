# mpe-instruments-android

An Android app that plays the Faust instruments from `mpe-instruments` from a
Seaboard BLOCK (or any MPE controller) over USB or Bluetooth LE. It also keeps
the MIDI stream clean, so a lost link never leaves a stuck note (the logic from
`seaboard-mpe`).

```
Seaboard -> android.media.midi -> core (clean) -> voice engine (Faust) -> Oboe -> speaker
```

Status: planning. Nothing is built yet.

## Layout (planned)

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
