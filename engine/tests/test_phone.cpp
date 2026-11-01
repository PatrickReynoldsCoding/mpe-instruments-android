// The phone as a controller: tilt (gyro) and breath (microphone) acting on every instrument,
// and the breath detector on synthetic microphone signals.
#include <algorithm>
#include <random>
#include "check.h"
#include "mpe/breath.h"

using namespace mpe;

static VoiceView firstVoice(Rig& r) {
    VoiceView v[32];
    int n = r.engine.voices(v, 32);
    for (int i = 0; i < n; i++) if (v[i].gate) return v[i];
    return VoiceView{};
}

// ---- microphone signals ------------------------------------------------------------------------

struct Mic {
    Breath b{kSr};
    std::mt19937 rng{7};
    std::normal_distribution<float> g{0.f, 1.f};
    float brown = 0, t = 0;
    // Feeds `seconds` of a signal, 1 ms at a time; returns the breath level after each ms.
    template <class F> std::vector<float> feed(double seconds, F sample) {
        std::vector<float> lv, buf(48);
        for (int ms = 0; ms < int(seconds * 1000); ms++) {
            for (float& x : buf) { x = sample(); t += 1.f / kSr; }
            b.process(buf.data(), int(buf.size()));
            lv.push_back(b.level());
        }
        return lv;
    }
    float room() { return 0.0005f * g(rng); }                                          // a quiet room
    float blow(float amp) { brown = 0.995f * brown + 0.1f * g(rng); return amp * brown + room(); }   // air on the capsule: loud, low
};

static float maxOf(const std::vector<float>& v, size_t a, size_t b) {
    float m = 0;
    for (size_t i = a; i < std::min(b, v.size()); i++) m = std::max(m, v[i]);
    return m;
}
static float minOf(const std::vector<float>& v, size_t a, size_t b) {
    float m = 1e9f;
    for (size_t i = a; i < std::min(b, v.size()); i++) m = std::min(m, v[i]);
    return m;
}
static int firstAbove(const std::vector<float>& v, float th) { for (size_t i = 0; i < v.size(); i++) if (v[i] > th) return int(i); return -1; }
static int firstBelow(const std::vector<float>& v, float th) { for (size_t i = 0; i < v.size(); i++) if (v[i] < th) return int(i); return -1; }

