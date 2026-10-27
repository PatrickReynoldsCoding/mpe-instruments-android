#include "mpe/engine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include "faust/dsp/dsp.h"
#include "faust/gui/MapUI.h"

namespace mpe {

static const int kChannels = 16;      // MPE: channel 0 is the master, 1..15 carry notes
static const int kMaxBlock = 64;      // control values are refreshed at least this often

int64_t steadyNowNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}

// ---- instrument copies, one per MPE channel (one per finger) --------------------------------

struct Slot {
    dsp* d = nullptr;
    MapUI ui;
    // A control is null when the instrument does not use it (Faust drops unused ones).
    FAUSTFLOAT *freq = nullptr, *gate = nullptr, *pressure = nullptr, *slide = nullptr, *velocity = nullptr;
};

struct InstrumentSet {
    int index;
    int outputs = 1;
    Slot slots[kChannels];
    InstrumentSet(int idx, const InstrumentInfo& info, int sr) : index(idx) {
        for (int ch = 1; ch < kChannels; ch++) {
            Slot& s = slots[ch];
            s.d = info.create();
            s.d->init(sr);
            s.d->buildUserInterface(&s.ui);
            s.freq = s.ui.getParamZone("freq");
            s.gate = s.ui.getParamZone("gate");
            s.pressure = s.ui.getParamZone("pressure");
            s.slide = s.ui.getParamZone("slide");
            s.velocity = s.ui.getParamZone("velocity");
            outputs = std::min(2, s.d->getNumOutputs());
        }
    }
    ~InstrumentSet() { for (Slot& s : slots) delete s.d; }
};

// ---- per-channel MPE state and playback state -----------------------------------------------

struct Engine::Channel {
    // from MIDI
    int note = -1;                  // -1 = no note held
    int noteOnCount = 0;
    float bend = 0, pressure = 0, slide = 0.5f, velocity = 0;
    // playback
    int seenNoteOn = 0;
    bool sounding = false;
    float freq = 440, pressureS = 0, slideS = 0.5f, level = 0;
    int quietSamples = 0;
};

static float smoothTo(float cur, float target, float coef) { return cur + (target - cur) * coef; }
static float noteToHz(float note) { return 440.f * std::pow(2.f, (note - 69.f) / 12.f); }

Engine::Engine(int sampleRate) : sr_(sampleRate), ch_(new Channel[kChannels]) {}

Engine::~Engine() {
    delete cur_;
    delete pending_.exchange(nullptr);
    InstrumentSet* s;
    while (retired_.pop(s)) delete s;
}

bool Engine::pushMidi(const Msg& m) {
    if (queue_.push(m)) return true;
    overflow_ = true;
    dropped_++;
    return false;
}

bool Engine::setInstrument(int index) {
    int n = 0;
    const InstrumentInfo* all = instruments(n);
    if (index < 0 || index >= n) return false;
    InstrumentSet* fresh = new InstrumentSet(index, all[index], sr_);
    delete pending_.exchange(fresh);        // a still-unclaimed older request is replaced
    current_ = index;
    collectRetired();
    return true;
}

void Engine::collectRetired() {
    InstrumentSet* s;
    while (retired_.pop(s)) delete s;
}

void Engine::apply(const Msg& m) {
    const int ch = m.channel();
    Channel& c = ch_[ch];
    switch (m.type()) {
    case kNoteOn:
        if (ch == 0) return;
        if (m.d2 > 0) {
            c.bend = 0;
            c.pressure = 0;
            c.velocity = m.d2 / 127.f;
            c.note = m.d1;
            c.noteOnCount++;
        } else {
            c.note = -1;
        }
        return;
    case kNoteOff:
        if (ch != 0) c.note = -1;
        return;
    case kPitchBend: c.bend = bend14(m) / 8192.f * bendRange_.load(); return;
    case kChanPress: c.pressure = m.d1 / 127.f; return;
    case kController:
        if (m.d1 == 74 && ch != 0) c.slide = m.d2 / 127.f;
        else if (m.d1 == 120 || m.d1 == 123) panicFlag_ = true;
        return;
    default: return;
    }
}

