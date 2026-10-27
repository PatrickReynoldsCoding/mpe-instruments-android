// The Android side of the engine: an Oboe low-latency stream, the Engine, and an AMidi
// port that is polled INSIDE the audio callback.
//
// Why poll inside the callback: AMidiOutputPort_receive() never blocks, so the audio
// thread can read the controller itself. That removes a thread hop and any Java/JNI
// step per message, so a message waits at most until the next callback (one burst).
#pragma once
#include <jni.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <sched.h>
#include <amidi/AMidi.h>
#include <oboe/Oboe.h>
#include "mpe/engine.h"

class Synth : public oboe::AudioStreamDataCallback, public oboe::AudioStreamErrorCallback {
public:
    Synth() = default;
    ~Synth();

    bool start();                                   // open the stream and play
    void stop();
    bool running() const { return running_; }

    // MIDI. `device` is the Java android.media.midi.MidiDevice; `port` is the index
    // of one of its OUTPUT ports (the controller sends on these).
    bool attachMidi(JNIEnv* env, jobject device, int port);
    void detachMidi();
    bool midiAttached() const { return port_.load() != nullptr; }

    // Settings: remembered here so they survive the engine being rebuilt (sample rate change).
    void setInstrument(int index);
    void setVolume(float v);
    void setTranspose(int semitones);
    void setBendRange(float semitones);
    void setWatchdogMs(int ms);
    void panic();
    void resetLatencyTuner() { tuneReset_ = true; }

    // Everything the status line shows. Order is fixed: see Stats.kt.
    static constexpr int kStatCount = 19;
    void fillStats(double out[kStatCount]);

    // Debug: record what the audio callback produced, so tests can check the real output.
    bool captureStart(double seconds);              // control thread
    double captureProgress() const;                 // 0..1
    bool captureSave(const char* wavPath);          // after progress reaches 1

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* stream, void* data, int32_t frames) override;
    void onErrorAfterClose(oboe::AudioStream* stream, oboe::Result error) override;

private:
    bool open();
    void restartStream(const char* why);
    void healthLoop();
    void pollMidi(int64_t nowNs);
    void tune(oboe::AudioStream* stream, int32_t frames);
    std::shared_ptr<mpe::Engine> engine() { std::lock_guard<std::mutex> g(mu_); return engine_; }

    std::mutex mu_;                                 // guards stream_ / engine_ pointers for non-audio threads
    std::shared_ptr<oboe::AudioStream> stream_;
    std::shared_ptr<mpe::Engine> engine_;
    mpe::Engine* audioEngine_ = nullptr;            // what the audio thread uses; changed only while the stream is closed
    std::unique_ptr<mpe::MidiInput> midi_;          // used only on the audio thread
    // latency tuner state (see tune()); audio thread only, except the reset request
    int64_t tuneFrames_ = 0;                        // frames played since the stream opened, up to half a second
    int32_t tuneXruns_ = 0;
    bool tuneBaseSet_ = false;
    std::atomic<bool> tuneReset_{false};
    bool floatOut_ = true;                          // stream format is float, else 16-bit
    std::vector<float> scratch_;                    // float render buffer for a 16-bit stream
    cpu_set_t fastCores_{};                         // where the audio thread pins itself
    int fastCoreCount_ = -1;                        // -1 = not looked up yet, 0 = all cores alike
    pid_t pinnedTid_ = 0;                           // audio thread only
    std::atomic<bool> pinned_{false};
    std::atomic<bool> running_{false};
    std::mutex restartMu_;                          // one stream restart at a time
    std::thread health_;
    std::atomic<bool> healthRun_{false};
    std::atomic<int> restarts_{0};

    // MIDI port, shared between the control thread (attach/detach) and the audio thread (poll).
    AMidiDevice* device_ = nullptr;
    std::atomic<AMidiOutputPort*> port_{nullptr};
    std::atomic<bool> polling_{false};
    std::atomic<bool> lost_{false};                 // tell the audio thread to release every note
    std::atomic<bool> portBroken_{false};
    std::atomic<uint64_t> midiBytes_{0}, midiErrors_{0};

    // debug capture: written by the audio thread only while armed
    std::vector<float> cap_;
    std::atomic<int64_t> capPos_{0}, capLen_{0};
    std::atomic<bool> capArmed_{false};
    int capRate_ = 48000;

    // settings
    std::atomic<int> instrument_{0}, transpose_{0}, watchdogMs_{0};
    std::atomic<float> volume_{0.8f}, bendRange_{48.f};
};
