#include "mpe/looper.h"
#include <algorithm>
#include <cmath>

namespace mpe {

// Layers are stored at half scale, so a mix up to twice full scale still fits in 16 bits.
static constexpr float kStore = 32767.f * 0.5f, kLoad = 1.f / kStore;

static int16_t toS16(float x) { return int16_t(std::lround(std::min(std::max(x * kStore, -32767.f), 32767.f))); }

Looper::Looper(int sampleRate) : sr_(sampleRate), fade_(std::max(1, sampleRate / 500)) {}

Looper::~Looper() {
    for (Layer& l : layers_) delete[] l.buf;
    Cmd c;
    while (cmds_.pop(c)) delete[] c.buf;
    collect();
}

void Looper::collect() {
    int16_t* b;
    while (retired_.pop(b)) delete[] b;
}

void Looper::press() {
    collect();
    const LoopState s = LoopState(pubState_.load());
    switch (s) {
    case LoopState::Empty: {
        const int frames = kMaxSeconds * sr_;
        if (!cmds_.push({Arm, new int16_t[size_t(frames) * 2](), frames})) return;
        break;
    }
    case LoopState::Armed: cmds_.push({Cancel}); break;
    case LoopState::Recording: cmds_.push({Stop}); break;
    case LoopState::Playing: {
        const int frames = pubLength_.load();
        if (frames <= 0 || pubCount_.load() >= kMaxLayers) return;
        int16_t* b = new int16_t[size_t(frames) * 2]();
        if (!cmds_.push({Overdub, b, frames})) delete[] b;
        break;
    }
    case LoopState::Overdubbing: cmds_.push({Stop}); break;
    }
}

void Looper::undo() { cmds_.push({Undo}); }
void Looper::clear() { cmds_.push({Clear}); }
void Looper::setPaused(bool p) { cmds_.push({p ? Pause : Resume}); }
void Looper::setLayer(int i, float volume, bool muted) { Cmd c{SetLayer}; c.layer = i; c.vol = volume; c.mute = muted; cmds_.push(c); }

LoopView Looper::view() const {
    LoopView v;
    v.state = LoopState(pubState_.load(std::memory_order_relaxed));
    v.length = pubLength_.load(std::memory_order_relaxed);
    v.pos = pubPos_.load(std::memory_order_relaxed);
    v.layers = pubCount_.load(std::memory_order_relaxed);
    v.paused = pubPaused_.load(std::memory_order_relaxed);
    for (int i = 0; i < kMaxLayers; i++) v.level[i] = pubLevel_[i].load(std::memory_order_relaxed);
    return v;
}

// The first loop ends: its length is what was recorded, or that rounded to whole beats. Then it
// plays from where the recording got to, so the time carries straight on.
void Looper::endFirst(float bpm) {
    Layer& L = layers_[0];
    int len = rec_;
    if (snap_.load(std::memory_order_relaxed) && bpm > 0) {
        const double beat = 60.0 * sr_ / bpm;
        const int beats = std::max(1, int(std::lround(rec_ / beat)));
        len = std::min(int(std::lround(beats * beat)), L.frames);
    }
    if (len < fade_ * 4) {                                     // nothing worth keeping
        retire(L.buf); L = Layer(); state_ = LoopState::Empty; rec_ = 0; return;
    }
    // Fade the ends, so the seam where the loop goes round does not click.
    const int end = std::min(rec_, len);
    for (int i = 0; i < fade_ && i < end; i++) {
        const float g = float(i) / fade_;
        L.buf[2 * i] = int16_t(L.buf[2 * i] * g); L.buf[2 * i + 1] = int16_t(L.buf[2 * i + 1] * g);
        const int j = end - 1 - i;
        L.buf[2 * j] = int16_t(L.buf[2 * j] * g); L.buf[2 * j + 1] = int16_t(L.buf[2 * j + 1] * g);
    }
    L.frames = len;
    L.peak = recPeak_;
    length_ = len;
    pos_ = rec_ % len;
    count_ = 1;
    state_ = LoopState::Playing;
}

// A layer ends (one whole pass, or stopped early): fade out where it stopped.
void Looper::endOverdub() {
    Layer& L = layers_[count_];
    if (rec_ < length_) {
        for (int i = 0; i < fade_ && i < rec_; i++) {
            const int j = ((pos_ - 1 - i) % length_ + length_) % length_;
            const float g = float(i) / fade_;
            L.buf[2 * j] = int16_t(L.buf[2 * j] * g); L.buf[2 * j + 1] = int16_t(L.buf[2 * j + 1] * g);
        }
    }
    L.peak = recPeak_;
    count_++;
    state_ = LoopState::Playing;
}

void Looper::process(float* l, float* r, int n, bool noteStarted, float bpm) {
    Cmd c;
    while (cmds_.pop(c)) {
        switch (c.op) {
        case Arm:
            if (state_ == LoopState::Empty) { layers_[0] = Layer{c.buf, c.frames}; state_ = LoopState::Armed; rec_ = 0; recPeak_ = 0; }
            else retire(c.buf);
            break;
        case Cancel:
            if (state_ == LoopState::Armed) { retire(layers_[0].buf); layers_[0] = Layer(); state_ = LoopState::Empty; }
            break;
        case Stop:
            if (state_ == LoopState::Recording) endFirst(bpm);
            else if (state_ == LoopState::Overdubbing) endOverdub();
            break;
        case Overdub:
            if (state_ == LoopState::Playing && count_ < kMaxLayers && c.frames == length_) {
                layers_[count_] = Layer{c.buf, c.frames};
                state_ = LoopState::Overdubbing; rec_ = 0; recPeak_ = 0; paused_ = false;
            } else retire(c.buf);
            break;
        case Undo:
            if (state_ == LoopState::Overdubbing) { retire(layers_[count_].buf); layers_[count_] = Layer(); state_ = LoopState::Playing; }
            else if (count_ > 0 && (state_ == LoopState::Playing)) {
                retire(layers_[--count_].buf); layers_[count_] = Layer();
                if (count_ == 0) { state_ = LoopState::Empty; length_ = pos_ = 0; paused_ = false; }
            } else if (state_ == LoopState::Armed || state_ == LoopState::Recording) {
                retire(layers_[0].buf); layers_[0] = Layer(); state_ = LoopState::Empty;
            }
            break;
        case Clear:
            for (Layer& L : layers_) { retire(L.buf); L = Layer(); }
            count_ = 0; state_ = LoopState::Empty; length_ = pos_ = rec_ = 0; paused_ = false;
            break;
        case Pause: if (state_ == LoopState::Playing) paused_ = true; break;
        case Resume: paused_ = false; break;
        case SetLayer:
            if (c.layer >= 0 && c.layer < kMaxLayers) { layers_[c.layer].vol = c.vol; layers_[c.layer].mute = c.mute; }
            break;
        }
    }

    if (state_ == LoopState::Armed && noteStarted) state_ = LoopState::Recording;

    if (state_ == LoopState::Recording) {
        Layer& L = layers_[0];
        for (int i = 0; i < n && rec_ < L.frames; i++, rec_++) {
            L.buf[2 * rec_] = toS16(l[i]); L.buf[2 * rec_ + 1] = toS16(r[i]);
            recPeak_ = std::max(recPeak_, std::max(std::fabs(l[i]), std::fabs(r[i])));
        }
        if (rec_ >= L.frames) endFirst(bpm);                   // the longest a loop can be
    } else if ((state_ == LoopState::Playing && !paused_) || state_ == LoopState::Overdubbing) {
        const bool dub = state_ == LoopState::Overdubbing;
        for (int i = 0; i < n; i++) {
            const float inL = l[i], inR = r[i];
            float ol = 0, orr = 0;
            for (int k = 0; k < count_; k++) {
                const Layer& L = layers_[k];
                if (L.mute) continue;
                ol += L.buf[2 * pos_] * kLoad * L.vol; orr += L.buf[2 * pos_ + 1] * kLoad * L.vol;
            }
            if (dub && rec_ < length_) {
                Layer& D = layers_[count_];
                const float g = rec_ < fade_ ? float(rec_) / fade_ : rec_ >= length_ - fade_ ? float(length_ - 1 - rec_) / fade_ : 1.f;
                D.buf[2 * pos_] = toS16(inL * g); D.buf[2 * pos_ + 1] = toS16(inR * g);
                recPeak_ = std::max(recPeak_, std::max(std::fabs(inL), std::fabs(inR)));
                if (++rec_ >= length_) { pos_ = (pos_ + 1) % length_; endOverdub(); l[i] += ol; r[i] += orr; continue; }
            }
            l[i] += ol; r[i] += orr;
            pos_ = (pos_ + 1) % length_;
        }
    }

    pubState_.store(int(state_), std::memory_order_relaxed);
    pubLength_.store(length_, std::memory_order_relaxed);
    pubPos_.store(state_ == LoopState::Recording ? rec_ : pos_, std::memory_order_relaxed);
    pubCount_.store(count_, std::memory_order_relaxed);
    pubPaused_.store(paused_, std::memory_order_relaxed);
    for (int i = 0; i < kMaxLayers; i++) pubLevel_[i].store(i < count_ ? layers_[i].peak : 0.f, std::memory_order_relaxed);
}

}  // namespace mpe
