"""End-to-end tests on an Android emulator (or a phone), with the fake Seaboard.

    source tools/env.sh
    gradle assembleDebug                       # builds app + fakeseaboard
    python3 -I tools/emulator_test.py [--no-install] [--serial emulator-5554]

What it does: installs the app and the fake Seaboard (a virtual MIDI device), then drives both
over adb. The fake Seaboard plays notes, replays the recorded session, drops the link, and is
unplugged; the app records its own audio output, which is pulled back and measured here.
Nothing needs a real controller. Needs an emulator that is already booted.
"""
import argparse, json, os, re, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP, FAKE = "com.patrickreynolds.mpeinstruments", "com.patrickreynolds.fakeseaboard"
ap = argparse.ArgumentParser()
ap.add_argument("--serial", default=None)
ap.add_argument("--no-install", action="store_true")
ap.add_argument("--only", default="", help="run tests whose name contains this")
args = ap.parse_args()
ADB = ["adb"] + (["-s", args.serial] if args.serial else [])
OUT = tempfile.mkdtemp(prefix="emutest-")

failures, results = [], []
def check(name, ok, detail=""):
    print(("PASS  " if ok else "FAIL  ") + name + (f"   ({detail})" if detail else ""), flush=True)
    results.append((name, ok, detail))
    if not ok: failures.append(name)

def adb(*a, check_rc=True):
    r = subprocess.run(ADB + list(a), capture_output=True, text=True)
    if check_rc and r.returncode != 0: raise RuntimeError(f"adb {' '.join(a)}: {r.stderr.strip()}")
    return r.stdout

def app_cmd(action, *extras): adb("shell", "am", "broadcast", "-n", f"{APP}/.DebugReceiver", "-a", action, *extras)
def fake_cmd(action, *extras): adb("shell", "am", "broadcast", "-n", f"{FAKE}/.CommandReceiver", "-a", action, *extras)

def log_since(mark, tags="mpe-test:I fakeseaboard:I mpe:I"):
    out = adb("logcat", "-d", "-v", "epoch", "-s", *tags.split())
    lines = []
    for line in out.splitlines():
        m = re.match(r"\s*(\d+\.\d+)", line)
        if m and float(m.group(1)) >= mark: lines.append(line)
    return lines

def device_now():
    return float(adb("shell", "date", "+%s.%N").strip()[:17])

def wait_log(mark, pattern, timeout=20):
    end = time.time() + timeout
    while time.time() < end:
        for line in log_since(mark):
            if re.search(pattern, line): return line
        time.sleep(0.3)
    return None

def stats():
    mark = device_now()
    app_cmd("STATS")
    line = wait_log(mark, r"STATS ")
    if not line: return {}
    return {k: v.strip('"') for k, v in re.findall(r'(\w+)=("[^"]*"|\S+)', line.split("STATS ", 1)[1])}

def record(seconds, name, during):
    """Starts recording the app's output, runs `during()`, waits, pulls and measures the WAV."""
    mark = device_now()
    app_cmd("RECORD", "--ef", "seconds", str(seconds), "--es", "name", name)
    if not wait_log(mark, r"RECORD started " + name, 5): return None
    during()
    line = wait_log(mark, r"RECORD (saved|failed) .*" + name, seconds + 15)
    if not line or "failed" in line: return None
    local = os.path.join(OUT, name + ".wav")
    # The file can take a moment to become visible through shared storage on a slow machine.
    for attempt in range(10):
        r = subprocess.run(ADB + ["pull", f"/sdcard/Android/data/{APP}/files/{name}.wav", local], capture_output=True, text=True)
        if r.returncode == 0: return local
        time.sleep(0.5)
    print(f"      could not pull {name}.wav: {r.stderr.strip() or r.stdout.strip()}")
    return None

def analyze(wav, hz=None, a=None, b=None):
    cmd = [sys.executable, "-I", os.path.join(ROOT, "tools", "analyze_wav.py"), wav]
    if hz: cmd += [str(hz)] + ([str(a), str(b)] if a is not None else [])
    return json.loads(subprocess.run(cmd, capture_output=True, text=True, check=True).stdout)

