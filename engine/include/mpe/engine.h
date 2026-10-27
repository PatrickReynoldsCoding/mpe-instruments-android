// The sound engine: MPE messages in, stereo audio out. No Android and no ALSA here.
//
// Threads:
//   MIDI thread   -> MidiInput::feed()          (parse, clean, queue; wait-free)
//   audio thread  -> Engine::render()           (never allocates, never locks)
//   control/UI    -> setInstrument(), setVolume(), ... collectRetired()
#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>
#include "mpe/clean.h"
#include "mpe/midi.h"
#include "mpe/spsc.h"

class dsp;   // Faust

namespace mpe {

// One compiled-in instrument. The table is generated from instruments/<group>/<name>.dsp.
struct InstrumentInfo {
    const char* id;            // "strings/cello"
    const char* group;         // "strings"
    const char* name;          // "Cello"
    const char* description;
    dsp* (*create)();
    const char* (*meta)(const char* key);   // a `declare <key> "<value>"` from the .dsp, or ""
};
const InstrumentInfo* instruments(int& count);
int findInstrument(const char* id);                 // index, or -1

// How the fingers are turned into notes. Handed to the audio thread whole (setPerformance).
enum class Layout : int { Single = 0, Split = 1, Layer = 2 };
enum class ArpOrder : int { Up = 0, Down = 1, UpDown = 2, AsPlayed = 3, Random = 4, Chord = 5 };

struct PartConfig {
    bool arp = false;          // false: every finger plays its own note, as an MPE controller does
    ArpOrder order = ArpOrder::Up;
    float rate = 4;            // arp steps per beat (4 = sixteenths)
    float gate = 0.6f;         // part of a step the note is held, 0.05..1
    int octaves = 1;           // the held notes, then again 1, 2 or 3 octaves up
    bool latch = false;        // keep arpeggiating the last chord after the hand lifts
    int transpose = 0;         // semitones, on top of the engine's transpose
    float volume = 1;
};

struct Performance {
    Layout layout = Layout::Single;
    int splitNote = 60;        // Split: notes below go to part 0 (left), the rest to part 1
    float bpm = 120;
    PartConfig part[2];
    // Pitch assist: a finger that rests is pulled to the nearest note of the scale.
    uint16_t scaleMask = 0;    // bit n: (root + n) % 12 is in the scale; 0 = off
    int scaleRoot = 0;         // 0 = C
    float assist = 0;          // 0 = off .. 1 = locked to the scale
};

// What is sounding, for the touch view and the tests. Copied out of the audio thread's state
// field by field, so a snapshot can mix two blocks; fine for drawing.
struct VoiceView {
    int part = 0, finger = -1, note = -1;   // finger = MIDI channel it follows, -1 = latched
    bool gate = false;
    float pitch = 0;           // semitones, as played (note + bend + transpose + assist)
    float pressure = 0, slide = 0, level = 0;
};
struct FingerView {
    int channel = 0, note = -1;             // note -1 = not held
    float pitch = 0, pressure = 0, slide = 0;
};

struct Stats {
    // Time from "bytes arrived" to "the audio block that plays them started", in ms.
    double eventAgeAvgMs = 0, eventAgeMaxMs = 0, eventAgeLastMs = 0;
    // Time spent in render() as a fraction of the audio it produced. Over 1.0 = dropouts.
    double loadAvg = 0, loadPeak = 0;
    uint64_t events = 0, dropped = 0, panics = 0, blocks = 0;
    uint64_t voiceResets = 0;     // voices that produced NaN/Inf and were silenced
    double noteCenter = -1;       // average of the last 16 notes played (as sent, no transpose); -1 = none yet
    uint64_t arpSteps = 0;
};

struct InstrumentSet;

class Engine {
public:
    explicit Engine(int sampleRate);
    ~Engine();

    // ---- audio thread ----
    // Writes `frames` interleaved stereo float frames. nowNs is the time the first frame
    // will start playing if known, else the callback time (steady clock).
    void render(float* out, int frames, int64_t nowNs);

    // ---- MIDI thread ----
    bool pushMidi(const Msg& m);                    // false if the queue was full (note-off may be lost)

