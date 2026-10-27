// Opens one AAudio output stream with the given settings, plays silence for a few seconds and
// prints what it got. Run on a phone over adb to find which settings reach its fastest output
// without reinstalling the app each time (see tools/aaudio_probe.sh).
//   aaudio_probe <usage> <format: f|i16> <sharing: x|s> <seconds> [content type] [buffer in bursts]
#include <aaudio/AAudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static aaudio_data_callback_result_t cb(AAudioStream* s, void* user, void* data, int32_t frames) {
    (void)user;
    memset(data, 0, (size_t)frames * 2 * (AAudioStream_getFormat(s) == AAUDIO_FORMAT_PCM_FLOAT ? 4 : 2));
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

int main(int argc, char** argv) {
    if (argc < 5) { fprintf(stderr, "usage: %s usage f|i16 x|s seconds [contentType]\n", argv[0]); return 2; }
    AAudioStreamBuilder* b;
    AAudio_createStreamBuilder(&b);
    AAudioStreamBuilder_setPerformanceMode(b, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setSharingMode(b, argv[3][0] == 'x' ? AAUDIO_SHARING_MODE_EXCLUSIVE : AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setUsage(b, atoi(argv[1]));
    if (argc > 5 && atoi(argv[5]) > 0) AAudioStreamBuilder_setContentType(b, atoi(argv[5]));
    AAudioStreamBuilder_setFormat(b, strcmp(argv[2], "f") == 0 ? AAUDIO_FORMAT_PCM_FLOAT : AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setChannelCount(b, 2);
    AAudioStreamBuilder_setDataCallback(b, cb, NULL);
    AAudioStream* s;
    aaudio_result_t r = AAudioStreamBuilder_openStream(b, &s);
    if (r != AAUDIO_OK) { printf("open failed: %s\n", AAudio_convertResultToText(r)); return 1; }
    AAudioStream_setBufferSizeInFrames(s, AAudioStream_getFramesPerBurst(s) * (argc > 6 ? atoi(argv[6]) : 2));
    AAudioStream_requestStart(s);
    sleep(1);
    int64_t pos, ns;
    double lat = -1;
    if (AAudioStream_getTimestamp(s, CLOCK_MONOTONIC, &pos, &ns) == AAUDIO_OK) {
        struct timespec t;
        clock_gettime(CLOCK_MONOTONIC, &t);
        int64_t now = t.tv_sec * 1000000000LL + t.tv_nsec;
        int64_t written = AAudioStream_getFramesWritten(s);
        int64_t appNs = ns + (written - pos) * 1000000000LL / AAudioStream_getSampleRate(s);
        lat = (appNs - now) / 1e6;
    }
    printf("rate %d burst %d buffer %d sharing %s perf %d format %d latency %.1f ms\n",
           AAudioStream_getSampleRate(s), AAudioStream_getFramesPerBurst(s), AAudioStream_getBufferSizeInFrames(s),
           AAudioStream_getSharingMode(s) == AAUDIO_SHARING_MODE_EXCLUSIVE ? "exclusive" : "shared",
           AAudioStream_getPerformanceMode(s), AAudioStream_getFormat(s), lat);
    fflush(stdout);
    sleep((unsigned)atoi(argv[4]));
    printf("after %s s: xruns %d, buffer %d\n", argv[4], AAudioStream_getXRunCount(s), AAudioStream_getBufferSizeInFrames(s));
    AAudioStream_requestStop(s);
    AAudioStream_close(s);
    return 0;
}