def rms_between(res, a, b):
    env = res["rms_per_100ms"][int(a * 10):int(b * 10)]
    return max(env) if env else 0.0

def midi_hz(n): return 440 * 2 ** ((n - 69) / 12)

def want(name): return args.only in name

# ---- setup ---------------------------------------------------------------------------------
if not args.no_install:
    adb("install", "-r", "-g", os.path.join(ROOT, "fakeseaboard/build/outputs/apk/debug/fakeseaboard-debug.apk"))
    adb("install", "-r", "-g", os.path.join(ROOT, "app/build/outputs/apk/debug/app-debug.apk"))
adb("shell", "pm", "enable", f"{FAKE}/.FakeSeaboardService", check_rc=False)   # in case a test left it unplugged
adb("shell", "am", "force-stop", APP)
adb("logcat", "-c")
adb("shell", "am", "start", "-n", f"{APP}/.MainActivity")
end = time.time() + 15
st = {}
while time.time() < end:
    st = stats()
    if st.get("midiState") == "1": break
    time.sleep(0.5)
check("app starts and auto-connects to the (emulated) Seaboard", st.get("midiState") == "1",
      f'connected="{st.get("connected")}", path={st.get("path")}, burst={st.get("burst")}')
app_cmd("SET", "--ei", "watchdog", "0", "--ei", "transpose", "0", "--ef", "volume", "0.8")

# ---- 1. pitch through the whole chain, several instruments --------------------------------
for inst, note in [("strings/cello", 50), ("strings/violin", 69), ("winds/flute", 79), ("synths/fm-keys", 60),
                   ("voices/soprano", 72), ("plucked/steel-guitar", 59)]:
    name = "pitch " + inst
    if not want(name): continue
    app_cmd("SET", "--es", "instrument", inst)
    time.sleep(0.3)
    tag = inst.replace("/", "_")
    wav = record(4, tag, lambda: fake_cmd("NOTE", "--ei", "channel", "3", "--ei", "note", str(note), "--ef", "seconds", "2"))
    if not wav: check(name, False, "no recording"); continue
    # The note starts a little after recording starts; judge the steady middle of the held part.
    res = analyze(wav, midi_hz(note), 0.8, 1.8)
    check(f"{name}: audible while held", res["held_rms"] > 0.005, f"rms {res['held_rms']}")
    check(f"{name}: note {note} in tune", res["cents_off"] is not None and abs(res["cents_off"]) <= 30, f"{res['cents_off']} cents")
    check(f"{name}: no clipping", res["peak"] < 0.99, f"peak {res['peak']}")
    if inst.startswith("plucked"):
        check(f"{name}: rings on after release (a plucked string)", True)
    else:
        check(f"{name}: quiet by the end", rms_between(res, 3.6, 4.0) < 0.01, f"last 0.4 s rms {rms_between(res, 3.6, 4.0)}")

# ---- 2. the recorded session, 30 s of it, through the real MIDI stack ----------------------
if want("session"):
    app_cmd("SET", "--es", "instrument", "strings/violin", "--ei", "transpose", "-60")   # played 5 octaves up
    before = stats()
    mark = device_now()
    wav = record(34, "session", lambda: fake_cmd("PLAY", "--es", "file", "seaboard-session.mid", "--ef", "seconds", "30"))
    sent_line = wait_log(mark, r"PLAY end", 10) or ""
    sent = int(re.search(r"sent (\d+)", sent_line).group(1)) if "sent" in sent_line else -1
    time.sleep(0.5)
    after = stats()
    got = int(after.get("events", 0)) - int(before.get("events", 0))
    # The app's clean-up stage drops stray note-offs, so it may count slightly fewer than were sent.
    check("session: every message reached the engine", sent > 0 and sent - 5 <= got <= sent, f"sent {sent}, engine got {got}")
    check("session: none dropped, no panics", after.get("dropped") == before.get("dropped") and after.get("panics") == before.get("panics"),
          f"dropped {after.get('dropped')}, panics {after.get('panics')}")
    check("session: MIDI -> synth stays under 2 callbacks", float(after.get("ageMaxMs", 1e9)) <= 2 * 1000 * int(after.get("burst", 960)) / 48000 + 5,
          f"max {float(after.get('ageMaxMs', 0)):.1f} ms, burst {after.get('burst')} frames")
    if wav:
        res = analyze(wav)
        env = res["rms_per_100ms"]
        playing = [v for v in env[5:300] if v > 0.002]
        check("session: audible while playing", len(playing) > 100, f"{len(playing)} of 295 windows audible")
        check("session: no clipping", res["peak"] < 0.99, f"peak {res['peak']}")
    else:
        check("session: recording", False)
    fake_cmd("STOP")
    app_cmd("SET", "--ei", "transpose", "0")
    time.sleep(1)

