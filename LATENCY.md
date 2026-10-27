# Latency

Goal: show that an Android phone can play an MPE controller with latency close to
iOS and macOS. This file says what the app does about it, what is measured, and what
has **not** been measured yet. Nothing here claims a number that was not measured.

## Where the time goes

```
finger -> Seaboard scan ----- USB or BLE ----- Android MIDI service -> [app callback] -> audio buffer -> DAC -> air
 (device)        (transport)                         (OS)                 (us)         (OS + hardware)
```

| Stage | Who controls it | What the app does |
|---|---|---|
| Controller scan time | the device | nothing to do |
| USB transfer | the USB spec (1 ms polling at full speed, 125 us at high speed) | nothing to do |
| BLE transfer | the BLE connection interval, set by the phone and the device | see "Bluetooth" below |
| Android MIDI service -> app | Android | read the port from the audio thread (below) |
| Wait for the next audio callback | burst size | smallest burst the device gives |
| Our rendering | us | Faust code; measured, about 1-10% of a callback on the desktop (see `engine/tools/bench`) |
| Audio buffer and hardware | Android audio HAL | Oboe low-latency + exclusive (MMAP), native rate, 2 bursts, auto-tuned |

## What the app does

1. **Audio output: Oboe, `PerformanceMode::LowLatency`, `SharingMode::Exclusive`, `Usage::Game`.**
   Per Google's Oboe guide, the default mode can cost over 200 ms and shared mode adds to
   the buffer. If the device refuses exclusive mode the stream still opens, in shared mode;
   the status screen says which one was granted and whether MMAP is in use.
2. **Native sample rate.** No `setSampleRate`. Forcing 44.1 kHz on a 48 kHz device sends audio
   through a slower path. The engine takes whatever rate the stream gives it.
3. **Small buffer, tuned automatically.** The buffer starts at 2 bursts. Oboe's `LatencyTuner`
   lowers it as far as it can without underruns and raises it if one happens. "Reset latency
   tuner" starts the search again.
4. **MIDI is read inside the audio callback**, with the NDK AMidi API (`AMidiOutputPort_receive`
   is non-blocking, and Google says it is safe in an audio callback). There is no separate MIDI
   thread, no queue between threads on the hot path, and no Java or JNI call per message. A
   message waits at most until the next callback, which is one burst.
5. **Callback rules.** The engine never allocates, locks, or logs in `render`. Instrument
   switching is built off the audio thread and swapped in with an atomic pointer.
6. **No Java `MidiReceiver`.** That path goes through Binder, a Java thread and the garbage
   collector, and is the usual source of "Android MIDI is laggy" reports.

## What the status screen measures

| Line | Meaning |
|---|---|
| MIDI -> synth | time from the timestamp Android put on the message to the start of the callback that played it. Includes the wait for the callback. |
| synth -> speaker | Oboe's `calculateLatencyMillis()`: buffer plus hardware, from the stream's own timestamps |
| total | the two added. This is the latency that this app and Android add. |
| underruns | glitches. If this grows, the tuner raises the buffer. |
| cpu load | time in `render` divided by the audio produced. Over 100% means dropouts. |

The engine tests check the first line's logic: an event that arrives during a callback is played
by the next one, so the engine adds at most one callback (`test_engine`, "engine adds at most one
callback").

## What is NOT measured yet, and how to measure it

- **The whole chain, finger to sound.** The total above starts when Android receives the note.
  It leaves out the controller's scan time and the USB or BLE transfer. A true figure needs an
  outside measurement: record the key strike and the sound with one microphone or a second
  phone's slow-motion video, and count the frames. Do this for USB and for BLE, on at least
  two phones, and write the numbers in the table below.
- **Whether the AMidi timestamp is the arrival time we assume.** The app uses it only if it is
  within one second of "now", otherwise it counts the message as arriving at the callback.
  Check on a real phone that "MIDI -> synth" is a believable few milliseconds and not zero.
- **Whether exclusive/MMAP is granted** on the target phone. The status screen shows it.
- **Bluetooth connection interval.** With the framework's BLE MIDI the app cannot set it.
  Compare against the same controller on iOS if you can.

## Results

| Phone | Android | Link | Audio path | burst/buffer | MIDI -> synth | synth -> speaker | finger -> sound (outside) |
|---|---|---|---|---|---|---|---|
| (not yet measured) | | | | | | | |

## Ideas, not done

- Android Dynamic Performance Framework (`PerformanceHintManager`) hints for the audio thread, API 33+.
- Splitting the callback at the event's sample position so a note starts mid-burst, not at the start of it.
- A full-duplex Oboe loopback test (OboeTester has one) to give the audio-only round trip for each phone.
- Compare `Usage::Game` and `Usage::Media`.
