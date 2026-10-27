#include "Synth.h"
#include <android/log.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <sched.h>
#include <unistd.h>

#define TAG "mpe"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)

// Every core except the slowest cluster, by top clock; empty if all cores are alike.
// The scheduler puts a light real-time thread like ours on a little core, where a burst of
// notes can take most of the callback time (measured on a POCO F2 Pro: 78% peak on a little
// core), so the audio thread pins itself to these.
static cpu_set_t fastCores(int& count) {
    cpu_set_t set;
    CPU_ZERO(&set);
    count = 0;
    long freq[CPU_SETSIZE] = {};
    long lo = 0, hi = 0;
    int n = 0;
    for (; n < 64; n++) {
        char path[96];
        std::snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", n);
        FILE* f = std::fopen(path, "r");
        if (!f) break;
        if (std::fscanf(f, "%ld", &freq[n]) != 1) freq[n] = 0;
        std::fclose(f);
        if (n == 0 || freq[n] < lo) lo = freq[n];
        hi = std::max(hi, freq[n]);
    }
    if (hi == lo) return set;
    for (int i = 0; i < n; i++)
        if (freq[i] > lo) { CPU_SET(i, &set); count++; }
    return set;
}

Synth::~Synth() {
    healthRun_ = false;
    if (health_.joinable()) health_.join();
    stop();
    detachMidi();
}

bool Synth::open() {
    oboe::AudioStreamBuilder b;
    b.setDirection(oboe::Direction::Output)
        ->setPerformanceMode(oboe::PerformanceMode::LowLatency)    // the default mode is a high-latency one
        ->setSharingMode(oboe::SharingMode::Exclusive)             // MMAP: falls back to shared if refused
        ->setUsage(oboe::Usage::Game)                              // keeps the low-latency path, also with Bluetooth audio
        // 16-bit, not float: many vendor HALs only offer their fastest output ("raw", FAST|RAW)
        // for PCM_16. A float stream is then mixed on the primary output instead (measured on a
        // POCO F2 Pro: raw has a 144-frame HAL buffer, primary 192 plus the mixer). The engine
        // still renders float; onAudioReady converts.
        ->setFormat(oboe::AudioFormat::I16)
        ->setFormatConversionAllowed(false)
        ->setChannelCount(oboe::ChannelCount::Stereo)
        // No setSampleRate(): the device's own rate (almost always 48000) avoids a slower path.
        // No setFramesPerDataCallback(): the callback size is the device burst.
        ->setDataCallback(this)
        ->setErrorCallback(this);

    std::shared_ptr<oboe::AudioStream> s;
    oboe::Result r = b.openStream(s);
    if (r != oboe::Result::OK) { LOGW("openStream failed: %s", oboe::convertToText(r)); return false; }

    // One burst to start; tune() adds one only if an underrun happens. On a POCO F2 Pro one
    // burst instead of two took output latency from 31 to 23 ms with no underruns.
    if (fastCoreCount_ < 0) {
        fastCores_ = fastCores(fastCoreCount_);
        LOGI("audio thread will use %d fast cores", fastCoreCount_);
    }
    s->setBufferSizeInFrames(s->getFramesPerBurst());
    tuneFrames_ = 0;
    tuneBaseSet_ = false;
    tuneReset_ = false;
    LOGI("stream: %d Hz, burst %d, buffer %d, sharing %s, %s, mmap %d, format %s",
         s->getSampleRate(), s->getFramesPerBurst(), s->getBufferSizeInFrames(),
         s->getSharingMode() == oboe::SharingMode::Exclusive ? "exclusive" : "shared",
         s->usesAAudio() ? "AAudio" : "OpenSL ES", (int)oboe::OboeExtensions::isMMapUsed(s.get()),
         oboe::convertToText(s->getFormat()));
    // Float scratch for a 16-bit stream, sized once here: the callback never allocates.
    floatOut_ = s->getFormat() == oboe::AudioFormat::Float;
    scratch_.assign(size_t(std::max(s->getBufferCapacityInFrames(), s->getFramesPerBurst() * 4)) * 2, 0.f);

    auto eng = std::make_shared<mpe::Engine>(s->getSampleRate());
    eng->setVolume(volume_);
    eng->setTranspose(transpose_);
    eng->setBendRange(bendRange_);
    eng->setWatchdogMs(watchdogMs_);
    eng->setInstrument(0, instrument_[0]);
    if (instrument_[1] >= 0) eng->setInstrument(1, instrument_[1]);
    {
        std::lock_guard<std::mutex> g(mu_);
        eng->setPerformance(perf_);
        stream_ = s;
        engine_ = eng;
    }
    audioEngine_ = eng.get();
    midi_ = std::make_unique<mpe::MidiInput>(*eng);
    return true;
}

