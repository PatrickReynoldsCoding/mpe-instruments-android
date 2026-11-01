#include "mpe/breath.h"
#include <algorithm>
#include <cmath>

namespace mpe {

static const float kCutHz = 150.f;            // keep only the rumble of the air on the capsule
static const float kSilentDb = -110.f;        // a window this quiet is digital silence (a stream starting), not a room
static const float kFloorMaxDb = -45.f;       // no room is louder than this down there: anything above is playing
static const float kFloorBiasDb = 2.f;        // the quietest window is a little under the room's usual level

float Breath::Biquad::run(float x) {
    const float y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    return y;
}

Breath::Breath(int sampleRate) : sr_(sampleRate) {
    // Two 2nd-order low-passes: 24 dB per octave above 150 Hz.
    const float w0 = 2.f * float(M_PI) * kCutHz / sr_, cw = std::cos(w0), alpha = std::sin(w0) / (2.f * 0.7071f);
    const float a0 = 1.f + alpha;
    for (Biquad& b : lp_) {
        b.b0 = (1.f - cw) / 2.f / a0; b.b1 = (1.f - cw) / a0; b.b2 = b.b0;
        b.a1 = -2.f * cw / a0; b.a2 = (1.f - alpha) / a0;
    }
    dcA_ = std::exp(-2.f * float(M_PI) * 20.f / sr_);
    attack_ = 1.f - std::exp(-1.f / (0.004f * sr_));
    release_ = 1.f - std::exp(-1.f / (0.02f * sr_));
    controlEvery_ = std::max(1, sr_ / 1000);                      // 1 ms
    const float tick = float(controlEvery_) / sr_;
    lvUp_ = 1.f - std::exp(-tick / 0.004f);
    lvDown_ = 1.f - std::exp(-tick / 0.04f);
    winLen_ = std::max(1, sr_ / 20);                              // 50 ms
    const float win = float(winLen_) / sr_;
    floorDown_ = 1.f - std::exp(-win / 0.05f);                    // a quieter room is taken at once
    floorUp_ = 1.f - std::exp(-win / 0.8f);                       // a louder one glides in
    reset();
}

void Breath::reset() {
    for (Biquad& b : lp_) b.z1 = b.z2 = 0;
    dcX_ = dcY_ = 0;
    env_ = lv_ = 0;
    count_ = 0;
    winSum_ = 0;
    winCount_ = 0;
    for (float& b : blocks_) b = 1e9f;
    blockPos_ = blockWins_ = 0;
    blockMin_ = 1e9f;
    fresh_ = true;
    level_ = 0;
}

void Breath::process(const float* x, int n) {
    for (int i = 0; i < n; i++) {
        float v = x[i];
        if (!std::isfinite(v)) v = 0;
        dcY_ = v - dcX_ + dcA_ * dcY_;            // DC blocker (20 Hz)
        dcX_ = v;
        const float y = lp_[1].run(lp_[0].run(dcY_));
        const float p = y * y;
        env_ += (p - env_) * (p > env_ ? attack_ : release_);
        winSum_ += p;
        if (++winCount_ >= winLen_) { window(float(winSum_ / winLen_)); winSum_ = 0; winCount_ = 0; }
        if (++count_ >= controlEvery_) { count_ = 0; control(); }
    }
}

// Every 50 ms: the floor, from the quietest window of the last 10 s.
void Breath::window(float meanPower) {
    const float db = 10.f * std::log10(meanPower + 1e-15f);
    if (db > kSilentDb) {
        blockMin_ = std::min(blockMin_, db);
        if (fresh_) { floorDb_ = std::min(db + kFloorBiasDb, kFloorMaxDb); fresh_ = false; }
    }
    if (++blockWins_ >= kWindowsPerBlock) {
        blocks_[blockPos_] = blockMin_;
        blockPos_ = (blockPos_ + 1) % kBlocks;
        blockWins_ = 0;
        blockMin_ = 1e9f;
    }
    float quietest = blockMin_;
    for (float b : blocks_) quietest = std::min(quietest, b);
    if (quietest > 1e8f) return;                                  // only silence so far
    const float target = std::min(std::max(quietest + kFloorBiasDb, -100.f), kFloorMaxDb);
    floorDb_ += (target - floorDb_) * (target < floorDb_ ? floorDown_ : floorUp_);
}

// Every millisecond: the breath.
void Breath::control() {
    const float db = 10.f * std::log10(env_ + 1e-15f);
    const float s = std::min(std::max(sensitivity_.load(std::memory_order_relaxed), 0.f), 1.f);
    // Sensitivity 0.5: breath starts 18 dB over the room (and never below -65 dBFS) and is full 30 dB higher.
    const float margin = 28.f - 20.f * s, lowest = -55.f - 20.f * s, span = 36.f - 12.f * s;
    // Our own sound from the speaker, as loud as it can be at the microphone, does not count.
    const float echo = echoDb_.load(std::memory_order_relaxed) + couplingDb_.load(std::memory_order_relaxed);
    const float on = std::max(std::max(floorDb_ + margin, lowest), echo), full = on + span;
    const float raw = std::min(std::max((db - on) / span, 0.f), 1.f);
    lv_ += (raw - lv_) * (raw > lv_ ? lvUp_ : lvDown_);
    const float out = lv_ < 0.02f ? 0.f : lv_;                  // no flicker just above nothing
    level_.store(out, std::memory_order_relaxed);
    db_.store(db, std::memory_order_relaxed);
    pubFloor_.store(floorDb_, std::memory_order_relaxed);
    pubOn_.store(on, std::memory_order_relaxed);
    pubFull_.store(full, std::memory_order_relaxed);
    pubEcho_.store(echo, std::memory_order_relaxed);
}

Breath::Meter Breath::meter() const {
    return {db_.load(std::memory_order_relaxed), pubFloor_.load(std::memory_order_relaxed),
            pubOn_.load(std::memory_order_relaxed), pubFull_.load(std::memory_order_relaxed),
            level_.load(std::memory_order_relaxed), pubEcho_.load(std::memory_order_relaxed)};
}

}  // namespace mpe
