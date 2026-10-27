#include "Synth.h"
#include <android/log.h>
#include <cstdio>
#include <cstring>
#include <algorithm>

#define TAG "mpe"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)

Synth::~Synth() { stop(); detachMidi(); }

bool Synth::open() {
    oboe::AudioStreamBuilder b;
    b.setDirection(oboe::Direction::Output)
        ->setPerformanceMode(oboe::PerformanceMode::LowLatency)    // the default mode is a high-latency one
        ->setSharingMode(oboe::SharingMode::Exclusive)             // MMAP: falls back to shared if refused
        ->setUsage(oboe::Usage::Game)                              // keeps the low-latency path, also with Bluetooth audio
        ->setFormat(oboe::AudioFormat::Float)
        ->setChannelCount(oboe::ChannelCount::Stereo)
        // No setSampleRate(): the device's own rate (almost always 48000) avoids a slower path.
        // No setFramesPerDataCallback(): the callback size is the device burst.
        ->setDataCallback(this)
        ->setErrorCallback(this);

    std::shared_ptr<oboe::AudioStream> s;
    oboe::Result r = b.openStream(s);
    if (r != oboe::Result::OK) { LOGW("openStream failed: %s", oboe::convertToText(r)); return false; }

    // Two bursts to start; the tuner raises it only if an underrun happens.
    s->setBufferSizeInFrames(s->getFramesPerBurst() * 2);
    LOGI("stream: %d Hz, burst %d, buffer %d, sharing %s, %s, mmap %d",
         s->getSampleRate(), s->getFramesPerBurst(), s->getBufferSizeInFrames(),
         s->getSharingMode() == oboe::SharingMode::Exclusive ? "exclusive" : "shared",
         s->usesAAudio() ? "AAudio" : "OpenSL ES", (int)oboe::OboeExtensions::isMMapUsed(s.get()));

    auto eng = std::make_shared<mpe::Engine>(s->getSampleRate());
    eng->setVolume(volume_);
    eng->setTranspose(transpose_);
    eng->setBendRange(bendRange_);
    eng->setWatchdogMs(watchdogMs_);
    eng->setInstrument(instrument_);
    {
        std::lock_guard<std::mutex> g(mu_);
        stream_ = s;
        engine_ = eng;
    }
    audioEngine_ = eng.get();
    midi_ = std::make_unique<mpe::MidiInput>(*eng);
    tuner_ = std::make_unique<oboe::LatencyTuner>(*s);
    return true;
}

bool Synth::start() {
    if (running_) return true;
    if (!open()) return false;
    oboe::Result r = stream_->requestStart();
    if (r != oboe::Result::OK) { LOGW("requestStart failed: %s", oboe::convertToText(r)); return false; }
    running_ = true;
    return true;
}

void Synth::stop() {
    running_ = false;
    std::shared_ptr<oboe::AudioStream> s;
    {
        std::lock_guard<std::mutex> g(mu_);
        s = stream_;
    }
    if (s) { s->requestStop(); s->close(); }
    std::lock_guard<std::mutex> g(mu_);
    stream_.reset();
    audioEngine_ = nullptr;
}

// Headphones unplugged, the audio route changed, the stream died: open a new one.
void Synth::onErrorAfterClose(oboe::AudioStream*, oboe::Result error) {
    LOGW("stream closed with error: %s", oboe::convertToText(error));
    if (!running_) return;
    {
        std::lock_guard<std::mutex> g(mu_);
        stream_.reset();
        audioEngine_ = nullptr;
    }
    running_ = false;
    start();
}

oboe::DataCallbackResult Synth::onAudioReady(oboe::AudioStream* stream, void* data, int32_t frames) {
    const int64_t now = mpe::steadyNowNs();
    if (lost_.exchange(false)) midi_->deviceLost();
    pollMidi(now);
    audioEngine_->render(static_cast<float*>(data), frames, now);
    if (capArmed_.load(std::memory_order_acquire)) {
        const int64_t pos = capPos_.load(std::memory_order_relaxed), len = capLen_.load(std::memory_order_relaxed);
        const int64_t n = std::min<int64_t>(frames, len - pos);
        std::memcpy(cap_.data() + 2 * pos, data, sizeof(float) * 2 * n);
        capPos_.store(pos + n, std::memory_order_release);
        if (pos + n >= len) capArmed_.store(false, std::memory_order_release);
    }
    if (tuner_) tuner_->tune();
    (void)stream;
    return oboe::DataCallbackResult::Continue;
}

void Synth::pollMidi(int64_t nowNs) {
    // Dekker-style guard: detachMidi() swaps port_ to null and then waits for polling_ to clear,
    // so the port is never closed while this function is inside AMidiOutputPort_receive().
    polling_.store(true, std::memory_order_seq_cst);
    AMidiOutputPort* p = port_.load(std::memory_order_seq_cst);
    if (p && !portBroken_.load()) {
        uint8_t buf[256];
        for (int i = 0; i < 64; i++) {                     // bounded: never starve the audio
            int32_t opcode = 0;
            size_t nbytes = 0;
            int64_t ts = 0;
            ssize_t n = AMidiOutputPort_receive(p, &opcode, buf, sizeof(buf), &nbytes, &ts);
            if (n < 0) { midiErrors_++; portBroken_ = true; lost_ = true; break; }   // device gone
            if (n == 0) break;                               // nothing more waiting
            if (opcode != AMIDI_OPCODE_DATA || nbytes == 0) continue;
            // `ts` is when the framework got the bytes (CLOCK_MONOTONIC, same clock as steadyNowNs).
            // Trust it only if it is plausible; otherwise count the message as arriving now.
            int64_t t = (ts > 0 && ts <= nowNs && nowNs - ts < 1000000000LL) ? ts : nowNs;
            midiBytes_ += nbytes;
            midi_->feed(buf, (int)nbytes, t);
        }
    }
    polling_.store(false, std::memory_order_release);
}

