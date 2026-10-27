# First run on a real phone

About 20 minutes. Everything before this was tested on the emulator with a fake Seaboard
(`tools/emulator_test.py`, 43 checks), so this is about the parts an emulator cannot show:
the real USB and Bluetooth paths, the phone's audio latency, and how it feels to play.

You need: the phone, a USB-C to USB-C cable **that carries data** (one you have used to copy
files), and the Seaboard charged. A USB-C OTG adapter only if the plain cable does not work.

## 1. Once: USB debugging

Settings → About phone → tap **Build number** 7 times. Then Settings → System → Developer
options → **USB debugging** on.

## 2. Phone plugged into the computer

    tools/phone.sh check        # what the phone offers (low-latency flag, MMAP policy)
    tools/phone.sh install      # app + fake Seaboard
    tools/phone.sh fake-test    # the emulator suite, on the phone (it will make sound)

`fake-test` checks the whole app on the phone's real audio hardware before the Seaboard is
involved. If something fails here, it is the phone's audio, not the controller.

    tools/phone.sh report       # saves the numbers to captures/

## 3. Seaboard over USB

Unplug the phone from the computer. Plug the Seaboard into the phone. Open the app.

- It should say **Connected: Seaboard BLOCK** by itself, within a second or two.
- Play. Try every instrument for a moment, chords, slides, pressure.
- Pull the cable mid-note: the sound must stop.
- Screenshot the "Latency and load" section while playing.

If it never says Connected: try the OTG adapter. If the app does not list the Seaboard at all,
the cable is probably charge-only.

## 4. Seaboard over Bluetooth

Turn the Seaboard's Bluetooth on (see its manual). In the app: **Scan for Bluetooth LE
controllers**, then tap the Seaboard. Set the watchdog to **1500 ms**. Play, and screenshot
the numbers again. Walk away with the Seaboard until the link drops: the sound must stop.

## 5. Finger to sound, measured from outside

The app measures from "Android received the note" to "sound leaves the buffer". It cannot see
the Seaboard's own scan time or the USB/Bluetooth transfer. To get the real figure:

1. Instrument **struck / Marimba** (a sharp attack is easy to see).
2. A second phone filming in slow motion (240 fps if possible), close to the Seaboard and to
   the playing phone's speaker.
3. Tap one key firmly, about 10 times, a second apart.
4. Do it over USB, then over Bluetooth. If you can, also with the Seaboard on a Mac or iPad
   playing a similar sound: that is the comparison that answers "is Android slower?".

Send the videos; the gap between the finger landing and the sound is counted frame by frame
(at 240 fps one frame is about 4 ms).

## What to send back

- The `captures/phone-*.txt` file from step 2 (it is saved in the repo folder).
- Screenshots from steps 3 and 4.
- Anything that felt wrong: notes that hang, late or uneven notes, crackles.
- The videos from step 5.
