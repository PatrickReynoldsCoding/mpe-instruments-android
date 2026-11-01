#include "Mic.h"
#include <android/log.h>
#include <algorithm>

#define TAG "mpe"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)

// Unprocessed first: the usual voice processing (noise suppression) is made to remove exactly
// the rumble of air on the capsule that breath is. Voice recognition (no noise suppression on
// most phones) if the phone does not offer it.
bool Mic::open() {
    const oboe::InputPreset presets[] = {oboe::InputPreset::Unprocessed, oboe::InputPreset::VoiceRecognition};
    for (oboe::InputPreset preset : presets) {
        oboe::AudioStreamBuilder b;
        b.setDirection(oboe::Direction::Input)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Shared)
            ->setFormat(oboe::AudioFormat::Float)
            ->setFormatConversionAllowed(true)
            ->setChannelCount(oboe::ChannelCount::Mono)
            ->setChannelConversionAllowed(true)
            ->setInputPreset(preset)
            ->setDataCallback(this)
            ->setErrorCallback(this);
        std::shared_ptr<oboe::AudioStream> s;
        oboe::Result r = b.openStream(s);
        if (r != oboe::Result::OK) { LOGW("mic: open (preset %d) failed: %s", int(preset), oboe::convertToText(r)); continue; }
        if (!breath_ || s->getSampleRate() != lastRate_) {
            breath_ = std::make_unique<mpe::Breath>(s->getSampleRate());
            lastRate_ = s->getSampleRate();
        } else {
            breath_->reset();
        }
        r = s->requestStart();
        if (r != oboe::Result::OK) { LOGW("mic: start failed: %s", oboe::convertToText(r)); s->close(); continue; }
        LOGI("mic: %d Hz, %d ch, burst %d, preset %d, %s", s->getSampleRate(), s->getChannelCount(), s->getFramesPerBurst(),
             int(preset), s->getSharingMode() == oboe::SharingMode::Exclusive ? "exclusive" : "shared");
        stream_ = s;
        return true;
    }
    return false;
}

bool Mic::start() {
    std::lock_guard<std::mutex> g(mu_);
    wanted_ = true;
    if (stream_) return true;
    const bool ok = open();
    state_ = ok ? 1 : -1;
    return ok;
}

void Mic::stop() {
    std::lock_guard<std::mutex> g(mu_);
    wanted_ = false;
    if (stream_) { stream_->requestStop(); stream_->close(); stream_.reset(); }
    state_ = 0;
    level_ = 0;
    db_ = -120;
}

oboe::DataCallbackResult Mic::onAudioReady(oboe::AudioStream* s, void* data, int32_t frames) {
    mpe::Breath& b = *breath_;
    b.setSensitivity(sensitivity_.load(std::memory_order_relaxed));
    b.setEcho(echo_.load(std::memory_order_relaxed));
    const float* x = static_cast<const float*>(data);
    const int ch = s->getChannelCount();
    if (ch == 1) {
        b.process(x, frames);
    } else {
        for (int32_t done = 0; done < frames;) {
            const int n = std::min<int32_t>(1024, frames - done);
            for (int i = 0; i < n; i++) { float v = 0; for (int c = 0; c < ch; c++) v += x[(done + i) * ch + c]; mono_[i] = v / ch; }
            b.process(mono_, n);
            done += n;
        }
    }
    const mpe::Breath::Meter m = b.meter();
    level_.store(m.level, std::memory_order_relaxed);
    db_.store(m.db, std::memory_order_relaxed);
    floor_.store(m.floorDb, std::memory_order_relaxed);
    on_.store(m.onDb, std::memory_order_relaxed);
    full_.store(m.fullDb, std::memory_order_relaxed);
    echoShown_.store(m.echoDb, std::memory_order_relaxed);
    return oboe::DataCallbackResult::Continue;
}

// The microphone went away (a headset unplugged, another app took it): open it again.
void Mic::onErrorAfterClose(oboe::AudioStream*, oboe::Result error) {
    LOGW("mic: stream closed with error: %s", oboe::convertToText(error));
    std::lock_guard<std::mutex> g(mu_);
    stream_.reset();
    level_ = 0;
    if (!wanted_) return;
    state_ = open() ? 1 : -1;
}

mpe::Breath::Meter Mic::meter() const {
    return {db_.load(std::memory_order_relaxed), floor_.load(std::memory_order_relaxed), on_.load(std::memory_order_relaxed),
            full_.load(std::memory_order_relaxed), level_.load(std::memory_order_relaxed), echoShown_.load(std::memory_order_relaxed)};
}