bool Synth::start() {
    if (running_) return true;
    if (!open()) return false;
    oboe::Result r;
    {
        std::lock_guard<std::mutex> g(mu_);
        r = stream_ ? stream_->requestStart() : oboe::Result::ErrorClosed;
    }
    if (r != oboe::Result::OK) { LOGW("requestStart failed: %s", oboe::convertToText(r)); return false; }
    running_ = true;
    if (!healthRun_.exchange(true)) health_ = std::thread(&Synth::healthLoop, this);
    return true;
}

// Close and reopen the stream, keeping the settings and the MIDI port.
void Synth::restartStream(const char* why) {
    std::lock_guard<std::mutex> r(restartMu_);
    LOGW("restarting audio stream: %s", why);
    stop();
    start();
    restarts_++;
}

// A stream can keep calling back yet be broken: after the audio server restarts, the legacy
// (non-MMAP) path reconnects by itself but its timestamps are left seconds off, which also
// makes the latency readout wrong. If the stream reports an impossible latency twice in a
// row, reopen it.
void Synth::healthLoop() {
    int bad = 0;
    bool pinLogged = false;
    while (healthRun_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (pinned_ != pinLogged) { pinLogged = pinned_; LOGI("audio thread %s the fast cores", pinLogged ? "pinned to" : "not pinned to"); }
        if (!running_) { bad = 0; continue; }
        bool impossible = false;
        {
            std::lock_guard<std::mutex> g(mu_);
            if (!stream_) continue;
            auto lat = stream_->calculateLatencyMillis();
            impossible = lat && lat.value() > 1000.0;
        }
        bad = impossible ? bad + 1 : 0;
        if (bad >= 2) { restartStream("output latency is impossible (timestamps broken)"); bad = 0; }
    }
}

void Synth::stop() {
    running_ = false;
    // Close under mu_: other threads only touch the stream while holding mu_, so nobody can be
    // inside getTimestamp() or getXRunCount() on a stream that is being closed (that crashes).
    std::lock_guard<std::mutex> g(mu_);
    if (stream_) { stream_->requestStop(); stream_->close(); }
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
    std::lock_guard<std::mutex> r(restartMu_);
    start();
    restarts_++;
}

oboe::DataCallbackResult Synth::onAudioReady(oboe::AudioStream* stream, void* data, int32_t frames) {
    const int64_t now = mpe::steadyNowNs();
    // Once per audio thread (a reopened stream gets a new one): move it to the fast cores.
    if (fastCoreCount_ > 0 && gettid() != pinnedTid_) {
        pinnedTid_ = gettid();
        pinned_ = sched_setaffinity(0, sizeof(fastCores_), &fastCores_) == 0;
    }
    if (lost_.exchange(false)) midi_->deviceLost();
    pollMidi(now);
    // Render float, in pieces that fit the scratch buffer, converting when the stream is 16-bit.
    const int32_t chunk = floatOut_ ? frames : int32_t(scratch_.size() / 2);
    for (int32_t done = 0; done < frames;) {
        const int32_t n = std::min(chunk, frames - done);
        float* f = floatOut_ ? static_cast<float*>(data) + 2 * done : scratch_.data();
        audioEngine_->render(f, n, now);
        if (!floatOut_) {
            int16_t* o = static_cast<int16_t*>(data) + 2 * done;
            for (int32_t i = 0; i < 2 * n; i++)
                o[i] = int16_t(std::max(-1.f, std::min(1.f, f[i])) * 32767.f);
        }
        if (capArmed_.load(std::memory_order_acquire)) {
            const int64_t pos = capPos_.load(std::memory_order_relaxed), len = capLen_.load(std::memory_order_relaxed);
            const int64_t c = std::min<int64_t>(n, len - pos);
            std::memcpy(cap_.data() + 2 * pos, f, sizeof(float) * 2 * c);
            capPos_.store(pos + c, std::memory_order_release);
            if (pos + c >= len) capArmed_.store(false, std::memory_order_release);
        }
        done += n;
    }
    tune(stream, frames);
    (void)stream;
    return oboe::DataCallbackResult::Continue;
}