# ---- 3. Bluetooth-style drop: the link goes silent mid-note --------------------------------
if want("watchdog"):
    app_cmd("SET", "--es", "instrument", "strings/cello", "--ei", "watchdog", "1500")
    p0 = int(stats().get("panics", 0))
    def drop():
        fake_cmd("HOLD", "--ei", "channel", "2", "--ei", "note", "48", "--ef", "seconds", "1.0")
        time.sleep(1.5)
        fake_cmd("SILENCE")
    wav = record(6, "watchdog", drop)
    p1 = int(stats().get("panics", 0))
    check("watchdog on: one panic after the link went silent", p1 == p0 + 1, f"panics {p0} -> {p1}")
    if wav:
        res = analyze(wav)
        check("watchdog on: the note was playing", rms_between(res, 0.5, 1.0) > 0.005)
        check("watchdog on: the note stopped by itself", rms_between(res, 5.0, 6.0) < 0.003, f"rms at 5-6 s {rms_between(res, 5.0, 6.0)}")

    app_cmd("SET", "--ei", "watchdog", "0")
    def drop2():
        fake_cmd("HOLD", "--ei", "channel", "2", "--ei", "note", "48", "--ef", "seconds", "1.0")
        time.sleep(1.5)
        fake_cmd("SILENCE")
    wav = record(4, "nowatchdog", drop2)
    if wav:
        res = analyze(wav)
        check("watchdog off: a held note keeps sounding (expected; this is why BLE needs it)", rms_between(res, 3.0, 4.0) > 0.005)
    app_cmd("PANIC")
    time.sleep(1)

# ---- 4. cable pulled mid-note, then plugged back in -----------------------------------------
if want("unplug"):
    app_cmd("SET", "--es", "instrument", "strings/cello")
    def pull():
        fake_cmd("HOLD", "--ei", "channel", "4", "--ei", "note", "48", "--ef", "seconds", "1.0")
        time.sleep(1.5)
        fake_cmd("UNPLUG")
    wav = record(6, "unplug", pull)
    st = stats()
    check("unplug: the app notices the device is gone", st.get("midiState") == "0" and st.get("connected") == "",
          f'midiState={st.get("midiState")} connected="{st.get("connected")}"')
    if wav:
        res = analyze(wav)
        check("unplug: the note was playing before", rms_between(res, 0.5, 1.2) > 0.005)
        check("unplug: the held note stops (no stuck note)", rms_between(res, 5.0, 6.0) < 0.003, f"rms at 5-6 s {rms_between(res, 5.0, 6.0)}")
    fake_cmd("PLUG")
    end = time.time() + 10
    while time.time() < end and stats().get("midiState") != "1": time.sleep(0.5)
    st = stats()
    check("replug: the app reconnects by itself", st.get("midiState") == "1", f'connected="{st.get("connected")}"')
    wav = record(3, "replug", lambda: fake_cmd("NOTE", "--ei", "channel", "1", "--ei", "note", "50", "--ef", "seconds", "1.5"))
    if wav:
        res = analyze(wav, midi_hz(50), 0.6, 1.4)
        check("replug: plays in tune again", res["cents_off"] is not None and abs(res["cents_off"]) <= 30, f"{res['cents_off']} cents")

