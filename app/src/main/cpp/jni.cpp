// JNI glue: one Synth for the whole process, called from Kotlin (Native.kt).
#include <jni.h>
#include <algorithm>
#include <memory>
#include "Synth.h"

static Synth& synth() { static Synth s; return s; }

#define FN(ret, name) extern "C" JNIEXPORT ret JNICALL Java_com_patrickreynolds_mpeinstruments_Native_##name

FN(jboolean, nativeStart)(JNIEnv*, jobject) { return synth().start(); }
FN(void, nativeStop)(JNIEnv*, jobject) { synth().stop(); }

FN(jboolean, nativeAttachMidi)(JNIEnv* env, jobject, jobject device, jint port) {
    return synth().attachMidi(env, device, port);
}
FN(void, nativeDetachMidi)(JNIEnv*, jobject) { synth().detachMidi(); }
FN(void, nativeTouch)(JNIEnv*, jobject, jint status, jint d1, jint d2) {
    synth().touchMidi(uint8_t(status), uint8_t(d1), uint8_t(d2));
}

FN(jint, nativeInstrumentCount)(JNIEnv*, jobject) { int n = 0; mpe::instruments(n); return n; }
FN(jstring, nativeInstrumentField)(JNIEnv* env, jobject, jint index, jint field) {
    int n = 0;
    const mpe::InstrumentInfo* all = mpe::instruments(n);
    if (index < 0 || index >= n) return env->NewStringUTF("");
    const mpe::InstrumentInfo& i = all[index];
    const char* v = field == 0 ? i.id : field == 1 ? i.group : field == 2 ? i.name : field == 3 ? i.description
                  : i.meta("range");                 // field 4: "low high" in Hz, or ""
    return env->NewStringUTF(v ? v : "");
}

FN(void, nativeSetInstrument)(JNIEnv*, jobject, jint i) { synth().setInstrument(i); }
FN(void, nativeSetPartInstrument)(JNIEnv*, jobject, jint part, jint i) { synth().setInstrument(part, i); }

// The whole Performance as one float array. Layout must match Perf.toArray() in Perf.kt.
FN(void, nativeSetPerformance)(JNIEnv* env, jobject, jfloatArray arr) {
    constexpr int kHead = 11, kPart = 13;
    if (env->GetArrayLength(arr) < kHead + 2 * kPart) return;
    float a[kHead + 2 * kPart];
    env->GetFloatArrayRegion(arr, 0, kHead + 2 * kPart, a);
    mpe::Performance p;
    p.layout = mpe::Layout(int(a[0]));
    p.splitNote = int(a[1]);
    p.bpm = a[2];
    p.scaleMask = uint16_t(int(a[3]));
    p.scaleRoot = int(a[4]);
    p.assist = a[5];
    p.reverbSize = a[6];
    p.reverbDamp = a[7];
    p.delayBeats = a[8];
    p.delayFeedback = a[9];
    p.delayPingPong = a[10] > 0.5f;
    for (int k = 0; k < 2; k++) {
        const float* b = a + kHead + k * kPart;
        mpe::PartConfig& c = p.part[k];
        c.arp = b[0] > 0.5f;
        c.order = mpe::ArpOrder(int(b[1]));
        c.rate = b[2];
        c.gate = b[3];
        c.octaves = int(b[4]);
        c.latch = b[5] > 0.5f;
        c.transpose = int(b[6]);
        c.volume = b[7];
        c.brightness = b[8];
        c.drive = b[9];
        c.pressureCurve = b[10];
        c.reverb = b[11];
        c.delay = b[12];
    }
    synth().setPerformance(p);
}

// Sounding voices into `out`, 8 floats each: part, finger, note, gate, pitch, pressure, slide, level.
FN(jint, nativeVoices)(JNIEnv* env, jobject, jfloatArray out) {
    mpe::VoiceView v[32];
    const int n = std::min(synth().voices(v, 32), int(env->GetArrayLength(out) / 8));
    float a[32 * 8];
    for (int i = 0; i < n; i++) {
        float* o = a + 8 * i;
        o[0] = v[i].part; o[1] = v[i].finger; o[2] = v[i].note; o[3] = v[i].gate;
        o[4] = v[i].pitch; o[5] = v[i].pressure; o[6] = v[i].slide; o[7] = v[i].level;
    }
    if (n) env->SetFloatArrayRegion(out, 0, 8 * n, a);
    return n;
}
// Held fingers into `out`, 5 floats each: channel, note, pitch, pressure, slide.
FN(jint, nativeFingers)(JNIEnv* env, jobject, jfloatArray out) {
    mpe::FingerView f[16];
    const int n = std::min(synth().fingers(f, 16), int(env->GetArrayLength(out) / 5));
    float a[16 * 5];
    for (int i = 0; i < n; i++) {
        float* o = a + 5 * i;
        o[0] = f[i].channel; o[1] = f[i].note; o[2] = f[i].pitch; o[3] = f[i].pressure; o[4] = f[i].slide;
    }
    if (n) env->SetFloatArrayRegion(out, 0, 5 * n, a);
    return n;
}
FN(void, nativeSetVolume)(JNIEnv*, jobject, jfloat v) { synth().setVolume(v); }
FN(void, nativeSetTranspose)(JNIEnv*, jobject, jint s) { synth().setTranspose(s); }
FN(void, nativeSetBendRange)(JNIEnv*, jobject, jfloat s) { synth().setBendRange(s); }
FN(void, nativeSetWatchdogMs)(JNIEnv*, jobject, jint ms) { synth().setWatchdogMs(ms); }
FN(void, nativePanic)(JNIEnv*, jobject) { synth().panic(); }
FN(void, nativeResetLatencyTuner)(JNIEnv*, jobject) { synth().resetLatencyTuner(); }

FN(jdoubleArray, nativeStats)(JNIEnv* env, jobject) {
    double v[Synth::kStatCount];
    synth().fillStats(v);
    jdoubleArray arr = env->NewDoubleArray(Synth::kStatCount);
    env->SetDoubleArrayRegion(arr, 0, Synth::kStatCount, v);
    return arr;
}

FN(jboolean, nativeCaptureStart)(JNIEnv*, jobject, jfloat seconds) { return synth().captureStart(seconds); }
FN(jdouble, nativeCaptureProgress)(JNIEnv*, jobject) { return synth().captureProgress(); }
FN(jboolean, nativeCaptureSave)(JNIEnv* env, jobject, jstring path) {
    const char* p = env->GetStringUTFChars(path, nullptr);
    bool ok = synth().captureSave(p);
    env->ReleaseStringUTFChars(path, p);
    return ok;
}