// Our own latency tuner, instead of oboe::LatencyTuner: that one also counts the underrun many
// devices report while a stream starts (seen on a POCO F2 Pro), and then keeps two bursts for
// good. Here underruns only count once the stream has played for half a second; each one
// after that adds a burst.
void Synth::tune(oboe::AudioStream* stream, int32_t frames) {
    if (tuneReset_.exchange(false)) {                     // the "Reset latency tuner" button
        stream->setBufferSizeInFrames(stream->getFramesPerBurst());
        tuneBaseSet_ = false;                             // count from the current xruns on
    }
    if (tuneFrames_ < stream->getSampleRate() / 2) { tuneFrames_ += frames; return; }
    auto x = stream->getXRunCount();
    if (!x) return;
    if (!tuneBaseSet_) { tuneXruns_ = x.value(); tuneBaseSet_ = true; return; }
    if (x.value() > tuneXruns_) {
        tuneXruns_ = x.value();
        stream->setBufferSizeInFrames(stream->getBufferSizeInFrames() + stream->getFramesPerBurst());
    }
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

void Synth::setInstrument(int part, int index) {
    if (part < 0 || part > 1) return;
    instrument_[part] = index;
    if (auto e = engine()) { e->setInstrument(part, index); }
}
void Synth::setPerformance(const mpe::Performance& p) {
    std::lock_guard<std::mutex> g(mu_);
    perf_ = p;
    if (engine_) engine_->setPerformance(p);
}
int Synth::voices(mpe::VoiceView* out, int max) { auto e = engine(); return e ? e->voices(out, max) : 0; }
int Synth::fingers(mpe::FingerView* out, int max) { auto e = engine(); return e ? e->fingers(out, max) : 0; }
void Synth::setVolume(float v) { volume_ = v; if (auto e = engine()) e->setVolume(v); }
void Synth::setTranspose(int s) { transpose_ = s; if (auto e = engine()) e->setTranspose(s); }
void Synth::setBendRange(float s) { bendRange_ = s; if (auto e = engine()) e->setBendRange(s); }
void Synth::setWatchdogMs(int ms) { watchdogMs_ = ms; if (auto e = engine()) e->setWatchdogMs(ms); }
void Synth::panic() { if (auto e = engine()) e->panic(); }

// Order must match Stats.kt.
void Synth::fillStats(double o[kStatCount]) {
    std::memset(o, 0, sizeof(double) * kStatCount);
    std::lock_guard<std::mutex> g(mu_);              // the stream cannot be closed while we read it
    if (engine_) {
        mpe::Stats st = engine_->stats();
        o[0] = st.eventAgeAvgMs; o[1] = st.eventAgeMaxMs; o[2] = st.eventAgeLastMs;
        o[3] = st.loadAvg; o[4] = st.loadPeak;
        o[5] = engine_->activeVoices();
        o[6] = (double)st.events; o[7] = (double)st.dropped; o[8] = (double)st.panics;
        o[9] = engine_->sampleRate();
        o[19] = st.noteCenter;
        o[20] = (double)st.arpSteps;
    }
    if (stream_) {
        oboe::AudioStream* s = stream_.get();
        auto lat = s->calculateLatencyMillis();               // buffer + hardware, from the stream's own timestamps
        o[10] = lat ? lat.value() : -1;
        o[11] = s->getBufferSizeInFrames();
        o[12] = s->getFramesPerBurst();
        auto x = s->getXRunCount();
        o[13] = x ? x.value() : -1;
        o[14] = s->getSharingMode() == oboe::SharingMode::Exclusive ? 1 : 0;
        o[15] = s->usesAAudio() ? (oboe::OboeExtensions::isMMapUsed(s) ? 2 : 1) : 0;   // 0 OpenSL, 1 AAudio, 2 AAudio+MMAP
    }
    o[16] = (double)midiBytes_.load();
    o[17] = port_.load() ? (portBroken_.load() ? -1 : 1) : 0;
    o[18] = restarts_.load();
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