bool Synth::attachMidi(JNIEnv* env, jobject deviceObj, int portNumber) {
    detachMidi();
    AMidiDevice* dev = nullptr;
    if (AMidiDevice_fromJava(env, deviceObj, &dev) != AMEDIA_OK || !dev) { LOGW("AMidiDevice_fromJava failed"); return false; }
    AMidiOutputPort* p = nullptr;
    if (AMidiOutputPort_open(dev, portNumber, &p) != AMEDIA_OK || !p) {
        LOGW("AMidiOutputPort_open(%d) failed", portNumber);
        AMidiDevice_release(dev);
        return false;
    }
    device_ = dev;
    portBroken_ = false;
    port_.store(p, std::memory_order_seq_cst);
    LOGI("MIDI attached, output port %d", portNumber);
    return true;
}

void Synth::detachMidi() {
    AMidiOutputPort* p = port_.exchange(nullptr, std::memory_order_seq_cst);
    if (!p) return;
    while (polling_.load(std::memory_order_seq_cst)) { /* the audio thread is inside receive(): a few microseconds */ }
    AMidiOutputPort_close(p);
    if (device_) { AMidiDevice_release(device_); device_ = nullptr; }
    lost_ = true;                                           // end any held note
    LOGI("MIDI detached");
}

void Synth::setInstrument(int index) {
    instrument_ = index;
    if (auto e = engine()) { e->setInstrument(index); }
}
void Synth::setVolume(float v) { volume_ = v; if (auto e = engine()) e->setVolume(v); }
void Synth::setTranspose(int s) { transpose_ = s; if (auto e = engine()) e->setTranspose(s); }
void Synth::setBendRange(float s) { bendRange_ = s; if (auto e = engine()) e->setBendRange(s); }
void Synth::setWatchdogMs(int ms) { watchdogMs_ = ms; if (auto e = engine()) e->setWatchdogMs(ms); }
void Synth::panic() { if (auto e = engine()) e->panic(); }

// Order must match Stats.kt.
void Synth::fillStats(double o[kStatCount]) {
    std::memset(o, 0, sizeof(double) * kStatCount);
    std::shared_ptr<oboe::AudioStream> s;
    std::shared_ptr<mpe::Engine> e;
    { std::lock_guard<std::mutex> g(mu_); s = stream_; e = engine_; }
    if (e) {
        mpe::Stats st = e->stats();
        o[0] = st.eventAgeAvgMs; o[1] = st.eventAgeMaxMs; o[2] = st.eventAgeLastMs;
        o[3] = st.loadAvg; o[4] = st.loadPeak;
        o[5] = e->activeVoices();
        o[6] = (double)st.events; o[7] = (double)st.dropped; o[8] = (double)st.panics;
        o[9] = e->sampleRate();
    }
    if (s) {
        auto lat = s->calculateLatencyMillis();               // buffer + hardware, from the stream's own timestamps
        o[10] = lat ? lat.value() : -1;
        o[11] = s->getBufferSizeInFrames();
        o[12] = s->getFramesPerBurst();
        auto x = s->getXRunCount();
        o[13] = x ? x.value() : -1;
        o[14] = s->getSharingMode() == oboe::SharingMode::Exclusive ? 1 : 0;
        o[15] = s->usesAAudio() ? (oboe::OboeExtensions::isMMapUsed(s.get()) ? 2 : 1) : 0;   // 0 OpenSL, 1 AAudio, 2 AAudio+MMAP
    }
    o[16] = (double)midiBytes_.load();
    o[17] = port_.load() ? (portBroken_.load() ? -1 : 1) : 0;
}

bool Synth::captureStart(double seconds) {
    if (capArmed_.load()) return false;
    auto e = engine();
    capRate_ = e ? e->sampleRate() : 48000;
    const int64_t frames = int64_t(seconds * capRate_);
    cap_.assign(size_t(frames) * 2, 0.f);              // the audio thread does not touch cap_ while unarmed
    capLen_ = frames;
    capPos_ = 0;
    capArmed_.store(true, std::memory_order_release);
    LOGI("capture: recording %.1f s", seconds);
    return true;
}

double Synth::captureProgress() const {
    const int64_t len = capLen_.load();
    return len ? double(capPos_.load(std::memory_order_acquire)) / len : 0;
}

// 16-bit stereo WAV.
bool Synth::captureSave(const char* path) {
    if (capArmed_.load() || capLen_.load() == 0) return false;
    FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    const uint32_t frames = uint32_t(capPos_.load()), dataBytes = frames * 4;
    auto u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f); u32(36 + dataBytes); std::fwrite("WAVEfmt ", 1, 8, f);
    u32(16); u16(1); u16(2); u32(capRate_); u32(capRate_ * 4); u16(4); u16(16);
    std::fwrite("data", 1, 4, f); u32(dataBytes);
    for (uint32_t i = 0; i < frames * 2; i++) {
        int16_t v = int16_t(std::max(-1.f, std::min(1.f, cap_[i])) * 32767.f);
        std::fwrite(&v, 2, 1, f);
    }
    std::fclose(f);
    LOGI("capture: saved %u frames to %s", frames, path);
    return true;
}