static void breathDetector() {
    {
        Mic m;
        auto quiet = m.feed(3, [&] { return m.room(); });
        CHECK(maxOf(quiet, 500, quiet.size()) == 0.f, "breath: a quiet room reads 0 (max %.2f)", maxOf(quiet, 500, quiet.size()));
        auto blow = m.feed(1, [&] { return m.blow(0.08f); });
        const int on = firstAbove(blow, 0.5f);
        CHECK(on >= 0 && on <= 25, "breath: blowing reads within 25 ms (%d ms)", on);
        CHECK(minOf(blow, 100, blow.size()) > 0.6f, "breath: a steady blow stays up (min %.2f)", minOf(blow, 100, blow.size()));
        auto soft = m.feed(1.5, [&] { return m.blow(0.003f); });
        double mean = 0, var = 0;
        for (size_t i = 500; i < soft.size(); i++) mean += soft[i];
        mean /= soft.size() - 500;
        for (size_t i = 500; i < soft.size(); i++) var += (soft[i] - mean) * (soft[i] - mean);
        const double sd = std::sqrt(var / (soft.size() - 500));
        // The test's breath is a narrow, very low rumble, whose loudness wobbles by itself: the
        // hardest case. sd 0.08 is about +-1 dB of loudness in Swell. Smoother would be slower to stop.
        CHECK(mean > 0.2 && mean < 0.8 && sd < 0.08, "breath: a steady medium breath reads steadily (%.2f, sd %.3f)", mean, sd);
        auto after = m.feed(1, [&] { return m.room(); });
        const int off = firstBelow(after, 0.02f);
        CHECK(off >= 0 && off <= 250, "breath: stopping reads 0 within 250 ms (%d ms)", off);
        Breath::Meter me = m.b.meter();
        CHECK(me.floorDb < me.onDb && me.onDb < me.fullDb, "breath: meter floor %.0f < on %.0f < full %.0f dB", me.floorDb, me.onDb, me.fullDb);
    }
    {
        // The phone's speaker playing loudly, a bass voice's 110 Hz and its overtones, as the
        // microphone hears it: a phone speaker rolls off under its resonance (2nd order, 500 Hz).
        // The engine says how loud its own low end is (here: what a low-pass of the same note gives).
        Mic m;
        m.feed(3, [&] { return m.room(); });
        float outPow = 0;
        for (int h = 1; h <= 12; h++) {
            const double f = 110.0 * h, g = 1 / std::sqrt(1 + std::pow(f / 150, 8));   // 24 dB/oct at 150 Hz
            outPow += float(std::pow(0.12 / h * g, 2) / 2);
        }
        m.b.setEcho(10 * std::log10(outPow));
        const float w0 = 2 * float(M_PI) * 500 / kSr, cw = std::cos(w0), al = std::sin(w0) / (2 * 0.7071f), a0 = 1 + al;
        const float b0 = (1 + cw) / 2 / a0, b1 = -(1 + cw) / a0, a1 = -2 * cw / a0, a2 = (1 - al) / a0;
        float z1 = 0, z2 = 0;
        auto bleed = m.feed(2, [&] {
            const float w = 2 * float(M_PI) * m.t;
            float x = 0;
            for (int h = 1; h <= 12; h++) x += 0.12f / h * std::sin(110 * h * w);
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b0 * x - a2 * y;
            return 0.8f * y + m.room();
        });
        CHECK(maxOf(bleed, 0, bleed.size()) < 0.05f, "breath: the app's own sound from the speaker is not breath (max %.2f, echo %.0f dB)",
              maxOf(bleed, 0, bleed.size()), m.b.meter().echoDb);
        auto both = m.feed(1, [&] { return m.blow(0.08f); });
        CHECK(minOf(both, 50, both.size()) > 0.5f, "breath: blowing still reads while the speaker plays (min %.2f)", minOf(both, 50, both.size()));
    }
    {
        // A fan starts: low rumble, steady. It reads as breath at first, then is learnt.
        Mic m;
        m.feed(3, [&] { return m.room(); });
        float fan = 0;
        auto f = m.feed(12, [&] { fan = 0.99f * fan + 0.02f * m.g(m.rng); return 0.02f * fan + m.room(); });
        CHECK(maxOf(f, 0, 500) > 0.f && maxOf(f, 11000, f.size()) == 0.f,
              "breath: a steady fan is learnt as the floor (first %.2f, after 11 s %.2f)", maxOf(f, 0, 500), maxOf(f, 11000, f.size()));
    }
    {
        // Long phrases with a short breath between them: the floor must not creep up into the playing.
        Mic m;
        m.feed(3, [&] { return m.room(); });
        std::vector<float> all;
        for (int k = 0; k < 4; k++) {
            auto a = m.feed(4, [&] { return m.blow(0.06f); });
            auto gap = m.feed(0.3, [&] { return m.room(); });
            all.insert(all.end(), a.begin(), a.end());
        }
        CHECK(minOf(all, all.size() - 500, all.size()) > 0.5f, "breath: after 16 s of phrases the blow still reads (min %.2f)",
              minOf(all, all.size() - 500, all.size()));
    }
    {
        // Sensitivity: the same soft breath reads higher when more sensitive.
        float read[2];
        for (int k = 0; k < 2; k++) {
            Mic m;
            m.b.setSensitivity(k ? 0.9f : 0.1f);
            m.feed(3, [&] { return m.room(); });
            auto a = m.feed(0.5, [&] { return m.blow(0.01f); });
            read[k] = a.back();
        }
        CHECK(read[1] > read[0] + 0.15f, "breath: more sensitive reads a soft breath higher (%.2f vs %.2f)", read[1], read[0]);
    }
    {
        Mic m;
        auto z = m.feed(1, [] { return 0.f; });          // a stream that delivers zeros first
        auto q = m.feed(3, [&] { return m.room(); });
        CHECK(maxOf(q, 0, q.size()) == 0.f, "breath: digital silence at the start does not make the room read as breath (max %.2f)",
              maxOf(q, 0, q.size()));
    }
}

// ---- tilt ----------------------------------------------------------------------------------

