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

// ---- fingers, parts and voices -----------------------------------------------------------
//
// A finger is one MPE channel: the note, bend, pressure and slide the controller sends on it.
// A part is an instrument and 15 voices (one copy of the instrument each). In play mode voice i
// follows finger i, as an MPE synth does. In arp mode the part's held fingers are played one at
// a time on a clock, each note still following its finger's bend, pressure and slide.
// Split sends each new finger to the left or right part by its note; Layer sends it to both.

struct Engine::Channel {
    int note = -1;                  // -1 = no note held
    int noteOnCount = 0;
    float bend = 0, pressure = 0, slide = 0.5f, velocity = 0;
    int parts = 1;                  // bit p: this finger plays part p (fixed at note on)
    uint32_t order = 0;             // press order, for ArpOrder::AsPlayed
};

struct Engine::Voice {
    int finger = -1, fingerNoteOn = 0;        // the finger it follows (arp: while that note is held)
    int note = -1;                  // base note, octave offset included
    bool gate = false, sounding = false, start = false;
    int seen = 0;                   // play mode: the finger's noteOnCount already handled
    float bend = 0, pressure = 0, slide = 0.5f, velocity = 0;   // last expression from the finger
    float freq = 440, pitch = 0, pressureS = 0, slideS = 0.5f, level = 0;
    float pull = 0, prevPitch = 0, speed = 0;   // pitch assist
    int quietSamples = 0;
    int64_t started = 0;
};

struct Engine::Part {
    InstrumentSet* set = nullptr;
    Voice v[kChannels];
    bool arp = false;               // the mode in effect
    bool running = false;
    double nextStep = 0;            // frame of the next arp step
    int64_t gateOff = INT64_MAX;
    uint32_t step = 0;
    struct Entry { int finger, noteOn, note; float bend, pressure, slide, velocity; uint32_t order; };
    Entry latch[kChannels];         // the last chord held, for latch
    int latchCount = 0;
};

static float smoothTo(float cur, float target, float coef) { return cur + (target - cur) * coef; }
static float noteToHz(float note) { return 440.f * std::pow(2.f, (note - 69.f) / 12.f); }

Engine::Engine(int sampleRate)
    : sr_(sampleRate), ch_(new Channel[kChannels]), parts_(new Part[2]),
      pubVoices_(new PubVoice[2 * kChannels]), pubFingers_(new PubFinger[kChannels]) {}

Engine::~Engine() {
    for (int p = 0; p < 2; p++) { delete parts_[p].set; delete pending_[p].exchange(nullptr); }
    delete pendingPerf_.exchange(nullptr);
    collectRetired();
}

bool Engine::pushMidi(const Msg& m) {
    if (queue_.push(m)) return true;
    overflow_ = true;
    dropped_++;
    return false;
}

bool Engine::setInstrument(int part, int index) {
    int n = 0;
    const InstrumentInfo* all = instruments(n);
    if (part < 0 || part > 1 || index < 0 || index >= n) return false;
    InstrumentSet* fresh = new InstrumentSet(index, all[index], sr_);
    delete pending_[part].exchange(fresh);  // a still-unclaimed older request is replaced
    current_[part] = index;
    collectRetired();
    return true;
}

void Engine::setPerformance(const Performance& p) {
    delete pendingPerf_.exchange(new Performance(p));
    collectRetired();
}

