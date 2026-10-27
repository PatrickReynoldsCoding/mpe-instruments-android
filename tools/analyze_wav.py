"""Measures a WAV recorded by the app (DebugReceiver RECORD): level over time and pitch.

    python3 -I tools/analyze_wav.py file.wav [expected_hz] [start_s end_s]

Prints JSON. Pitch uses the same method as mpe-instruments' tools/pitch.py: the strongest
peak within 60 cents of the expected frequency, checked against the loudest partial."""
import json, sys, wave
import numpy as np

def load(path):
    with wave.open(path, "rb") as w:
        sr, ch, n = w.getframerate(), w.getnchannels(), w.getnframes()
        x = np.frombuffer(w.readframes(n), dtype="<i2").astype(np.float64) / 32768.0
    return x.reshape(-1, ch)[:, 0], sr

def cents_off(x, sr, target):
    x = (x - x.mean()) * np.hanning(len(x))
    n = len(x) * 8
    mag = np.abs(np.fft.rfft(x, n)); bin_hz = sr / n
    def peak(f, win=60):
        lo, hi = int(f * 2 ** (-win / 1200) / bin_hz), int(f * 2 ** (win / 1200) / bin_hz) + 1
        i = lo + int(np.argmax(mag[lo:hi + 1]))
        a, b, c = np.log(mag[i - 1:i + 2] + 1e-12)
        return (i + 0.5 * (a - c) / (a - 2 * b + c + 1e-12)) * bin_hz, mag[i]
    f0, a0 = peak(target)
    loud = mag[int(target * 0.45 / bin_hz):int(target * 2.2 / bin_hz)].max()
    if a0 < 0.08 * loud: return 1200.0
    if peak(target / 2)[1] > 0.3 * a0: return -1200.0
    return float(1200 * np.log2(f0 / target))

x, sr = load(sys.argv[1])
rms = lambda a: float(np.sqrt(np.mean(a ** 2))) if len(a) else 0.0
win = int(0.1 * sr)
env = [round(rms(x[i:i + win]), 4) for i in range(0, len(x), win)]
out = {"seconds": round(len(x) / sr, 2), "rate": sr, "peak": round(float(np.abs(x).max()), 3), "rms_per_100ms": env}
if len(sys.argv) > 2:
    target = float(sys.argv[2])
    a, b = (float(sys.argv[3]), float(sys.argv[4])) if len(sys.argv) > 4 else (0.6, 1.4)
    seg = x[int(a * sr):int(b * sr)]
    out["held_rms"] = round(rms(seg), 4)
    out["cents_off"] = round(cents_off(seg, sr, target), 1) if rms(seg) > 1e-4 else None
print(json.dumps(out))