# ---- 5. switching instrument while a note is held --------------------------------------------
if want("switch"):
    app_cmd("SET", "--es", "instrument", "strings/cello")
    def switch():
        fake_cmd("NOTE", "--ei", "channel", "5", "--ei", "note", "57", "--ef", "seconds", "3")
        time.sleep(1.0)
        app_cmd("SET", "--es", "instrument", "strings/violin")
        time.sleep(0.7)
        app_cmd("SET", "--es", "instrument", "winds/flute")
    wav = record(5, "switch", switch)
    if wav:
        res = analyze(wav, midi_hz(57), 2.2, 3.0)
        check("switch: still sounding on the new instrument", res["held_rms"] > 0.003, f"rms {res['held_rms']}")
        check("switch: no clipping during the switches", res["peak"] < 0.99, f"peak {res['peak']}")
    time.sleep(1)

# ---- 6. screen off: the foreground service must keep the audio and MIDI going -----------------
if want("screen"):
    app_cmd("SET", "--es", "instrument", "strings/cello")
    adb("shell", "input", "keyevent", "KEYCODE_SLEEP")
    time.sleep(3)
    wav = record(4, "screenoff", lambda: fake_cmd("NOTE", "--ei", "channel", "6", "--ei", "note", "50", "--ef", "seconds", "2"))
    if wav:
        res = analyze(wav, midi_hz(50), 0.8, 1.8)
        check("screen off: still plays, in tune", res["cents_off"] is not None and abs(res["cents_off"]) <= 30 and res["held_rms"] > 0.005,
              f"rms {res['held_rms']}, {res['cents_off']} cents")
    else:
        check("screen off: recording", False)
    st = stats()
    check("screen off: still connected", st.get("midiState") == "1")
    # Doze: what a phone does after lying screen-off for a while.
    adb("shell", "dumpsys", "deviceidle", "force-idle", check_rc=False)
    time.sleep(2)
    wav = record(4, "doze", lambda: fake_cmd("NOTE", "--ei", "channel", "7", "--ei", "note", "52", "--ef", "seconds", "2"))
    if wav:
        res = analyze(wav, midi_hz(52), 0.8, 1.8)
        check("doze: still plays, in tune", res["cents_off"] is not None and abs(res["cents_off"]) <= 30 and res["held_rms"] > 0.005,
              f"rms {res['held_rms']}, {res['cents_off']} cents")
    else:
        check("doze: recording", False)
    adb("shell", "dumpsys", "deviceidle", "unforce", check_rc=False)
    adb("shell", "input", "keyevent", "KEYCODE_WAKEUP")
    adb("shell", "wm", "dismiss-keyguard", check_rc=False)
    time.sleep(1)

# ---- 7. the audio server dies (what a route change or a media crash does): recover, no crash ---
if want("audioserver"):
    rooted = "root" in adb("root", check_rc=False) or adb("shell", "id", check_rc=False).startswith("uid=0")
    if rooted:
        time.sleep(3)
        adb("wait-for-device")
    if not rooted or not adb("shell", "pidof", "audioserver", check_rc=False).strip():
        print("SKIP  audioserver: needs a rooted emulator (adb root)")
    else:
        pid0 = adb("shell", "pidof", APP, check_rc=False).strip()
        r0 = int(stats().get("restarts", 0))
        adb("shell", "kill", adb("shell", "pidof", "audioserver").strip())
        time.sleep(7)
        pid1 = adb("shell", "pidof", APP, check_rc=False).strip()
        st = stats()
        check("audioserver killed: the app survives", pid1 == pid0 and pid0 != "", f"pid {pid0} -> {pid1}")
        check("audioserver killed: the stream is reopened", int(st.get("restarts", 0)) > r0, f"restarts {r0} -> {st.get('restarts')}")
        check("audioserver killed: latency readout is sane again", 0 < float(st.get("outLatencyMs", -1)) < 1000, f"{st.get('outLatencyMs')} ms")
        check("audioserver killed: still connected to the Seaboard", st.get("midiState") == "1")
        app_cmd("SET", "--es", "instrument", "strings/cello")
        wav = record(4, "afterserver", lambda: fake_cmd("NOTE", "--ei", "channel", "8", "--ei", "note", "50", "--ef", "seconds", "2"))
        if wav:
            res = analyze(wav, midi_hz(50), 0.8, 1.8)
            check("audioserver killed: plays in tune afterwards", res["cents_off"] is not None and abs(res["cents_off"]) <= 30, f"{res['cents_off']} cents")