void Engine::collectRetired() {
    InstrumentSet* s;
    while (retired_.pop(s)) delete s;
    Performance* q;
    while (retiredPerf_.pop(q)) delete q;
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
            c.order = ++pressCounter_;
            c.parts = perf_.layout == Layout::Layer ? 3
                    : perf_.layout == Layout::Split ? (m.d1 < perf_.splitNote ? 1 : 2) : 1;
            recentNotes_[recentPos_] = m.d1;
            recentPos_ = (recentPos_ + 1) % 16;
            recentCount_ = std::min(recentCount_ + 1, 16);
            int sum = 0;
            for (int i = 0; i < recentCount_; i++) sum += recentNotes_[i];
            noteCenter_ = double(sum) / recentCount_;
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

void Engine::releasePart(int p) {
    Part& P = parts_[p];
    for (int i = 1; i < kChannels; i++) {
        Voice& v = P.v[i];
        v.gate = false;
        v.start = false;
        v.finger = -1;
        v.seen = ch_[i].noteOnCount;            // play mode: do not restart fingers already down
    }
    P.running = false;
    P.gateOff = INT64_MAX;
}

// Instruments and settings from the control thread, picked up between blocks.
void Engine::takeUpdates() {
    for (int p = 0; p < 2; p++) {
        if (InstrumentSet* fresh = pending_[p].exchange(nullptr)) {
            Part& P = parts_[p];
            if (P.set && !retired_.push(P.set)) delete P.set;    // (only if the control side never collects)
            P.set = fresh;
            for (int i = 1; i < kChannels; i++) {
                Voice& v = P.v[i];
                v.sounding = false;
                if (P.arp) { if (v.gate) v.start = true; }                 // the arp note goes on, on the new sound
                else if (ch_[i].note >= 0 && (ch_[i].parts >> p & 1)) v.seen = ch_[i].noteOnCount - 1;   // restart held notes
            }
        }
    }
    if (Performance* np = pendingPerf_.exchange(nullptr)) {
        for (int p = 0; p < 2; p++)
            if (np->part[p].arp != parts_[p].arp) { releasePart(p); parts_[p].arp = np->part[p].arp; }
        perf_ = *np;
        if (!retiredPerf_.push(np)) delete np;
    }
}

int Engine::freeVoice(int p, int64_t frame) {
    Part& P = parts_[p];
    int best = -1;
    for (int i = 1; i < kChannels; i++) if (!P.v[i].sounding && !P.v[i].start) return i;
    for (int i = 1; i < kChannels; i++)                       // the oldest one already released
        if (!P.v[i].gate && (best < 0 || P.v[i].started < P.v[best].started)) best = i;
    if (best > 0) return best;
    best = 1;
    for (int i = 2; i < kChannels; i++) if (P.v[i].started < P.v[best].started) best = i;
    (void)frame;
    return best;
}

void Engine::stepArp(int p, int64_t frame) {
    Part& P = parts_[p];
    const PartConfig& cfg = perf_.part[p];
    Part::Entry set[kChannels];
    int n = 0;
    for (int ch = 1; ch < kChannels; ch++) {
        const Channel& f = ch_[ch];
        if (f.note >= 0 && (f.parts >> p & 1))
            set[n++] = {ch, f.noteOnCount, f.note, f.bend, f.pressure, f.slide, f.velocity, f.order};
    }
    if (n) { std::copy(set, set + n, P.latch); P.latchCount = n; }
    else if (cfg.latch && P.latchCount) { n = P.latchCount; std::copy(P.latch, P.latch + n, set); }
    for (int i = 1; i < kChannels; i++) P.v[i].gate = false;  // end the last step's note (it rings on)
    if (!n) { P.running = false; P.gateOff = INT64_MAX; return; }

    if (cfg.order == ArpOrder::AsPlayed) std::sort(set, set + n, [](auto& a, auto& b) { return a.order < b.order; });
    else std::sort(set, set + n, [](auto& a, auto& b) { return a.note < b.note; });
    const int oct = std::min(std::max(cfg.octaves, 1), 4);
    const int len = n * oct;

    auto start = [&](const Part::Entry& e, int octave) {
        Voice& v = P.v[freeVoice(p, frame)];
        v.finger = e.finger;
        v.fingerNoteOn = e.noteOn;
        v.note = e.note + 12 * octave;
        v.bend = e.bend; v.pressure = e.pressure; v.slide = e.slide; v.velocity = e.velocity;
        v.gate = true;
        v.start = true;
        v.started = frame;
    };
    if (cfg.order == ArpOrder::Chord) {
        for (int i = 0; i < n; i++) start(set[i], int(P.step % oct));
    } else {
        int idx = int(P.step % len);
        if (cfg.order == ArpOrder::Down) idx = len - 1 - idx;
        else if (cfg.order == ArpOrder::UpDown) {
            const int per = len > 1 ? 2 * len - 2 : 1;
            const int k = int(P.step % per);
            idx = k < len ? k : per - k;
        } else if (cfg.order == ArpOrder::Random) {
            rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
            idx = int(rng_ % uint32_t(len));
        }
        start(set[idx % n], idx / n);
    }
    P.step++;
    arpSteps_++;

    const double stepLen = sr_ * 60.0 / (std::max(20.f, perf_.bpm) * std::max(0.25f, cfg.rate));
    if (!P.running) { P.running = true; P.nextStep = double(frame); }
    P.nextStep += stepLen;
    const float gate = std::min(std::max(cfg.gate, 0.05f), 1.f);
    P.gateOff = gate >= 0.999f ? INT64_MAX : frame + std::max<int64_t>(1, int64_t(gate * stepLen));
}

// Pitch assist: while a finger rests (pitch moving slower than 4 semitones/s), pull it towards
// the nearest note of the scale, by `assist`, over about 80 ms. A glide is left alone.
// Bend arrives in steps a few ms apart, so the speed is a peak that decays over ~30 ms; a block
// between two steps does not count as resting.
float Engine::assisted(float pitch, float& pull, float& prev, float& speed, float dt) const {
    speed = std::max(std::fabs(pitch - prev) / dt, speed * std::exp(-dt / 0.03f));
    prev = pitch;
    if (!perf_.scaleMask || perf_.assist <= 0) { pull = 0; return pitch; }
    pull = smoothTo(pull, speed < 4.f ? perf_.assist : 0.f, 1.f - std::exp(-dt / 0.08f));
    const float rel = pitch - perf_.scaleRoot;
    const int base = int(std::floor(rel));
    float nearest = rel, bestDist = 1e9f;
    for (int k = -6; k <= 7; k++) {
        const int cand = base + k;
        if (!(perf_.scaleMask >> (((cand % 12) + 12) % 12) & 1)) continue;
        const float d = std::fabs(cand - rel);
        if (d < bestDist) { bestDist = d; nearest = float(cand); }
    }
    return pitch + (nearest + perf_.scaleRoot - pitch) * pull;
}

void Engine::renderVoices(int p, float* mixL, float* mixR, int n, float dt) {
    Part& P = parts_[p];
    if (!P.set) return;
    const int transpose = transpose_.load() + perf_.part[p].transpose;
    const float velScale = velocityScale_.load(), gain = perf_.part[p].volume;
    auto coef = [&](float tau) { return 1.f - std::exp(-dt / tau); };
    const float cFreq = coef(0.004f), cPress = coef(0.012f), cSlide = coef(0.02f);
    const int quietLimit = int(0.4f * sr_);
    float inBuf[kMaxBlock] = {0}, bufL[kMaxBlock], bufR[kMaxBlock];
    float* ins[1] = { inBuf };

    for (int i = 1; i < kChannels; i++) {
        Slot& sl = P.set->slots[i];
        Voice& v = P.v[i];
        if (!P.arp) {                                            // play mode: voice i is finger i
            const Channel& f = ch_[i];
            const bool mine = f.parts >> p & 1;
            if (f.noteOnCount != v.seen) {
                v.seen = f.noteOnCount;
                if (mine) {
                    if (f.note < 0) v.sounding = false;          // on and off within one block
                    else { v.start = true; v.note = f.note; v.velocity = f.velocity; v.finger = i; v.fingerNoteOn = f.noteOnCount; }
                }
            }
            v.gate = mine && f.note >= 0;
        }
        // Expression: from the finger while it still holds this note, else the last values.
        if (v.finger > 0) {
            const Channel& f = ch_[v.finger];
            if (f.noteOnCount == v.fingerNoteOn && f.note >= 0) { v.bend = f.bend; v.pressure = f.pressure; v.slide = f.slide; }
        }
        const float target = v.note + transpose + v.bend;

        if (v.start) {                                           // a new note on this voice
            v.start = false;
            v.sounding = true;
            v.quietSamples = 0;
            v.prevPitch = target;
            v.speed = 0;
            v.pull = perf_.scaleMask ? perf_.assist : 0.f;
            v.pitch = assisted(target, v.pull, v.prevPitch, v.speed, dt);
            v.freq = noteToHz(v.pitch);
            v.pressureS = v.pressure;
            v.slideS = v.slide;
            sl.d->instanceClear();                               // so `gate > gate'` sees a clean 0 -> 1
        }
        if (!v.sounding) continue;

        if (v.gate) {                                            // after release, freeze pitch so ringing notes don't drift
            v.pitch = assisted(target, v.pull, v.prevPitch, v.speed, dt);
            v.freq = smoothTo(v.freq, noteToHz(v.pitch), cFreq);
            v.pressureS = smoothTo(v.pressureS, v.pressure, cPress);
            v.slideS = smoothTo(v.slideS, v.slide, cSlide);
        }
        v.freq = std::min(std::max(v.freq, 20.f), 5000.f);

        if (sl.freq) *sl.freq = v.freq;
        if (sl.gate) *sl.gate = v.gate ? 1.f : 0.f;
        if (sl.pressure) *sl.pressure = v.pressureS;
        if (sl.slide) *sl.slide = v.slideS;
        if (sl.velocity) *sl.velocity = std::min(1.f, v.velocity * velScale);

        float* outs[2] = { bufL, bufR };
        sl.d->compute(n, ins, outs);
        const float* r = P.set->outputs > 1 ? bufR : bufL;
        float e = 0;
        for (int k = 0; k < n; k++) e += bufL[k] * bufL[k] + r[k] * r[k];
        if (!std::isfinite(e) || e > 1e6f) {
            // The instrument blew up (NaN/Inf or runaway feedback, e.g. a note far outside its
            // range). Drop this block, reset the voice, and let the rest of the mix play on.
            sl.d->instanceClear();
            v.sounding = false;
            v.level = 0;
            voiceResets_++;
            continue;
        }
        for (int k = 0; k < n; k++) { mixL[k] += bufL[k] * gain; mixR[k] += r[k] * gain; }
        v.level = smoothTo(v.level, std::sqrt(e / (2 * n)), 0.2f);
        if (!v.gate) v.quietSamples = (v.level < 3e-5f) ? v.quietSamples + n : 0;
        if (!v.gate && v.quietSamples > quietLimit) v.sounding = false;     // fully decayed
    }
}

void Engine::publish() {
    for (int p = 0; p < 2; p++)
        for (int i = 1; i < kChannels; i++) {
            const Voice& v = parts_[p].v[i];
            PubVoice& o = pubVoices_[p * kChannels + i];
            o.on.store(v.sounding, std::memory_order_relaxed);
            o.finger.store(v.finger > 0 && ch_[v.finger].noteOnCount == v.fingerNoteOn && ch_[v.finger].note >= 0 ? v.finger : -1,
                           std::memory_order_relaxed);
            o.note.store(v.note, std::memory_order_relaxed);
            o.gate.store(v.gate, std::memory_order_relaxed);
            o.pitch.store(v.pitch, std::memory_order_relaxed);
            o.pressure.store(v.pressureS, std::memory_order_relaxed);
            o.slide.store(v.slideS, std::memory_order_relaxed);
            o.level.store(v.level, std::memory_order_relaxed);
        }
    const int transpose = transpose_.load();
    for (int ch = 1; ch < kChannels; ch++) {
        const Channel& c = ch_[ch];
        PubFinger& o = pubFingers_[ch];
        o.note.store(c.note, std::memory_order_relaxed);
        o.pitch.store(c.note + c.bend + transpose, std::memory_order_relaxed);
        o.pressure.store(c.pressure, std::memory_order_relaxed);
        o.slide.store(c.slide, std::memory_order_relaxed);
    }
}

int Engine::voices(VoiceView* out, int max) const {
    int n = 0;
    for (int p = 0; p < 2; p++)
        for (int i = 1; i < kChannels && n < max; i++) {
            const PubVoice& o = pubVoices_[p * kChannels + i];
            if (!o.on.load(std::memory_order_relaxed)) continue;
            VoiceView& v = out[n++];
            v.part = p;
            v.finger = o.finger.load(std::memory_order_relaxed);
            v.note = o.note.load(std::memory_order_relaxed);
            v.gate = o.gate.load(std::memory_order_relaxed);
            v.pitch = o.pitch.load(std::memory_order_relaxed);
            v.pressure = o.pressure.load(std::memory_order_relaxed);
            v.slide = o.slide.load(std::memory_order_relaxed);
            v.level = o.level.load(std::memory_order_relaxed);
        }
    return n;
}

int Engine::fingers(FingerView* out, int max) const {
    int n = 0;
    for (int ch = 1; ch < kChannels && n < max; ch++) {
        const PubFinger& o = pubFingers_[ch];
        const int note = o.note.load(std::memory_order_relaxed);
        if (note < 0) continue;
        out[n++] = {ch, note, o.pitch.load(std::memory_order_relaxed),
                    o.pressure.load(std::memory_order_relaxed), o.slide.load(std::memory_order_relaxed)};
    }
    return n;
}

void Engine::render(float* out, int frames, int64_t nowNs) {
    const int64_t t0 = steadyNowNs();
    takeUpdates();

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
        for (int p = 0; p < 2; p++) { releasePart(p); parts_[p].latchCount = 0; }   // a latched arp stops too
        panics_++;
    }

    const float vol = volume_.load();
    float mixL[kMaxBlock], mixR[kMaxBlock];
    int done = 0;
    while (done < frames) {
        const int64_t now = frameCounter_ + done;
        // Arp clock: steps and note ends land on the exact frame; blocks are cut there.
        int64_t next = now + std::min(kMaxBlock, frames - done);
        for (int p = 0; p < 2; p++) {
            Part& P = parts_[p];
            if (!P.arp || !P.set) continue;
            bool held = false;
            for (int ch = 1; ch < kChannels; ch++) held = held || (ch_[ch].note >= 0 && (ch_[ch].parts >> p & 1));
            if (!P.running && held) stepArp(p, now);                         // the first note plays at once
            else if (P.running && !held && !perf_.part[p].latch) stepArp(p, now);   // hand lifted: stop now
            if (P.running) {
                if (P.gateOff <= now) {
                    for (int i = 1; i < kChannels; i++) P.v[i].gate = false;
                    P.gateOff = INT64_MAX;
                }
                if (int64_t(P.nextStep) <= now) stepArp(p, now);
                if (P.running) next = std::min(next, std::max(now + 1, int64_t(P.nextStep)));
                if (P.gateOff != INT64_MAX) next = std::min(next, std::max(now + 1, P.gateOff));
            }
        }
        const int n = int(next - now);
        const float dt = float(n) / sr_;

        std::fill(mixL, mixL + n, 0.f);
        std::fill(mixR, mixR + n, 0.f);
        for (int p = 0; p < 2; p++) {
            if (p == 1 && perf_.layout == Layout::Single) {          // part 1 off: let its notes ring out
                bool any = false;
                for (int i = 1; i < kChannels; i++) any = any || parts_[1].v[i].sounding;
                if (!any) continue;
            }
            renderVoices(p, mixL, mixR, n, dt);
        }
        for (int i = 0; i < n; i++) {                                // soft limiter
            out[2 * (done + i)] = std::tanh(mixL[i] * vol * 1.5f) * 0.7f;
            out[2 * (done + i) + 1] = std::tanh(mixR[i] * vol * 1.5f) * 0.7f;
        }
        done += n;
    }
    int voices = 0;
    for (int p = 0; p < 2; p++) for (int i = 1; i < kChannels; i++) voices += parts_[p].v[i].sounding;
    activeVoices_ = voices;
    frameCounter_ += frames;
    publish();

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
    s.noteCenter = noteCenter_.load();
    s.arpSteps = arpSteps_.load();
    return s;
}

void MidiInput::feed(const uint8_t* bytes, int n, int64_t timeNs) {
    parser_.feed(bytes, n, timeNs, [this](const Msg& m) { clean_.process(m); });
}

}  // namespace mpe
