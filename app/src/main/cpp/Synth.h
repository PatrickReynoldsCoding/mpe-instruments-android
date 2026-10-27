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
#include <vector>
#include <amidi/AMidi.h>
#include <oboe/Oboe.h>
#include <oboe/LatencyTuner.h>
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
    void resetLatencyTuner() { if (tuner_) tuner_->requestReset(); }

    // Everything the status line shows. Order is fixed: see Stats.kt.
    static constexpr int kStatCount = 18;
    void fillStats(double out[kStatCount]);

    // Debug: record what the audio callback produced, so tests can check the real output.
    bool captureStart(double seconds);              // control thread
    double captureProgress() const;                 // 0..1
    bool captureSave(const char* wavPath);          // after progress reaches 1

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* stream, void* data, int32_t frames) override;
    void onErrorAfterClose(oboe::AudioStream* stream, oboe::Result error) override;

private:
    bool open();
    void pollMidi(int64_t nowNs);
    std::shared_ptr<mpe::Engine> engine() { std::lock_guard<std::mutex> g(mu_); return engine_; }

    std::mutex mu_;                                 // guards stream_ / engine_ pointers for non-audio threads
    std::shared_ptr<oboe::AudioStream> stream_;
    std::shared_ptr<mpe::Engine> engine_;
    mpe::Engine* audioEngine_ = nullptr;            // what the audio thread uses; changed only while the stream is closed
    std::unique_ptr<mpe::MidiInput> midi_;          // used only on the audio thread
    std::unique_ptr<oboe::LatencyTuner> tuner_;
    std::atomic<bool> running_{false};

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
