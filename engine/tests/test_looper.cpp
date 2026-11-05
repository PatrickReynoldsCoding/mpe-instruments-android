// The looper: record a loop, play it round, layer more over it with another sound.
#include "check.h"

using namespace mpe;

static void waitState(Rig& r, LoopState s, double maxSec = 0.5) {
    for (double t = 0; t < maxSec && r.engine.looper().view().state != s; t += 0.01) r.run(0.01);
}

int main() {
    {
        // Arm, play a note: recording starts with it. Stop after ~1 s, snapped to beats at 120 bpm.
        Rig r("synths/lead");
        Performance p; p.bpm = 120;
        r.engine.setPerformance(p);
        Looper& L = r.engine.looper();
        L.press();
        r.run(0.3);
        CHECK(L.view().state == LoopState::Armed, "looper: armed until the first note");
        r.noteOn(1, 60, 100); r.press(1, 90);
        r.run(0.05);
        CHECK(L.view().state == LoopState::Recording, "looper: the first note starts the recording");
        r.run(0.4);
        r.noteOff(1, 60);
        r.run(0.6);                                        // about 1.05 s recorded: 2 beats at 120 bpm
        L.press();
        r.run(0.02);
        LoopView v = L.view();
        CHECK(v.state == LoopState::Playing && v.layers == 1, "looper: stopping makes the first loop");
        CHECK(std::abs(v.length - kSr) < 2, "looper: its length snaps to whole beats (%d frames, 1 s = %d)", v.length, kSr);
        // It plays round: the note comes back every second, though nothing is played now.
        auto x = r.run(3.0);
        CHECK(rms(x, 0, x.size()) > 0.02, "looper: the loop plays round with nothing held (rms %.3f)", rms(x, 0, x.size()));
        double a = rms(x, 0, kSr), b = rms(x, kSr, 2 * kSr), c = rms(x, 2 * kSr, 3 * kSr);
        CHECK(std::fabs(a - b) < 0.1 * a && std::fabs(b - c) < 0.1 * a, "looper: each pass is the same (%.3f %.3f %.3f)", a, b, c);

        // A second layer on another sound: one pass, then it plays along with the first.
        r.engine.setInstrument(0, findInstrument("drums/kit"));
        r.run(0.05);
        L.press();
        r.run(0.02);
        CHECK(L.view().state == LoopState::Overdubbing, "looper: pressing again records a layer");
        r.noteOn(2, 36, 120);                             // a kick
        r.run(0.2);
        r.noteOff(2, 36);
        waitState(r, LoopState::Playing, 1.5);
        v = L.view();
        CHECK(v.state == LoopState::Playing && v.layers == 2, "looper: the layer ends by itself after one pass (%d layers)", v.layers);
        auto y = r.run(2.0);
        CHECK(rms(y, 0, y.size()) > rms(x, 0, x.size()) * 1.05, "looper: two layers are louder than one (%.3f vs %.3f)",
              rms(y, 0, y.size()), rms(x, 0, x.size()));

        // Mute the second, undo it, pause, clear.
        L.setLayer(1, 1, true);
        r.run(0.02);
        auto z = r.run(2.0);
        CHECK(std::fabs(rms(z, 0, z.size()) - rms(x, 0, x.size())) < 0.1 * rms(x, 0, x.size()), "looper: a muted layer is silent");
        L.setLayer(1, 1, false);
        L.undo();
        r.run(0.02);
        CHECK(L.view().layers == 1, "looper: undo drops the last layer");
        L.setPaused(true);
        r.run(0.05);
        auto q = r.run(0.5);
        CHECK(rms(q, 0, q.size()) < 1e-4, "looper: paused is silent (%.5f)", rms(q, 0, q.size()));
        L.setPaused(false);
        L.clear();
        r.run(0.05);
        auto e = r.run(0.5);
        CHECK(L.view().state == LoopState::Empty && rms(e, 0, e.size()) < 1e-4, "looper: clear empties it");
        L.collect();
    }
    {
        // Without snap the length is what was played; armed then pressed again cancels.
        Rig r("synths/lead");
        Looper& L = r.engine.looper();
        L.setSnap(false);
        L.press(); r.run(0.05);
        L.press(); r.run(0.05);
        CHECK(L.view().state == LoopState::Empty, "looper: pressing while armed cancels");
        L.press(); r.run(0.05);
        r.noteOn(1, 60, 100); r.press(1, 90);
        r.run(0.7);
        L.press();
        r.run(0.01);
        const int len = L.view().length;
        CHECK(std::abs(len - int(0.7 * kSr)) < 256 + 64, "looper: without snap the loop is as long as played (%d frames)", len);
        r.noteOff(1, 60);
        L.collect();
    }
    {
        // Independent loops: a 1-beat loop and a 3-beat loop, each its own length; the second
        // records only what is played, not the first loop.
        Rig r("synths/lead");
        Performance p; p.bpm = 120;
        r.engine.setPerformance(p);
        Looper& A = r.engine.looper(0);
        Looper& B = r.engine.looper(1);
        A.press(); r.run(0.02);
        r.noteOn(1, 60, 100); r.press(1, 90); r.run(0.3); r.noteOff(1, 60); r.run(0.2);
        A.press(); r.run(0.02);
        CHECK(A.view().length == kSr / 2, "loops: the first is one beat (%d frames)", A.view().length);
        B.press(); r.run(0.3);
        CHECK(B.view().state == LoopState::Armed, "loops: a second loop arms while the first plays (the first's notes do not start it)");
        r.noteOn(2, 67, 100); r.press(2, 90); r.run(0.6); r.noteOff(2, 67); r.run(0.9);
        B.press(); r.run(0.02);
        CHECK(B.view().length == 3 * kSr / 2, "loops: the second has its own length, three beats (%d frames)", B.view().length);
        CHECK(A.view().state == LoopState::Playing && B.view().state == LoopState::Playing, "loops: both play");
        // Mute the first: what is left is the second alone, so it must not hold the first's note.
        A.setMuted(true);
        r.run(0.05);
        auto x = r.run(3.0);
        double lead = rms(x, 0, x.size());
        A.clear(); r.run(0.05);
        auto y = r.run(3.0);
        CHECK(lead > 0.02 && std::fabs(rms(y, 0, y.size()) - lead) < 0.05 * lead,
              "loops: muting or clearing the first leaves the second as it was (%.4f vs %.4f)", lead, rms(y, 0, y.size()));
        CHECK(B.view().state == LoopState::Playing && A.view().state == LoopState::Empty, "loops: clearing one leaves the other");
        B.clear(); r.run(0.02);
        A.collect(); B.collect();
    }
    return finish();
}