    // ---- control thread ----
    bool setInstrument(int index) { return setInstrument(0, index); }
    bool setInstrument(int part, int index);        // builds off the audio thread, then hands over
    void setPerformance(const Performance& p);      // takes effect at the next block
    void collectRetired();                          // frees what the audio thread has let go of
    void setVolume(float v) { volume_ = v; }
    void setTranspose(int semitones) { transpose_ = semitones; }
    void setBendRange(float semitones) { bendRange_ = semitones; }
    void setVelocityScale(float s) { velocityScale_ = s; }
    // If a note is held and no MIDI arrives for this long, release all (0 = off).
    // Meant for Bluetooth, where a lost link is silent.
    void setWatchdogMs(int ms) { watchdogMs_ = ms; }
    void panic() { panicFlag_ = true; }

    int sampleRate() const { return sr_; }
    int currentInstrument(int part = 0) const { return current_[part]; }
    Stats stats() const;
    int activeVoices() const { return activeVoices_; }
    int voices(VoiceView* out, int max) const;      // the sounding voices
    int fingers(FingerView* out, int max) const;    // the held fingers

private:
    struct Channel;
    struct Voice;
    struct Part;
    void apply(const Msg& m);
    void takeUpdates();
    void stepArp(int p, int64_t frame);
    void releasePart(int p);
    int freeVoice(int p, int64_t frame);
    float assisted(float pitch, float& pull, float& prev, float& speed, float dt) const;
    void renderVoices(int p, float* mixL, float* mixR, int n, float dt);
    void publish();

    const int sr_;
    Spsc<Msg> queue_{1024};
    std::unique_ptr<Channel[]> ch_;
    std::unique_ptr<Part[]> parts_;                 // 2; audio thread owns
    std::atomic<InstrumentSet*> pending_[2] = {};
    Spsc<InstrumentSet*> retired_{8};
    std::atomic<int> current_[2] = {{-1}, {-1}};
    Performance perf_;                              // audio thread's copy
    std::atomic<Performance*> pendingPerf_{nullptr};
    Spsc<Performance*> retiredPerf_{8};
    uint32_t pressCounter_ = 0, rng_ = 0x9e3779b9u;
    int recentNotes_[16] = {}, recentCount_ = 0, recentPos_ = 0;

    std::atomic<float> volume_{0.8f};
    std::atomic<int> transpose_{0};
    std::atomic<float> bendRange_{48.f};            // MPE default for member channels
    std::atomic<float> velocityScale_{5.f};         // the Block sends small velocities
    std::atomic<int> watchdogMs_{0};
    std::atomic<bool> panicFlag_{false};
    std::atomic<bool> overflow_{false};
    std::atomic<int> activeVoices_{0};
    int64_t lastEventFrame_ = 0, frameCounter_ = 0;

    // stats, written by the audio thread
    std::atomic<double> ageSum_{0}, ageMax_{0}, ageLast_{0}, loadSum_{0}, loadPeak_{0};
    std::atomic<uint64_t> ageCount_{0}, events_{0}, dropped_{0}, panics_{0}, blocks_{0}, voiceResets_{0}, arpSteps_{0};
    std::atomic<double> noteCenter_{-1};

    // published by the audio thread after each block, for voices() / fingers()
    struct PubVoice { std::atomic<int> finger{-1}, note{-1}; std::atomic<bool> gate{false}, on{false};
                      std::atomic<float> pitch{0}, pressure{0}, slide{0}, level{0}; };
    struct PubFinger { std::atomic<int> note{-1}; std::atomic<float> pitch{0}, pressure{0}, slide{0}; };
    std::unique_ptr<PubVoice[]> pubVoices_;         // 2 * 16
    std::unique_ptr<PubFinger[]> pubFingers_;       // 16
};

// MIDI thread front end: bytes in, cleaned messages queued for the engine.
class MidiInput {
public:
    explicit MidiInput(Engine& e) : engine_(e), clean_(&MidiInput::sinkThunk, this) {}

    // `timeNs` is when the bytes arrived (steady clock). Call from ONE thread.
    void feed(const uint8_t* bytes, int n, int64_t timeNs);
    // The device went away: end every note and centre bend/pressure.
    void deviceLost() { clean_.panic(); }
    const Clean& clean() const { return clean_; }

private:
    static void sinkThunk(void* self, const Msg& m) {
        MidiInput* s = static_cast<MidiInput*>(self);
        if (!s->engine_.pushMidi(m)) s->engine_.panic();     // lost an event: do not risk a stuck note
    }
    Engine& engine_;
    Clean clean_;
    Parser parser_;
};

int64_t steadyNowNs();

}  // namespace mpe
