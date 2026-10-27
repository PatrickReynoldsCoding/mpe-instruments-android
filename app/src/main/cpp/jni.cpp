// JNI glue: one Synth for the whole process, called from Kotlin (Native.kt).
#include <jni.h>
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

FN(jint, nativeInstrumentCount)(JNIEnv*, jobject) { int n = 0; mpe::instruments(n); return n; }
FN(jstring, nativeInstrumentField)(JNIEnv* env, jobject, jint index, jint field) {
    int n = 0;
    const mpe::InstrumentInfo* all = mpe::instruments(n);
    if (index < 0 || index >= n) return env->NewStringUTF("");
    const mpe::InstrumentInfo& i = all[index];
    const char* v = field == 0 ? i.id : field == 1 ? i.group : field == 2 ? i.name : i.description;
    return env->NewStringUTF(v ? v : "");
}

FN(void, nativeSetInstrument)(JNIEnv*, jobject, jint i) { synth().setInstrument(i); }
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
