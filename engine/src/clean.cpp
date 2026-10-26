#include "mpe/clean.h"

namespace mpe {

void Clean::endNote(int ch, uint8_t vel, int64_t t) {
    if (held_[ch] < 0) return;                      // nothing held: drop the stray off
    out(kNoteOff | ch, uint8_t(held_[ch]), vel, t);
    held_[ch] = -1;
}

void Clean::process(const Msg& in) {
    const int ch = in.channel();
    switch (in.type()) {
    case kNoteOn:
        if (in.d2 == 0) { endNote(ch, 0, in.timeNs); return; }
        if (held_[ch] >= 0) out(kNoteOff | ch, uint8_t(held_[ch]), 0, in.timeNs);   // old note never ended
        held_[ch] = in.d1;
        sink_(ctx_, in);
        return;
    case kNoteOff:
        endNote(ch, in.d2 ? in.d2 : 0, in.timeNs);
        return;
    case kController:
        if (in.d1 == 120 || in.d1 == 123) { panic(); return; }
        sink_(ctx_, in);
        return;
    case kPitchBend: case kChanPress: case kKeyPress: case kProgram:
        sink_(ctx_, in);
        return;
    default:
        return;
    }
}

void Clean::panic() {
    int n = 0;
    for (int ch = 0; ch < 16; ch++) {
        if (held_[ch] >= 0) { out(kNoteOff | ch, uint8_t(held_[ch]), 0, 0); held_[ch] = -1; n++; }
        out(kPitchBend | ch, 0, 64, 0);             // 8192 = centre
        out(kChanPress | ch, 0, 0, 0);
    }
    lastPanicReleased_ = n;
}

void Clean::sendMpeConfig() {
    out(kController | 0, 101, 0, 0); out(kController | 0, 100, 6, 0); out(kController | 0, 6, 15, 0);
    out(kController | 0, 101, 127, 0); out(kController | 0, 100, 127, 0);   // RPN null
}

}  // namespace mpe
