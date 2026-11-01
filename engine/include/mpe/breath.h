// Breath from a microphone: how hard someone is blowing on it, 0..1. No Android here.
//
// Blowing on a phone's microphone makes a loud, low rumble (the air hitting the capsule). The
// phone's own speaker makes almost nothing that low, and a room is quiet there. So the detector
// keeps only what is below 150 Hz, follows its level, and compares it with the room's noise
// floor: the quietest 50 ms of the last 10 seconds. A steady fan is learnt that way within about
// 10 s, while playing is not, because a player stops to breathe.
//
// The speaker: what the app plays can reach the microphone, and with Swell a note's own sound
// could then keep it playing. So the engine reports how loud its own output is down there
// (Engine::outputLowDb), and breath has to be louder than that, less the coupling, to count.
//
// Threads: process() and reset() on the microphone's thread; the rest from any thread.
#pragma once
#include <atomic>

namespace mpe {

class Breath {
public:
    explicit Breath(int sampleRate);

    void process(const float* mono, int n);        // microphone samples, -1..1
    void reset();                                  // forget everything (a new stream)

    // 0 = needs a hard blow .. 1 = a soft breath. Default 0.5.
    void setSensitivity(float s) { sensitivity_ = s; }
    // The app's own output below 150 Hz, in dBFS (Engine::outputLowDb), and how much quieter it
    // can be at the microphone, at most (speaker to microphone, full volume).
    void setEcho(float db) { echoDb_ = db; }
    void setCoupling(float db) { couplingDb_ = db; }
    float level() const { return level_.load(std::memory_order_relaxed); }

    // For the meter on the screen, all in dB (full scale = 0).
    struct Meter { float db, floorDb, onDb, fullDb, level, echoDb; };
    Meter meter() const;

private:
    struct Biquad { float b0, b1, b2, a1, a2, z1 = 0, z2 = 0; float run(float x); };
    void control();
    void window(float meanPower);

    int sr_;
    Biquad lp_[2];
    float dcX_ = 0, dcY_ = 0, dcA_;
    float env_ = 0, attack_, release_;             // fast power envelope: what the breath level reads
    float lv_ = 0, lvUp_, lvDown_;                 // the level, smoothed once more
    int count_ = 0, controlEvery_;
    double winSum_ = 0;                            // 50 ms windows of mean power: what the floor reads
    int winCount_ = 0, winLen_;
    static constexpr int kBlocks = 20, kWindowsPerBlock = 10;
    float blocks_[kBlocks];                        // the quietest window of each of the last 20 half seconds
    int blockPos_ = 0, blockWins_ = 0;
    float blockMin_ = 1e9f;
    float floorDb_ = -80.f, floorDown_, floorUp_;
    bool fresh_ = true;
    std::atomic<float> sensitivity_{0.5f}, echoDb_{-120.f}, couplingDb_{-24.f};
    std::atomic<float> level_{0}, db_{-120}, pubFloor_{-80}, pubOn_{-65}, pubFull_{-35}, pubEcho_{-120};
};

}  // namespace mpe