void Engine::render(float* out, int frames, int64_t nowNs) {
    const int64_t t0 = steadyNowNs();

    if (InstrumentSet* fresh = pending_.exchange(nullptr)) {        // instrument switch
        if (cur_ && !retired_.push(cur_)) delete cur_;              // (only if the control side never collects)
        cur_ = fresh;
        for (int ch = 1; ch < kChannels; ch++) {
            ch_[ch].sounding = false;
            if (ch_[ch].note >= 0) ch_[ch].seenNoteOn = ch_[ch].noteOnCount - 1;   // restart held notes
        }
    }

    // Apply every queued MIDI message before this block, so the block already plays it.
    Msg m;
    int got = 0;
    while (queue_.pop(m)) {
        apply(m);
        got++;
        if (m.timeNs > 0) {
            double age = (nowNs - m.timeNs) / 1e6;
            if (age < 0) age = 0;
            ageSum_.store(ageSum_.load(std::memory_order_relaxed) + age, std::memory_order_relaxed);
            ageCount_++;
            ageLast_ = age;
            if (age > ageMax_.load(std::memory_order_relaxed)) ageMax_ = age;
        }
    }
    if (got) { events_ += got; lastEventFrame_ = frameCounter_; }

    // Safety: a lost event, or a silent link while a note is held, must not leave a note stuck.
    bool doPanic = panicFlag_.exchange(false) || overflow_.exchange(false);
    const int wd = watchdogMs_.load();
    if (!doPanic && wd > 0 && !got) {
        bool any = false;
        for (int ch = 1; ch < kChannels; ch++) any = any || ch_[ch].note >= 0;
        if (any && (frameCounter_ - lastEventFrame_) * 1000 / sr_ > wd) doPanic = true;
    }
    if (doPanic) {
        for (int ch = 0; ch < kChannels; ch++) { ch_[ch].note = -1; ch_[ch].pressure = 0; ch_[ch].bend = 0; }
        panics_++;
    }

    float vol = volume_.load();
    const int transpose = transpose_.load();
    const float velScale = velocityScale_.load();
    int voices = 0;

    float inBuf[kMaxBlock] = {0}, bufL[kMaxBlock], bufR[kMaxBlock], mixL[kMaxBlock], mixR[kMaxBlock];
    float* ins[1] = { inBuf };
    int done = 0;
    while (done < frames) {
        const int n = std::min(kMaxBlock, frames - done);
        const float dt = float(n) / sr_;
        auto coef = [&](float tau) { return 1.f - std::exp(-dt / tau); };
        const float cFreq = coef(0.004f), cPress = coef(0.012f), cSlide = coef(0.02f);
        const int quietLimit = int(0.4f * sr_);

        std::fill(mixL, mixL + n, 0.f);
        std::fill(mixR, mixR + n, 0.f);
        if (cur_) for (int chn = 1; chn < kChannels; chn++) {
            Slot& sl = cur_->slots[chn];
            Channel& c = ch_[chn];
            const bool held = c.note >= 0;

            if (c.noteOnCount != c.seenNoteOn) {                     // a new note on this channel
                c.seenNoteOn = c.noteOnCount;
                if (!held) c.sounding = false;
                else {
                    c.sounding = true;
                    c.quietSamples = 0;
                    c.freq = noteToHz(c.note + transpose + c.bend);
                    c.pressureS = c.pressure;
                    c.slideS = c.slide;
                    sl.d->instanceClear();                           // so `gate > gate'` sees a clean 0 -> 1
                }
            }
            if (!c.sounding) continue;

            if (held) {                                              // after release, freeze pitch so ringing notes don't drift
                c.freq = smoothTo(c.freq, noteToHz(c.note + transpose + c.bend), cFreq);
                c.pressureS = smoothTo(c.pressureS, c.pressure, cPress);
                c.slideS = smoothTo(c.slideS, c.slide, cSlide);
            }
            c.freq = std::min(std::max(c.freq, 20.f), 5000.f);

            if (sl.freq) *sl.freq = c.freq;
            if (sl.gate) *sl.gate = held ? 1.f : 0.f;
            if (sl.pressure) *sl.pressure = c.pressureS;
            if (sl.slide) *sl.slide = c.slideS;
            if (sl.velocity) *sl.velocity = std::min(1.f, c.velocity * velScale);

            float* outs[2] = { bufL, bufR };
            sl.d->compute(n, ins, outs);
            const float* r = cur_->outputs > 1 ? bufR : bufL;
            float e = 0;
            for (int i = 0; i < n; i++) e += bufL[i] * bufL[i] + r[i] * r[i];
            if (!std::isfinite(e) || e > 1e6f) {
                // The instrument blew up (NaN/Inf or runaway feedback, e.g. a note far outside its
                // range). Drop this block, reset the voice, and let the rest of the mix play on.
                sl.d->instanceClear();
                c.sounding = false;
                c.level = 0;
                voiceResets_++;
                continue;
            }
            for (int i = 0; i < n; i++) { mixL[i] += bufL[i]; mixR[i] += r[i]; }
            c.level = smoothTo(c.level, std::sqrt(e / (2 * n)), 0.2f);
            if (!held) c.quietSamples = (c.level < 3e-5f) ? c.quietSamples + n : 0;
            if (!held && c.quietSamples > quietLimit) c.sounding = false;     // fully decayed
        }
        for (int i = 0; i < n; i++) {                                // soft limiter
            out[2 * (done + i)] = std::tanh(mixL[i] * vol * 1.5f) * 0.7f;
            out[2 * (done + i) + 1] = std::tanh(mixR[i] * vol * 1.5f) * 0.7f;
        }
        done += n;
    }
    for (int chn = 1; chn < kChannels; chn++) voices += ch_[chn].sounding;
    activeVoices_ = voices;
    frameCounter_ += frames;

    const double spent = (steadyNowNs() - t0) / 1e9, budget = double(frames) / sr_;
    const double load = spent / budget;
    loadSum_.store(loadSum_.load(std::memory_order_relaxed) + load, std::memory_order_relaxed);
    if (load > loadPeak_.load(std::memory_order_relaxed)) loadPeak_ = load;
    blocks_++;
}

Stats Engine::stats() const {
    Stats s;
    uint64_t n = ageCount_.load();
    s.eventAgeAvgMs = n ? ageSum_.load() / n : 0;
    s.eventAgeMaxMs = ageMax_.load();
    s.eventAgeLastMs = ageLast_.load();
    s.blocks = blocks_.load();
    s.loadAvg = s.blocks ? loadSum_.load() / s.blocks : 0;
    s.loadPeak = loadPeak_.load();
    s.events = events_.load();
    s.dropped = dropped_.load();
    s.panics = panics_.load();
    s.voiceResets = voiceResets_.load();
    return s;
}

void MidiInput::feed(const uint8_t* bytes, int n, int64_t timeNs) {
    parser_.feed(bytes, n, timeNs, [this](const Msg& m) { clean_.process(m); });
}

}  // namespace mpe
