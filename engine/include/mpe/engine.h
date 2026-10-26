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

struct Stats {
    // Time from "bytes arrived" to "the audio block that plays them started", in ms.
    double eventAgeAvgMs = 0, eventAgeMaxMs = 0, eventAgeLastMs = 0;
    // Time spent in render() as a fraction of the audio it produced. Over 1.0 = dropouts.
    double loadAvg = 0, loadPeak = 0;
    uint64_t events = 0, dropped = 0, panics = 0, blocks = 0;
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
    bool setInstrument(int index);                  // builds off the audio thread, then hands over
    void collectRetired();                          // frees instruments the audio thread has let go of
    void setVolume(float v) { volume_ = v; }
    void setTranspose(int semitones) { transpose_ = semitones; }
    void setBendRange(float semitones) { bendRange_ = semitones; }
    void setVelocityScale(float s) { velocityScale_ = s; }
    // If a note is held and no MIDI arrives for this long, release all (0 = off).
    // Meant for Bluetooth, where a lost link is silent.
    void setWatchdogMs(int ms) { watchdogMs_ = ms; }
    void panic() { panicFlag_ = true; }

    int sampleRate() const { return sr_; }
    int currentInstrument() const { return current_; }
    Stats stats() const;
    int activeVoices() const { return activeVoices_; }

private:
    struct Channel;
    void apply(const Msg& m);

    const int sr_;
    Spsc<Msg> queue_{1024};
    std::unique_ptr<Channel[]> ch_;
    InstrumentSet* cur_ = nullptr;                  // audio thread owns
    std::atomic<InstrumentSet*> pending_{nullptr};
    Spsc<InstrumentSet*> retired_{8};
    std::atomic<int> current_{-1};

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
    std::atomic<uint64_t> ageCount_{0}, events_{0}, dropped_{0}, panics_{0}, blocks_{0};
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
