// Keeps an MPE stream safe: every note-on gets a note-off. Same rules as
// `mpe-clean` in seaboard-mpe, but with no ALSA and no I/O, so it runs on
// Linux and Android alike.
//
//   * a note-on on a channel that still holds a note first ends the old note
//   * a note-off for a note that is not held is dropped
//   * note-on with velocity 0 is turned into a note-off
//   * all-notes-off / all-sound-off (CC 123 / 120) ends every held note
//   * panic() (device lost, buffer overflow) does the same and also centres bend
//     and zeroes pressure, so the next note does not start from a leftover value
#pragma once
#include "mpe/midi.h"

namespace mpe {

class Clean {
public:
    using Sink = void (*)(void* ctx, const Msg&);
    Clean(Sink sink, void* ctx) : sink_(sink), ctx_(ctx) { for (int& h : held_) h = -1; }

    void process(const Msg& in);
    void panic();                                   // returns how many held notes it ended
    int heldCount() const { int n = 0; for (int h : held_) n += h >= 0; return n; }
    int lastPanicReleased() const { return lastPanicReleased_; }

    // The MPE Configuration Message (RPN 6, lower zone, 15 members) on the master channel.
    void sendMpeConfig();

private:
    void out(uint8_t status, uint8_t d1, uint8_t d2, int64_t t) {
        Msg m; m.status = status; m.d1 = d1; m.d2 = d2; m.timeNs = t;
        sink_(ctx_, m);
    }
    void endNote(int ch, uint8_t vel, int64_t t);

    Sink sink_;
    void* ctx_;
    int held_[16];
    int lastPanicReleased_ = 0;
};

}  // namespace mpe