static void tilt() {
    {
        Rig r("synths/lead");
        r.engine.setTiltMode(TiltMode::Bend, 2);
        r.engine.setTilt(5);                             // the phone held at a slant before playing
        r.run(0.5);
        r.noteOn(1, 60, 100); r.press(1, 90);
        r.run(0.2);
        CHECK(std::fabs(firstVoice(r).pitch - 60) < 0.01f, "tilt bend: the first note starts in tune at any slant (%.3f)", firstVoice(r).pitch);
        r.engine.setTilt(5 + 1.5f);
        r.run(0.1);
        CHECK(std::fabs(firstVoice(r).pitch - 60) < 0.01f, "tilt bend: 1.5 degrees is inside the dead zone (%.3f)", firstVoice(r).pitch);
        r.engine.setTilt(5 + 16);
        r.run(0.1);
        CHECK(std::fabs(firstVoice(r).pitch - 61) < 0.02f, "tilt bend: 16 degrees clockwise bends +1 semitone of 2 (%.3f)", firstVoice(r).pitch);
        auto x = r.run(0.5);
        const double c = centsOff(x, 0, x.size(), midiHz(61));
        CHECK(std::fabs(c) < 15, "tilt bend: it sounds a semitone up (%.1f cents off C#)", c);
        ModView mv = r.engine.modifiers();
        CHECK(mv.held && std::fabs(mv.tilt - 16) < 0.1f && std::fabs(mv.bend - 1) < 0.02f,
              "tilt bend: the screen sees 16 degrees and +1 semitone (%.1f, %.2f)", mv.tilt, mv.bend);
        r.engine.setTilt(5 - 40);
        r.run(0.1);
        CHECK(std::fabs(firstVoice(r).pitch - 58) < 0.02f, "tilt bend: past 30 degrees it stops at the range (%.3f)", firstVoice(r).pitch);
        r.noteOff(1, 60);
        r.run(0.5);
        r.noteOn(2, 64, 100);
        r.run(0.1);
        CHECK(std::fabs(firstVoice(r).pitch - 64) < 0.01f, "tilt bend: after letting go, the new slant is the new zero (%.3f)", firstVoice(r).pitch);
    }
    {
        Rig r("synths/lead");
        r.engine.setTiltMode(TiltMode::Vibrato, 50);
        r.engine.setTilt(10);
        r.run(0.3);
        r.noteOn(1, 60, 100); r.press(1, 90);
        r.run(0.1);
        float lo = 1e9f, hi = -1e9f;
        double sum = 0;
        int n = 0;
        for (int k = 0; k < 400; k++) {                   // 2 s: a 5 Hz wobble of 3.5 degrees around a 10 degree slant
            const float t = k * 0.005f;
            r.engine.setTilt(10 + 3.5f * std::sin(2 * float(M_PI) * 5 * t));
            r.run(0.005, 240);
            if (k < 100) continue;
            const float p = firstVoice(r).pitch;
            lo = std::min(lo, p); hi = std::max(hi, p); sum += p; n++;
        }
        const float depth = (hi - lo) / 2;
        CHECK(depth > 0.3f && depth < 0.6f, "tilt vibrato: a 3.5 degree wobble gives about 50 cents (%.0f cents)", depth * 100);
        CHECK(std::fabs(sum / n - 60) < 0.05, "tilt vibrato: the vibrato is around the note, not the slant (mean %.3f)", sum / n);
        r.engine.setTilt(30);                             // turned and held
        r.run(0.8);
        CHECK(std::fabs(firstVoice(r).pitch - 60) < 0.03f, "tilt vibrato: holding a new angle comes back to the note (%.3f)", firstVoice(r).pitch);
        r.engine.setTilt(30.1f);
        r.run(0.2);
        CHECK(std::fabs(firstVoice(r).pitch - 60) < 0.005f, "tilt vibrato: a tremor under 0.2 degrees does nothing (%.4f)", firstVoice(r).pitch);
    }
    {
        Rig r("voices/soprano");
        r.engine.setTiltMode(TiltMode::Tone, 0.4f);
        r.run(0.2);
        r.noteOn(1, 67, 100); r.press(1, 90); r.slide(1, 64);
        r.run(0.2);
        const float s0 = firstVoice(r).slide;
        r.engine.setTilt(30);
        r.run(0.3);
        const float s1 = firstVoice(r).slide;
        r.engine.setTilt(-30);
        r.run(0.3);
        const float s2 = firstVoice(r).slide;
        CHECK(std::fabs(s1 - s0 - 0.4f) < 0.02f && std::fabs(s0 - s2 - 0.4f) < 0.02f && std::fabs(firstVoice(r).pitch - 67) < 0.01f,
              "tilt tone: 30 degrees each way moves slide by 0.4, pitch untouched (%.2f %.2f %.2f)", s2, s0, s1);
    }
    {
        Rig r("synths/lead");
        r.engine.setTiltMode(TiltMode::Bend, 2);
        r.run(0.2);
        r.noteOn(1, 60, 100); r.press(1, 90);
        r.run(0.1);
        r.engine.setTilt(30);
        r.run(0.1);
        r.engine.setTiltMode(TiltMode::Off, 0);
        r.run(0.05);
        CHECK(std::fabs(firstVoice(r).pitch - 60) < 0.01f, "tilt off: a bend in progress goes back to the note (%.3f)", firstVoice(r).pitch);
        r.engine.setTilt(-50);
        r.run(0.1);
        CHECK(firstVoice(r).pitch == 60.f, "tilt off: turning the phone does nothing (%.4f)", firstVoice(r).pitch);
    }
    {
        // With pitch assist on, a tilt bend is not pulled back onto the scale.
        Rig r("synths/lead");
        Performance p;
        p.scaleMask = 0b101010110101;                    // C major
        p.assist = 1;
        r.engine.setPerformance(p);
        r.engine.setTiltMode(TiltMode::Bend, 2);
        r.run(0.2);
        r.noteOn(1, 60, 100); r.press(1, 90);
        r.run(0.1);
        r.engine.setTilt(9);                             // +0.5 semitone: between C and C#
        r.run(0.4);
        CHECK(std::fabs(firstVoice(r).pitch - 60.5f) < 0.02f, "tilt bend: pitch assist leaves the tilt alone (%.3f)", firstVoice(r).pitch);
    }
}