# ---- 8. stress: dense playing while the cable is pulled and instruments change -------------------
if want("stress"):
    pid0 = adb("shell", "pidof", APP, check_rc=False).strip()
    app_cmd("SET", "--ei", "transpose", "-60", "--ei", "watchdog", "0")
    insts = ["strings/cello", "winds/flute", "synths/pad", "plucked/nylon-guitar", "voices/tenor", "struck/marimba"]
    fake_cmd("PLAY", "--es", "file", "seaboard-session.mid", "--ef", "speed", "2", "--ef", "seconds", "60")
    for i in range(8):
        for k in range(4):
            app_cmd("SET", "--es", "instrument", insts[(i * 4 + k) % len(insts)])
            time.sleep(0.15)
        fake_cmd("UNPLUG")
        time.sleep(0.4)
        fake_cmd("PLUG")
        time.sleep(1.2)
        # unplugging stops the player; start it again so the next cycle is played into
        fake_cmd("PLAY", "--es", "file", "seaboard-session.mid", "--ef", "speed", "2", "--ef", "seconds", "60")
    fake_cmd("STOP")
    end = time.time() + 10
    while time.time() < end and stats().get("midiState") != "1": time.sleep(0.5)
    pid1 = adb("shell", "pidof", APP, check_rc=False).strip()
    st = stats()
    check("stress: the app survived 8 unplug/replug cycles and 32 instrument switches", pid1 == pid0 and pid0 != "", f"pid {pid0} -> {pid1}")
    check("stress: connected again at the end", st.get("midiState") == "1", f'connected="{st.get("connected")}"')
    app_cmd("SET", "--es", "instrument", "strings/cello", "--ei", "transpose", "0")
    time.sleep(4)                                     # anything still ringing dies away
    wav = record(2, "afterstress", lambda: None)
    if wav:
        res = analyze(wav)
        check("stress: silence afterwards (no stuck note)", res["peak"] < 0.01, f"peak {res['peak']}")
    wav = record(3, "afterstress2", lambda: fake_cmd("NOTE", "--ei", "channel", "1", "--ei", "note", "50", "--ef", "seconds", "1.5"))
    if wav:
        res = analyze(wav, midi_hz(50), 0.6, 1.4)
        check("stress: still plays in tune", res["cents_off"] is not None and abs(res["cents_off"]) <= 30, f"{res['cents_off']} cents")

# ---- 9. app killed and started again ------------------------------------------------------------
if want("restart"):
    adb("shell", "am", "force-stop", APP)
    time.sleep(1)
    adb("shell", "am", "start", "-n", f"{APP}/.MainActivity")
    end = time.time() + 15
    while time.time() < end and stats().get("midiState") != "1": time.sleep(0.5)
    st = stats()
    check("restart: reconnects to the Seaboard after the app was killed", st.get("midiState") == "1")
    # Oboe's LatencyTuner starts at the smallest buffer and grows it on an underrun, so a few
    # underruns right after start are expected. Once settled, there must be no more.
    time.sleep(3)
    x0 = int(stats().get("xruns", 0))
    time.sleep(5)
    st = stats()
    x1 = int(st.get("xruns", 0))
    check("restart: no new underruns once the latency tuner has settled", x1 == x0,
          f"xruns {x0} -> {x1} over 5 s, buffer {st.get('buffer')} frames")

print(f"\n{len(results)} checks, {len(failures)} failed. Recordings in {OUT}")
for f in failures: print("  FAILED:", f)
sys.exit(1 if failures else 0)
