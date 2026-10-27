// The MIDI thread and the audio thread run at the same time in the app. Run this under
// ThreadSanitizer (cmake -DMPE_SANITIZE=thread) to catch data races.
#include <atomic>
#include <thread>
#include "check.h"

int main() {
    Rig r("strings/cello");
    std::atomic<bool> go{true};
    std::atomic<int> sent{0};

    // "MIDI thread": a busy controller, plus a cable pull now and then
    std::thread midi([&] {
        int i = 0;
        while (go) {
            int ch = 1 + (i % 15);
            uint8_t on[3] = {uint8_t(0x90 | ch), uint8_t(40 + i % 30), 90};
            r.in.feed(on, 3, mpe::steadyNowNs());
            for (int k = 0; k < 5; k++) { uint8_t p[2] = {uint8_t(0xD0 | ch), uint8_t((i + k) & 127)}; r.in.feed(p, 2, mpe::steadyNowNs()); }
            uint8_t off[3] = {uint8_t(0x80 | ch), uint8_t(40 + i % 30), 0};
            r.in.feed(off, 3, mpe::steadyNowNs());
            sent++;
            if (++i % 200 == 0) r.in.deviceLost();     // note: MidiInput is single-thread, so only this thread touches it
            std::this_thread::yield();
        }
    });
    // "control thread": settings and instrument switches
    std::thread control([&] {
        int n = 0; (void)mpe::instruments(n);
        for (int i = 0; go; i++) {
            r.engine.setVolume(0.5f + 0.1f * (i % 4));
            r.engine.setTranspose(i % 3 - 1);
            if (i % 10 == 0) r.engine.setInstrument(i / 10 % n);
            if (i % 10 == 5) r.engine.setInstrument(1, (i / 10 + 3) % n);
            if (i % 7 == 0) {                               // split, layer and the arp, toggled while playing
                mpe::Performance p;
                p.layout = mpe::Layout(i / 7 % 3);
                p.part[0].arp = i / 7 % 2;
                p.part[1].arp = i / 14 % 2;
                p.part[0].latch = i / 21 % 2;
                p.scaleMask = (i / 7 % 4) ? 0b101010110101 : 0;
                p.assist = 0.7f;
                r.engine.setPerformance(p);
            }
            if (i % 3 == 0) { mpe::VoiceView v[32]; mpe::FingerView f[16]; r.engine.voices(v, 32); r.engine.fingers(f, 16); }
            r.engine.collectRetired();
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    });
    // "audio thread": this one
    std::vector<float> buf(96 * 2);
    bool bad = false;
    auto end = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (std::chrono::steady_clock::now() < end) {
        r.engine.render(buf.data(), 96, mpe::steadyNowNs());
        for (float v : buf) if (!std::isfinite(v) || std::fabs(v) > 2.f) bad = true;
        r.engine.stats(); r.engine.activeVoices();
    }
    go = false;
    midi.join(); control.join();
    r.engine.collectRetired();
    CHECK(!bad, "output stayed finite and bounded for 3 s of concurrent MIDI, control and audio");
    CHECK(sent > 100, "the MIDI thread got through %d bursts", sent.load());
    return finish();
}
