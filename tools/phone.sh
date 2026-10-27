#!/usr/bin/env bash
# Helpers for the first runs on a real phone. Phone plugged into this computer with USB debugging on.
#   tools/phone.sh check      what the phone offers for low-latency audio and MIDI
#   tools/phone.sh install    install the app (debug build) and the fake Seaboard
#   tools/phone.sh fake-test  the full emulator test suite, on the phone, with the fake Seaboard
#   tools/phone.sh stats      the app's latency/load numbers right now
#   tools/phone.sh log        follow the app's log (Ctrl+C to stop)
#   tools/phone.sh report     save device info + stats to captures/phone-<model>-<date>.txt
set -euo pipefail
cd "$(dirname "$0")/.."
source tools/env.sh
APP=com.patrickreynolds.mpeinstruments
SERIAL=$(adb devices | awk 'NR>1 && $2=="device" && $1 !~ /^emulator-/ {print $1; exit}')
[ -n "$SERIAL" ] || { echo "No phone found. Plug it in, unlock it and accept 'Allow USB debugging'."; adb devices; exit 1; }
A=(adb -s "$SERIAL")

info() {
    echo "phone:      $("${A[@]}" shell getprop ro.product.manufacturer) $("${A[@]}" shell getprop ro.product.model)"
    echo "android:    $("${A[@]}" shell getprop ro.build.version.release) (API $("${A[@]}" shell getprop ro.build.version.sdk))"
    echo "features:   $("${A[@]}" shell pm list features | grep -oE 'android\.(hardware\.audio\.(low_latency|pro)|software\.midi|hardware\.usb\.host|hardware\.bluetooth_le)' | sort | tr '\n' ' ')"
    echo "mmap:       policy=$("${A[@]}" shell getprop aaudio.mmap_policy) exclusive_policy=$("${A[@]}" shell getprop aaudio.mmap_exclusive_policy)  (2 = allowed, 3 = always; empty = vendor default)"
    echo "hw burst:   $("${A[@]}" shell getprop aaudio.hw_burst_min_usec) us minimum (empty = not set)"
}

stats() {
    "${A[@]}" logcat -c
    "${A[@]}" shell am broadcast -n $APP/.DebugReceiver -a STATS >/dev/null
    sleep 1
    "${A[@]}" logcat -d -s mpe-test:I | grep STATS | sed 's/.*STATS //' | tr ' ' '\n'
}

case "${1:-}" in
    check) info ;;
    install)
        gradle assembleDebug -q
        "${A[@]}" install -r -g app/build/outputs/apk/debug/app-debug.apk
        "${A[@]}" install -r -g fakeseaboard/build/outputs/apk/debug/fakeseaboard-debug.apk
        "${A[@]}" shell am start -n $APP/.MainActivity >/dev/null
        echo "Installed and started." ;;
    fake-test) python3 -I tools/emulator_test.py --serial "$SERIAL" --no-install ;;
    stats) stats ;;
    log) "${A[@]}" logcat -v time -s mpe:* mpe-test:* AAudio:* OboeAudio:* MidiService:* AndroidRuntime:E ;;
    report)
        model=$("${A[@]}" shell getprop ro.product.model | tr ' ' '-')
        out="captures/phone-$model-$(date +%Y%m%d-%H%M).txt"
        { info; echo; echo "stats:"; stats; } | tee "$out"
        echo; echo "saved $out" ;;
    *) sed -n '2,9p' "$0"; exit 1 ;;
esac
