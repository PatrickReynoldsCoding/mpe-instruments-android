// Tiny test helpers: CHECK prints PASS/FAIL, the exit code says if any failed.
#pragma once
#include <cmath>
#include <cstdio>
#include <vector>
#include <complex>
#include <cstdint>
#include "mpe/engine.h"

inline int gFailures = 0, gChecks = 0;
#define CHECK(cond, ...) do { gChecks++; if (cond) { printf("PASS  "); } else { printf("FAIL  "); gFailures++; } \
    printf(__VA_ARGS__); printf("\n"); } while (0)
inline int finish() { printf("\n%d checks, %d failed\n", gChecks, gFailures); return gFailures ? 1 : 0; }

constexpr int kSr = 48000;

// Test double: a MIDI source that talks to an Engine the way the app does.
struct Rig {
    mpe::Engine engine;
    mpe::MidiInput in;
    int64_t clockNs = 1000000000;                 // fake steady clock, advances with rendered audio
    explicit Rig(const char* instrument, int sr = kSr) : engine(sr), in(engine) {
        int idx = mpe::findInstrument(instrument);
        if (idx < 0) { printf("unknown instrument %s\n", instrument); exit(2); }
        engine.setInstrument(idx);
    }
    void send(uint8_t status, uint8_t d1, uint8_t d2) { uint8_t b[3] = {status, d1, d2}; in.feed(b, 3, clockNs); }
    void noteOn(int ch, int note, int vel) { send(0x90 | ch, note, vel); }
    void noteOff(int ch, int note) { send(0x80 | ch, note, 0); }
    void bend(int ch, int v14) { send(0xE0 | ch, v14 & 127, (v14 >> 7) & 127); }   // v14: 0..16383
    void press(int ch, int v) { send(0xD0 | ch, v, 0); }
    void slide(int ch, int v) { send(0xB0 | ch, 74, v); }
    // Renders `seconds` of audio in callbacks of `block` frames, returns the left-channel mono.
    std::vector<float> run(double seconds, int block = 128) {
        std::vector<float> mono, buf(block * 2);
        int total = int(seconds * engine.sampleRate());
        for (int done = 0; done < total; done += block) {
            int n = std::min(block, total - done);
            engine.render(buf.data(), n, clockNs);
            for (int i = 0; i < n; i++) mono.push_back(buf[2 * i]);
            clockNs += int64_t(1e9 * n / engine.sampleRate());
        }
        return mono;
    }
};

inline double rms(const std::vector<float>& x, size_t a, size_t b) {
    if (b > x.size()) b = x.size();
    if (a >= b) return 0;
    double s = 0;
    for (size_t i = a; i < b; i++) s += double(x[i]) * x[i];
    return std::sqrt(s / (b - a));
}
inline bool finite(const std::vector<float>& x) { for (float v : x) if (!std::isfinite(v)) return false; return true; }
inline float peak(const std::vector<float>& x) { float p = 0; for (float v : x) p = std::max(p, std::fabs(v)); return p; }

// Fundamental by normalised autocorrelation over [fmin, fmax]; picks the first strong peak
// (not the global one) so a strong 2nd period cannot win. Returns 0 if nothing periodic.
inline double pitchHz(const std::vector<float>& x, size_t a, size_t b, double fmin, double fmax, int sr = kSr) {
    if (b > x.size()) b = x.size();
    int lagMin = int(sr / fmax), lagMax = int(sr / fmin);
    size_t n = b - a;
    if (n < size_t(lagMax) * 3) return 0;
    std::vector<double> r(lagMax + 2, 0);
    double e0 = 0;
    for (size_t i = a; i < b - lagMax; i++) e0 += double(x[i]) * x[i];
    if (e0 <= 0) return 0;
    for (int lag = lagMin; lag <= lagMax + 1; lag++) {
        double s = 0, e1 = 0;
        for (size_t i = a; i < b - lagMax; i++) { s += double(x[i]) * x[i + lag]; e1 += double(x[i + lag]) * x[i + lag]; }
        r[lag] = s / std::sqrt(e0 * e1 + 1e-30);
    }
    double best = 0;
    for (int lag = lagMin + 1; lag <= lagMax; lag++) best = std::max(best, r[lag]);
    for (int lag = lagMin + 1; lag <= lagMax; lag++) {
        if (r[lag] >= 0.9 * best && r[lag] >= r[lag - 1] && r[lag] >= r[lag + 1] && best > 0.5) {
            double y0 = r[lag - 1], y1 = r[lag], y2 = r[lag + 1];     // parabolic refinement
            double d = (y0 - 2 * y1 + y2);
            double off = d != 0 ? 0.5 * (y0 - y2) / d : 0;
            return sr / (lag + off);
        }
    }
    return 0;
}
inline double cents(double f, double ref) { return 1200.0 * std::log2(f / ref); }
inline double midiHz(double note) { return 440.0 * std::pow(2.0, (note - 69.0) / 12.0); }

// ---- fundamental check, same method as tools/pitch.py in mpe-instruments ----------------------
// Looks for the fundamental directly instead of guessing a pitch from the whole signal, because a
// body resonance (one loud overtone) or a sound with few or odd partials fools every guess-the-pitch
// method. Returns cents off target; -1200 = an octave low; +1200 = the fundamental is missing.
inline void fft(std::vector<std::complex<double>>& a) {
    size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; i++) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        double ang = -2 * M_PI / len;
        std::complex<double> wl(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<double> w(1);
            for (size_t k = 0; k < len / 2; k++) {
                auto u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v; a[i + k + len / 2] = u - v; w *= wl;
            }
        }
    }
}
inline double centsOff(const std::vector<float>& x, size_t a, size_t b, double target, int sr = kSr) {
    if (b > x.size()) b = x.size();
    size_t N = b - a, M = 1; while (M < N * 8) M <<= 1;
    double mean = 0; for (size_t i = a; i < b; i++) mean += x[i]; mean /= N;
    std::vector<std::complex<double>> buf(M);
    for (size_t i = 0; i < N; i++) buf[i] = (x[a + i] - mean) * (0.5 - 0.5 * std::cos(2 * M_PI * i / (N - 1)));
    fft(buf);
    const double bin = double(sr) / M;
    std::vector<double> mag(M / 2 + 1);
    for (size_t i = 0; i < mag.size(); i++) mag[i] = std::abs(buf[i]);
    auto peakNear = [&](double f, double window, double& level) {
        size_t lo = size_t(f * std::pow(2, -window / 1200) / bin), hi = size_t(f * std::pow(2, window / 1200) / bin) + 1;
        size_t best = lo;
        for (size_t i = lo; i <= hi && i < mag.size(); i++) if (mag[i] > mag[best]) best = i;
        double pos = best;
        if (best > 0 && best + 1 < mag.size()) {
            double l0 = std::log(mag[best - 1] + 1e-12), l1 = std::log(mag[best] + 1e-12), l2 = std::log(mag[best + 1] + 1e-12);
            pos = best + 0.5 * (l0 - l2) / (l0 - 2 * l1 + l2 + 1e-12);
        }
        level = mag[best];
        return pos * bin;
    };
    double a0 = 0, aHalf = 0;
    double f0 = peakNear(target, 60, a0);
    double loud = 0;
    for (size_t i = size_t(target * 0.45 / bin); i < size_t(target * 2.2 / bin) && i < mag.size(); i++) loud = std::max(loud, mag[i]);
    if (a0 < 0.08 * loud) return 1200;
    peakNear(target / 2, 60, aHalf);
    if (aHalf > 0.3 * a0) return -1200;
    return 1200 * std::log2(f0 / target);
}
