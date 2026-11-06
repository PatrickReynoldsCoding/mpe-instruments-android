// Plays every compiled instrument through the engine, driven by MIDI bytes like the app does.
#include <cstdlib>
#include <cstring>
#include <string>
#include "check.h"

using namespace mpe;

static double declaredNum(const InstrumentInfo& i, const char* key, int which, double def0, double def1 = 0) {
    const char* v = i.meta(key);
    double a = def0, b = def1;
    if (v[0]) sscanf(v, "%lf %lf", &a, &b);
    return which == 0 ? a : b;
}

int main() {
    const int n = instrumentCount();
    CHECK(n >= 10, "registry lists the instruments (%d)", n);

    for (int idx = 0; idx < n; idx++) {
        const InstrumentInfo& info = instrumentAt(idx);
        const bool pitched = std::strcmp(info.meta("pitched"), "no") != 0;
        const double lo = declaredNum(info, "range", 0, 130, 520), hi = declaredNum(info, "range", 1, 130, 520);
        const double winLo = declaredNum(info, "window", 0, 0.6, 1.4), winHi = declaredNum(info, "window", 1, 0.6, 1.4);
        const double ringout = std::strlen(info.meta("ringout")) ? atof(info.meta("ringout")) : 3.0;
        const double mid = std::sqrt(lo * hi);
        // nearest MIDI note to the middle of the range, so the check is on a real MIDI pitch
        const int note = int(std::lround(69 + 12 * std::log2(mid / 440.0)));
        const double want = midiHz(note);

        Rig r(info.id.c_str());
        r.noteOn(1, note, 90);
        r.press(1, 90); r.slide(1, 64);
        auto held = r.run(1.5);
        r.noteOff(1, note);
        auto tail = r.run(ringout + 0.5);

        CHECK(finite(held) && finite(tail), "%s: output is finite", info.id.c_str());
        CHECK(peak(held) <= 1.0f, "%s: no clipping (peak %.2f)", info.id.c_str(), peak(held));
        double lvl = rms(held, size_t(winLo * kSr), size_t(winHi * kSr));
        CHECK(lvl > 0.005, "%s: audible while held (rms %.4f)", info.id.c_str(), lvl);
        CHECK(rms(tail, tail.size() - size_t(0.3 * kSr), tail.size()) < std::max(0.005, 0.05 * lvl),
              "%s: stops after release", info.id.c_str());
        int freedAfter = 0;        // a slow release is fine, a voice that never frees is not
        for (; r.engine.activeVoices() > 0 && freedAfter < 20; freedAfter++) r.run(1.0);
        CHECK(r.engine.activeVoices() == 0, "%s: voice is freed after it has decayed (+%d s)", info.id.c_str(), freedAfter);
        if (pitched && lvl > 0.005) {
            double c = centsOff(held, size_t(winLo * kSr), size_t(winHi * kSr), want);
            CHECK(std::fabs(c) <= 30, "%s: note %d in tune (%+.0f cents)", info.id.c_str(), note, c);
        }
    }

    // Same note, different callback sizes (Android hands out 96, 192, 240 ... not just 64).
    {
        const int note = 57;
        double f0 = 0;
        for (int block : {32, 64, 96, 128, 192, 240, 480}) {
            Rig r("strings/cello");
            r.noteOn(1, note, 90); r.press(1, 80);
            auto x = r.run(1.5, block);
            double f = pitchHz(x, size_t(0.6 * kSr), size_t(1.4 * kSr), 80, 300);
            if (!f0) f0 = f;
            CHECK(finite(x) && std::fabs(cents(f, midiHz(note))) <= 30,
                  "callback of %d frames: finite and in tune (%.1f Hz)", block, f);
        }
    }

    // MPE: two fingers at once, each with its own bend.
    {
        Rig r("strings/violin");
        r.noteOn(1, 69, 90); r.press(1, 80);
        r.noteOn(2, 76, 90); r.press(2, 80);
        r.bend(2, 8192 + int(8192.0 * 2 / 48));            // +2 semitones on channel 2 only
        auto x = r.run(1.5);
        CHECK(r.engine.activeVoices() == 2, "two fingers = two voices");
        CHECK(finite(x) && rms(x, size_t(0.8 * kSr), size_t(1.4 * kSr)) > 0.005, "two fingers play");
        r.noteOff(1, 69); r.noteOff(2, 76);
        r.run(0.1);
    }
    {   // bend range and pitch: bend one finger +12 semitones (a quarter of the +/-48 range)
        Rig r("synths/lead");
        r.engine.setBendRange(48);
        r.noteOn(1, 60, 90); r.press(1, 80);
        r.run(0.5);
        r.bend(1, 8192 + 8192 / 4);
        auto x = r.run(1.0);
        double f = pitchHz(x, size_t(0.5 * kSr), size_t(1.0 * kSr), 200, 1200);
        CHECK(f > 0 && std::fabs(cents(f, midiHz(72))) <= 30, "bend +12 semitones lands an octave up (%.1f Hz)", f);
    }

    // Safety.
    {
        Rig r("strings/cello");
        r.noteOn(1, 57, 90); r.press(1, 80); r.run(0.5);
        r.in.deviceLost();                                  // USB unplugged mid note
        auto x = r.run(3.5);
        CHECK(rms(x, x.size() - size_t(0.3 * kSr), x.size()) < 0.005, "device lost mid-note: sound ends");
    }
    {
        Rig r("strings/cello");
        r.noteOn(1, 57, 90); r.press(1, 80); r.run(0.5);
        r.send(0xB0, 123, 0);                               // all notes off
        auto x = r.run(3.5);
        CHECK(rms(x, x.size() - size_t(0.3 * kSr), x.size()) < 0.005, "CC123: sound ends");
    }
    {
        Rig r("strings/cello");
        r.engine.setWatchdogMs(300);
        r.noteOn(1, 57, 90); r.press(1, 80);
        r.run(0.2);
        CHECK(r.engine.stats().panics == 0, "watchdog: no panic while link is recent");
        auto x = r.run(4.0);                                // silence on the link
        CHECK(r.engine.stats().panics == 1, "watchdog: panics once after 300 ms of silence with a held note");
        CHECK(rms(x, x.size() - size_t(0.3 * kSr), x.size()) < 0.005, "watchdog: the held note is released");
    }
    {
        Rig r("strings/cello");                             // default watchdog off: a held note keeps playing
        r.noteOn(1, 57, 90); r.press(1, 80);
        auto x = r.run(4.0);
        CHECK(r.engine.stats().panics == 0 && rms(x, x.size() - size_t(0.3 * kSr), x.size()) > 0.005,
              "watchdog off by default: a held note with a still finger keeps sounding");
    }
    {   // queue overflow: more events than fit, with no audio callback to drain them
        Rig r("strings/cello");
        r.noteOn(1, 57, 90);
        for (int i = 0; i < 3000; i++) r.press(1, i & 127);
        CHECK(r.engine.stats().dropped > 0, "overflow: dropped events are counted");
        r.noteOff(1, 57);                                   // this off was lost too
        auto x = r.run(4.0);
        CHECK(rms(x, x.size() - size_t(0.3 * kSr), x.size()) < 0.005, "overflow: no stuck note (engine panics)");
    }
    {   // switching instruments while a note is held
        Rig r("strings/cello");
        r.noteOn(1, 57, 90); r.press(1, 80); r.run(0.5);
        bool ok = true;
        for (const char* id : {"strings/violin", "winds/flute", "synths/pad", "strings/cello"}) {
            ok = ok && r.engine.setInstrument(findInstrument(id));
            auto x = r.run(0.3);
            ok = ok && finite(x);
        }
        r.engine.collectRetired();
        CHECK(ok, "switching instruments while a note is held keeps playing, no NaN");
        CHECK(rms(r.run(0.3), 0, size_t(0.3 * kSr)) > 0.005, "held note restarts on the new instrument");
        CHECK(!r.engine.setInstrument(-1) && !r.engine.setInstrument(9999), "bad instrument index is refused");
    }
    {   // per-finger volume (CC 7 on the finger's channel): sent before the note, it fades the note
        // to silence; it lasts only until the note ends
        auto level = [](int vol) {
            Rig r("synths/lead");
            if (vol >= 0) r.send(0xB1, 7, vol);
            r.noteOn(1, 60, 90); r.press(1, 64);
            return rms(r.run(0.5), size_t(0.2 * kSr), size_t(0.5 * kSr));
        };
        const double full = level(-1), half = level(64), none = level(0);
        CHECK(full > 0.01, "lead plays at the default volume (rms %.4f)", full);
        CHECK(std::fabs(half / full - 64 / 127.0) < 0.05, "CC 7 scales the note (%.3f of full)", half / full);
        CHECK(none < 1e-5, "CC 7 at 0 is silent, from the first sample (rms %.6f)", none);
        Rig r("synths/lead");
        r.send(0xB1, 7, 0); r.noteOn(1, 60, 90); r.press(1, 64); r.run(0.1); r.noteOff(1, 60); r.run(0.5);
        r.noteOn(1, 60, 90); r.press(1, 64);
        CHECK(rms(r.run(0.5), size_t(0.2 * kSr), size_t(0.5 * kSr)) > 0.01, "the next note on that channel is back at full volume");
    }
    {   // release: after the finger lifts the note plays on and fades over the part's release, then ends
        auto tail = [](float release) {
            Rig r("synths/lead");
            Performance p; p.part[0].release = release;
            r.engine.setPerformance(p);
            r.noteOn(1, 60, 90); r.press(1, 64);
            auto held = r.run(0.4);
            r.noteOff(1, 60);
            auto after = r.run(1.5);
            const double lvl = rms(held, size_t(0.2 * kSr), held.size());
            return std::vector<double>{lvl, rms(after, size_t(0.3 * kSr), size_t(0.4 * kSr)),
                                       rms(after, size_t(0.7 * kSr), size_t(0.8 * kSr)),
                                       rms(after, size_t(1.3 * kSr), after.size()), double(r.engine.activeVoices())};
        };
        auto none = tail(0), one = tail(1.0f);
        CHECK(none[1] < 0.1 * none[0], "no release: the lead stops soon after the finger lifts (%.4f of %.4f)", none[1], none[0]);
        CHECK(one[1] > 0.3 * one[0], "release 1 s: still sounding 0.3 s after the lift (%.3f of %.3f)", one[1], one[0]);
        CHECK(one[2] < one[1] && one[2] > 0.01 * one[0], "release 1 s: fading at 0.7 s (%.3f, was %.3f)", one[2], one[1]);
        CHECK(one[3] < 1e-4, "release 1 s: silent after it (%.6f)", one[3]);
        CHECK(one[4] == 0, "release 1 s: the voice is freed after the fade (%d active)", int(one[4]));
        // A new note on the same finger cuts the release.
        Rig r("synths/lead");
        Performance p; p.part[0].release = 3.0f;
        r.engine.setPerformance(p);
        r.noteOn(1, 60, 90); r.press(1, 64); r.run(0.3); r.noteOff(1, 60); r.run(0.1);
        r.noteOn(1, 67, 90); r.press(1, 64);
        auto again = r.run(0.5);
        CHECK(rms(again, size_t(0.3 * kSr), again.size()) > 0.5 * one[0], "a new note on that finger plays at full level");
    }
    {   // the drum kit: every note of the octave is a drum, struck, and dies away on its own
        auto bright = [](const std::vector<float>& x) {            // how much of it is high frequencies
            double d = 0, e = 0;
            for (size_t i = 1; i < x.size(); i++) { d += double(x[i] - x[i - 1]) * (x[i] - x[i - 1]); e += double(x[i]) * x[i]; }
            return std::sqrt(d / (e + 1e-20));
        };
        const char* names[12] = {"kick", "rim", "snare", "clap", "crash", "low tom", "closed hat", "mid tom",
                                 "pedal hat", "high tom", "open hat", "cowbell"};
        double brightness[12];
        for (int k = 0; k < 12; k++) {
            Rig r("drums/kit");
            r.noteOn(1, 36 + k, 100);
            auto x = r.run(0.3);
            r.noteOff(1, 36 + k);
            auto tail = r.run(4.0);
            CHECK(finite(x) && peak(x) <= 1.0f && peak(x) > 0.05f, "drums: %s sounds (peak %.2f)", names[k], peak(x));
            CHECK(rms(tail, tail.size() - size_t(0.3 * kSr), tail.size()) < 0.05 * peak(x), "drums: %s dies away", names[k]);
            brightness[k] = bright(x);
        }
        CHECK(brightness[0] < 0.3 * brightness[6] && brightness[0] < 0.3 * brightness[10],
              "drums: the kick is dark, the hats bright (%.3f vs %.3f, %.3f)", brightness[0], brightness[6], brightness[10]);
        Rig r("drums/kit");                                // the same drum in any octave
        r.noteOn(1, 60, 100);
        auto c4 = r.run(0.3);
        CHECK(bright(c4) < 0.3 * brightness[6], "drums: C in another octave is the kick too");
    }
    {   // latency bookkeeping: a message that arrived 5 ms before the block started
        Rig r("strings/cello");
        const uint8_t press[2] = {0xD1, 10};
        r.in.feed(press, 2, r.clockNs - 5000000);
        r.run(0.01);
        Stats s = r.engine.stats();
        CHECK(std::fabs(s.eventAgeLastMs - 5.0) < 0.01, "event age is measured (%.3f ms)", s.eventAgeLastMs);
    }
    {   // an event that arrives during a block is played by the NEXT block: age <= one callback
        Rig r("strings/cello");
        const int block = 96;
        std::vector<float> buf(block * 2);
        double worst = 0;
        for (int i = 0; i < 200; i++) {
            int64_t blockStart = r.clockNs;
            int64_t arrive = blockStart - int64_t(1e9 * (i % block) / kSr);       // arrives somewhere in the previous block
            const uint8_t press[2] = {0xD1, uint8_t(i & 127)};
            r.in.feed(press, 2, arrive);
            r.engine.render(buf.data(), block, blockStart);
            r.clockNs += int64_t(1e9 * block / kSr);
            worst = std::max(worst, r.engine.stats().eventAgeLastMs);
        }
        double oneBlockMs = 1000.0 * block / kSr;
        CHECK(worst <= oneBlockMs + 0.01, "engine adds at most one callback (%.2f ms of %.2f ms)", worst, oneBlockMs);
    }
    return finish();
}
