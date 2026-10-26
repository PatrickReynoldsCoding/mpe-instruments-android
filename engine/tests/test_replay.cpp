// Plays a recording of a real Seaboard through the engine, so the stream that
// actually comes out of the hardware (not a tidy made-up one) is tested every run.
#include <string>
#include "check.h"
#include "replay.h"

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
    return finish();
}
