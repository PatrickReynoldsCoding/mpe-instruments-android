// Plays a recording of a real Seaboard through the engine, so the stream that
// actually comes out of the hardware (not a tidy made-up one) is tested every run.
#include <string>
#include "check.h"
#include "replay.h"
#include "midifile.h"

int main() {
    const std::string path = "../../captures/seaboard-bowing.aseqdump.txt";
    auto msgs = loadAseqdump(path);
    CHECK(msgs.size() > 200, "capture loaded (%zu messages)", msgs.size());

    for (const char* inst : {"strings/cello", "strings/violin", "winds/flute", "synths/pad"}) {
        Rig r(inst);
        std::vector<float> all;
        int ons = 0;
        for (const auto& m : msgs) {
            if (m.type() == 0x90 && m.d2 > 0) ons++;
            r.send(m.status, m.d1, m.d2);
            auto x = r.run(0.002, 96);                 // about the pace of a live stream
            all.insert(all.end(), x.begin(), x.end());
        }
        CHECK(ons > 0 && finite(all), "%s: %d note-ons replayed, output finite", inst, ons);
        CHECK(peak(all) <= 1.0f, "%s: no clipping (peak %.2f)", inst, peak(all));
        CHECK(rms(all, 0, all.size()) > 0.0005, "%s: the replay is audible", inst);
        CHECK(r.in.clean().heldCount() == 0, "%s: the capture ends with no note held", inst);
        auto tail = r.run(5.0);
        CHECK(rms(tail, tail.size() - size_t(0.3 * kSr), tail.size()) < 0.005, "%s: silent after the replay", inst);
        CHECK(r.engine.stats().dropped == 0, "%s: no events dropped", inst);
    }
    // Same replay, but the cable is pulled halfway through a note.
    {
        Rig r("strings/cello");
        size_t cut = 0, seenOn = 0;
        for (size_t i = 0; i < msgs.size(); i++) if (msgs[i].type() == 0x90 && msgs[i].d2 > 0 && ++seenOn == 3) cut = i + 20;
        for (size_t i = 0; i < cut && i < msgs.size(); i++) { r.send(msgs[i].status, msgs[i].d1, msgs[i].d2); r.run(0.002, 96); }
        int held = r.in.clean().heldCount();
        r.in.deviceLost();
        auto tail = r.run(5.0);
        CHECK(held > 0, "cable pulled while %d note(s) held", held);
        CHECK(rms(tail, tail.size() - size_t(0.3 * kSr), tail.size()) < 0.005, "pulled cable: silent afterwards");
        CHECK(r.in.clean().heldCount() == 0, "pulled cable: nothing left held");
    }

    // ---- the 3-minute session recorded with timing (captures/seaboard-session.mid) ----
    auto session = loadMidiFile("../../captures/seaboard-session.mid");
    double length = session.empty() ? 0 : session.back().seconds;
    CHECK(session.size() > 10000 && length > 60, "timed session loaded (%zu messages, %.0f s)", session.size(), length);

    // Replayed at real speed, each message arriving at its recorded time, 96-frame callbacks.
    // The session was played with the Seaboard shifted up about 5 octaves (notes 96..127), far above
    // every instrument's range. Raw: it must survive. Down 5 octaves (36..67): it must also be clean.
    for (int transpose : {0, -60})
    for (const char* inst : {"strings/cello", "voices/soprano", "plucked/nylon-guitar", "synths/fm-keys"}) {
        Rig r(inst);
        r.engine.setTranspose(transpose);
        printf("  -- %s, transpose %d\n", inst, transpose);
        const int block = 96;
        std::vector<float> buf(block * 2);
        size_t next = 0;
        int64_t frame = 0;
        const int64_t total = int64_t((length + 5.0) * kSr);
        int maxVoices = 0;
        float pk = 0;
        bool fin = true;
        double loudest = 0;
        const int64_t t0 = r.clockNs;
        while (frame < total) {
            const double blockEnd = double(frame + block) / kSr;
            while (next < session.size() && session[next].seconds < blockEnd) {
                const mpe::Msg& m = session[next].msg;
                uint8_t bytes[3] = {m.status, m.d1, m.d2};
                int n = (m.type() == mpe::kProgram || m.type() == mpe::kChanPress) ? 2 : 3;
                r.in.feed(bytes, n, t0 + int64_t(session[next].seconds * 1e9));
                next++;
            }
            r.engine.render(buf.data(), block, t0 + int64_t(1e9 * (frame + block) / kSr));
            for (int i = 0; i < block * 2; i++) { fin = fin && std::isfinite(buf[i]); pk = std::max(pk, std::fabs(buf[i])); }
            maxVoices = std::max(maxVoices, r.engine.activeVoices());
            frame += block;
        }
        (void)loudest;
        mpe::Stats st = r.engine.stats();
        // The recording ends with the cable pulled mid-note (one note still held). The app
        // calls deviceLost() when the device goes away; do the same here.
        CHECK(r.in.clean().heldCount() == 1, "%s: the recording ends with a note held (cable pulled)", inst);
        r.in.deviceLost();
        CHECK(fin && pk <= 1.0f, "%s: session at real speed, finite, peak %.2f", inst, pk);
        CHECK(maxVoices >= 3 && maxVoices <= 15, "%s: up to %d voices at once (chords spread over channels)", inst, maxVoices);
        CHECK(st.dropped == 0 && st.panics == 0, "%s: no events dropped, no panics during play", inst);
        CHECK(r.in.clean().heldCount() == 0, "%s: nothing held after the device is lost", inst);
        CHECK(st.eventAgeMaxMs <= 1000.0 * block / kSr + 0.01, "%s: every message played within one callback (max %.2f ms)", inst, st.eventAgeMaxMs);
        std::vector<float> tail(block * 2);
        double tailRms = 0;
        for (int i = 0; i < 3 * kSr / block; i++) r.engine.render(tail.data(), block, 0);    // let it ring out
        for (int i = 0; i < 20; i++) { r.engine.render(tail.data(), block, 0); for (float v : tail) tailRms += v * v; }
        CHECK(std::sqrt(tailRms / (20 * block * 2)) < 0.005, "%s: silent after the session", inst);
        if (transpose == 0) {
            if (st.voiceResets) printf("      note: %s needed %llu voice reset(s) far above its range (NaN/Inf caught)\n", inst, (unsigned long long)st.voiceResets);
        } else {
            CHECK(st.voiceResets == 0, "%s: in a normal range no voice ever blows up (%llu resets)", inst, (unsigned long long)st.voiceResets);
        }
    }
    {   // The same session 8x faster than played: a burst far denser than a person makes.
        Rig r("strings/violin");
        const int block = 96;
        std::vector<float> buf(block * 2);
        size_t next = 0;
        int64_t frame = 0;
        while (next < session.size()) {
            const double blockEnd = double(frame + block) / kSr;
            while (next < session.size() && session[next].seconds / 8 < blockEnd) {
                const mpe::Msg& m = session[next].msg;
                uint8_t bytes[3] = {m.status, m.d1, m.d2};
                r.in.feed(bytes, (m.type() == mpe::kProgram || m.type() == mpe::kChanPress) ? 2 : 3, r.clockNs);
                next++;
            }
            r.engine.render(buf.data(), block, r.clockNs);
            frame += block;
        }
        CHECK(r.engine.stats().dropped == 0, "8x speed: no events dropped (queue is large enough)");
    }
    return finish();
}
