// The microphone, for breath: an Oboe input stream on its own callback thread, separate from
// the output so a microphone problem can never touch the sound. Only a level leaves it (see
// mpe::Breath); no sound is kept or stored.
#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <oboe/Oboe.h>
#include "mpe/breath.h"

class Mic : public oboe::AudioStreamDataCallback, public oboe::AudioStreamErrorCallback {
public:
    ~Mic() { stop(); }

    bool start();                                   // control thread; false: no permission or no microphone
    void stop();
    // 0 off, 1 listening, -1 could not open the microphone
    int state() const { return state_.load(); }

    void setSensitivity(float s) { sensitivity_ = s; }
    void setEcho(float db) { echo_.store(db, std::memory_order_relaxed); }      // the output's own low end (audio thread)

    float level() const { return level_.load(std::memory_order_relaxed); }
    mpe::Breath::Meter meter() const;

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* stream, void* data, int32_t frames) override;
    void onErrorAfterClose(oboe::AudioStream* stream, oboe::Result error) override;

private:
    bool open();

    std::mutex mu_;
    std::shared_ptr<oboe::AudioStream> stream_;
    std::unique_ptr<mpe::Breath> breath_;           // replaced only while the stream is closed
    int lastRate_ = 0;
    std::atomic<bool> wanted_{false};
    std::atomic<int> state_{0};
    std::atomic<float> sensitivity_{0.5f}, echo_{-120.f};
    // published by the microphone thread
    std::atomic<float> level_{0}, db_{-120}, floor_{-80}, on_{-65}, full_{-35}, echoShown_{-120};
    float mono_[1024];                              // a stereo-only microphone, mixed down
};
