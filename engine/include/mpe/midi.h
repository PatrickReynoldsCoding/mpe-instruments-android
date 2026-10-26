// MIDI byte stream -> complete channel messages.
//
// Android hands over chunks that can hold several messages, use running status,
// or put real-time bytes in the middle of a message. This turns that into one
// Msg at a time and ignores what the engine does not use (sysex, system common).
#pragma once
#include <cstdint>

namespace mpe {

struct Msg {
    uint8_t status = 0;      // 0x80..0xEF, channel in the low nibble
    uint8_t d1 = 0, d2 = 0;
    int64_t timeNs = 0;      // when the bytes arrived (steady clock), 0 if unknown

    uint8_t type() const { return status & 0xF0; }
    uint8_t channel() const { return status & 0x0F; }
};

constexpr uint8_t kNoteOff = 0x80, kNoteOn = 0x90, kKeyPress = 0xA0, kController = 0xB0,
                  kProgram = 0xC0, kChanPress = 0xD0, kPitchBend = 0xE0;

inline int bend14(const Msg& m) { return (m.d1 | (m.d2 << 7)) - 8192; }   // -8192..8191

class Parser {
public:
    // Feed one byte. Calls sink(msg) for each complete channel message.
    template <typename Sink>
    void feed(uint8_t b, int64_t timeNs, Sink&& sink) {
        if (b >= 0xF8) return;                       // real-time: may sit anywhere, changes nothing
        if (b & 0x80) {                              // a status byte
            if (b == 0xF0) { inSysex_ = true; running_ = 0; have_ = 0; return; }
            if (b == 0xF7) { inSysex_ = false; return; }
            inSysex_ = false;
            if (b >= 0xF0) { running_ = 0; have_ = 0; return; }   // system common: skipped
            running_ = b;
            have_ = 0;
            return;
        }
        if (inSysex_ || !running_) return;           // data byte with nothing to attach to
        data_[have_++] = b;
        if (have_ == need(running_)) {
            Msg m;
            m.status = running_;
            m.d1 = data_[0];
            m.d2 = have_ > 1 ? data_[1] : 0;
            m.timeNs = timeNs;
            have_ = 0;                               // running status: keep `running_`
            sink(m);
        }
    }

    template <typename Sink>
    void feed(const uint8_t* bytes, int n, int64_t timeNs, Sink&& sink) {
        for (int i = 0; i < n; i++) feed(bytes[i], timeNs, sink);
    }

    void reset() { running_ = 0; have_ = 0; inSysex_ = false; }

private:
    static int need(uint8_t status) {
        uint8_t t = status & 0xF0;
        return (t == kProgram || t == kChanPress) ? 1 : 2;
    }
    uint8_t running_ = 0, have_ = 0;
    uint8_t data_[2] = {0, 0};
    bool inSysex_ = false;
};

}  // namespace mpe