// ---- breath on the instruments -------------------------------------------------------------------

static void breathOnInstruments() {
    {
        // A guitar has no pressure control: swell works on its loudness anyway.
        Rig r("plucked/nylon-guitar");
        r.engine.setBreathMode(BreathMode::Swell);
        r.engine.setBreath(0);
        r.run(0.1);
        r.noteOn(1, 52, 110);
        auto still = r.run(0.3);
        r.engine.setBreath(1);
        auto blow = r.run(0.3);
        CHECK(rms(still, 0, still.size()) < 1e-4 && rms(blow, 0, blow.size()) > 0.01,
              "breath swell: a plucked note is silent until you blow (rms %.5f then %.3f)", rms(still, 0, still.size()), rms(blow, 0, blow.size()));
        ModView mv = r.engine.modifiers();
        CHECK(mv.breath > 0.99f, "breath swell: the screen sees the breath (%.2f)", mv.breath);
        CHECK(r.engine.outputLowDb() > -60 && r.engine.outputLowDb() < 0, "engine: reports its own low end for the detector (%.0f dBFS)",
              r.engine.outputLowDb());
    }
    {
        // A cello responds to pressure: with swell the breath is the pressure, whatever the finger does.
        Rig r("strings/cello");
        r.engine.setBreathMode(BreathMode::Swell);
        r.engine.setBreath(0.7f);
        r.run(0.1);
        r.noteOn(1, 48, 100); r.press(1, 12);
        r.run(0.3);
        CHECK(std::fabs(firstVoice(r).pressure - 0.7f) < 0.02f, "breath swell: breath replaces the finger's pressure (%.2f)", firstVoice(r).pressure);
        r.engine.setBreathMode(BreathMode::Off);
        r.run(0.3);
        CHECK(std::fabs(firstVoice(r).pressure - 12 / 127.f) < 0.02f, "breath off: the finger's pressure is back (%.2f)", firstVoice(r).pressure);
    }
    {
        // Breath tone: not blowing is dark, blowing opens it. Brightness = energy of the first difference.
        auto bright = [](const std::vector<float>& x) {
            double d = 0, e = 0;
            for (size_t i = 1; i < x.size(); i++) { d += double(x[i] - x[i - 1]) * (x[i] - x[i - 1]); e += double(x[i]) * x[i]; }
            return std::sqrt(d / (e + 1e-20));
        };
        Rig r("synths/lead");
        r.engine.setBreathMode(BreathMode::Tone);
        r.engine.setBreath(0);
        r.run(0.1);
        r.noteOn(1, 57, 100); r.press(1, 100);
        r.run(0.2);
        auto dark = r.run(0.3);
        r.engine.setBreath(1);
        r.run(0.1);
        auto open = r.run(0.3);
        CHECK(bright(dark) < 0.5 * bright(open) && rms(dark, 0, dark.size()) > 0.005,
              "breath tone: dark until you blow, but still heard (brightness %.3f vs %.3f)", bright(dark), bright(open));
    }
    {
        // Off: the microphone level is ignored.
        Rig r("synths/lead");
        r.engine.setBreath(0);
        r.run(0.1);
        r.noteOn(1, 60, 100); r.press(1, 100);
        auto x = r.run(0.3);
        CHECK(rms(x, kSr / 10, x.size()) > 0.01, "breath off: notes play without blowing (rms %.3f)", rms(x, kSr / 10, x.size()));
    }
}

int main() {
    breathDetector();
    tilt();
    breathOnInstruments();
    return finish();
}
