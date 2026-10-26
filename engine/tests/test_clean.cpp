// Clean-up core and MIDI parser: no audio, no Faust needed at run time.
#include <vector>
#include "check.h"
#include "mpe/clean.h"
#include "mpe/midi.h"

using namespace mpe;

struct Capture {
    std::vector<Msg> out;
    static void sink(void* self, const Msg& m) { static_cast<Capture*>(self)->out.push_back(m); }
};
static int count(const Capture& c, uint8_t type, int ch = -1) {
    int n = 0;
    for (const Msg& m : c.out) if (m.type() == type && (ch < 0 || m.channel() == ch)) n++;
    return n;
}
static Msg msg(uint8_t status, uint8_t d1, uint8_t d2) { Msg m; m.status = status; m.d1 = d1; m.d2 = d2; return m; }

int main() {
    {   Capture c; Clean cl(Capture::sink, &c);
        cl.process(msg(0x91, 60, 80));
        cl.process(msg(0x81, 60, 0));
        CHECK(count(c, kNoteOn) == 1 && count(c, kNoteOff) == 1, "normal note: one on, one off");
        CHECK(cl.heldCount() == 0, "nothing held afterwards");
    }
    {   Capture c; Clean cl(Capture::sink, &c);
        cl.process(msg(0x91, 60, 80));
        cl.process(msg(0x91, 64, 80));
        CHECK(count(c, kNoteOff, 1) == 1 && c.out[1].d1 == 60, "note-on on a held channel first ends the old note");
        CHECK(cl.heldCount() == 1, "the new note is held");
    }
    {   Capture c; Clean cl(Capture::sink, &c);
        cl.process(msg(0x81, 60, 0));
        CHECK(c.out.empty(), "stray note-off is dropped");
    }
    {   Capture c; Clean cl(Capture::sink, &c);
        cl.process(msg(0x92, 60, 80));
        cl.process(msg(0x92, 60, 0));
        CHECK(count(c, kNoteOff, 2) == 1, "note-on velocity 0 becomes a note-off");
    }
    {   Capture c; Clean cl(Capture::sink, &c);
        for (int ch = 1; ch <= 5; ch++) cl.process(msg(0x90 | ch, 50 + ch, 90));
        cl.process(msg(0xB0, 123, 0));
        CHECK(count(c, kNoteOff) == 5 && cl.heldCount() == 0, "CC123 ends all 5 held notes");
        CHECK(cl.lastPanicReleased() == 5, "panic reports 5 released");
    }
    {   Capture c; Clean cl(Capture::sink, &c);
        cl.process(msg(0x93, 70, 90));
        c.out.clear();
        cl.panic();
        int centre = 0, zero = 0;
        for (const Msg& m : c.out) {
            if (m.type() == kPitchBend && m.d1 == 0 && m.d2 == 64) centre++;
            if (m.type() == kChanPress && m.d1 == 0) zero++;
        }
        CHECK(centre == 16 && zero == 16, "panic centres bend and zeroes pressure on all 16 channels");
    }
    {   Capture c; Clean cl(Capture::sink, &c);
        cl.process(msg(0xE1, 1, 2)); cl.process(msg(0xD1, 50, 0)); cl.process(msg(0xB1, 74, 90));
        CHECK(c.out.size() == 3, "bend, pressure and slide pass through unchanged");
    }
    {   Capture c; Clean cl(Capture::sink, &c); cl.sendMpeConfig();
        CHECK(c.out.size() == 5 && c.out[2].d1 == 6 && c.out[2].d2 == 15, "MPE config message: RPN 6, 15 members");
    }

    // ---- parser ----
    auto parse = [](std::vector<uint8_t> bytes) { std::vector<Msg> v; Parser p; p.feed(bytes.data(), (int)bytes.size(), 5, [&](const Msg& m) { v.push_back(m); }); return v; };
    {   auto v = parse({0x91, 60, 80, 0x81, 60, 0});
        CHECK(v.size() == 2 && v[0].type() == kNoteOn && v[1].type() == kNoteOff, "parser: two messages in one chunk");
        CHECK(v[0].timeNs == 5, "parser: keeps the arrival time");
    }
    {   auto v = parse({0xE1, 0, 64, 1, 64, 2, 64});
        CHECK(v.size() == 3, "parser: running status (three bends after one status byte)");
    }
    {   auto v = parse({0xD1, 40, 50});
        CHECK(v.size() == 2 && v[0].d1 == 40 && v[1].d1 == 50, "parser: channel pressure has one data byte");
    }
    {   auto v = parse({0x91, 60, 0xF8, 80});
        CHECK(v.size() == 1 && v[0].d2 == 80, "parser: real-time clock byte in the middle of a message is ignored");
    }
    {   auto v = parse({0xF0, 1, 2, 3, 0xF7, 0x91, 60, 80});
        CHECK(v.size() == 1 && v[0].type() == kNoteOn, "parser: sysex is skipped");
    }
    {   auto v = parse({60, 80, 0x91, 61, 81});
        CHECK(v.size() == 1 && v[0].d1 == 61, "parser: data bytes with no status are dropped");
    }
    {   Parser p; std::vector<Msg> v; auto s = [&](const Msg& m) { v.push_back(m); };
        uint8_t a[] = {0x91, 60}; uint8_t b[] = {80};
        p.feed(a, 2, 0, s); p.feed(b, 1, 0, s);
        CHECK(v.size() == 1 && v[0].d2 == 80, "parser: a message split across two chunks");
    }
    {   Msg m = msg(0xE0, 0, 64); CHECK(bend14(m) == 0, "bend14: centre is 0");
        m = msg(0xE0, 127, 127); CHECK(bend14(m) == 8191, "bend14: max");
        m = msg(0xE0, 0, 0); CHECK(bend14(m) == -8192, "bend14: min");
    }
    return finish();
}
