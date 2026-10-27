// How expensive is the engine? Plays N fingers on every instrument and reports render time
// as a fraction of the audio produced (1.0 = exactly real time, over = dropouts).
//   bench [fingers=8] [frames-per-callback=96] [seconds=3]
#include <string>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "mpe/engine.h"

int main(int argc, char** argv) {
    int fingers = argc > 1 ? atoi(argv[1]) : 8, block = argc > 2 ? atoi(argv[2]) : 96;
    double seconds = argc > 3 ? atof(argv[3]) : 3;
    const bool fx = argc > 4 && std::string(argv[4]) == "fx";          // reverb and delay on
    const int sr = 48000;
    int n = 0;
    const mpe::InstrumentInfo* all = mpe::instruments(n);
    printf("%d fingers, %d frames per callback (%.2f ms), %.0f s each, %d Hz%s\n\n", fingers, block, 1000.0 * block / sr, seconds, sr,
           fx ? ", reverb and delay on" : "");
    printf("%-22s %8s %8s\n", "instrument", "avg", "peak");
    for (int i = 0; i < n; i++) {
        mpe::Engine e(sr);
        mpe::MidiInput in(e);
        e.setInstrument(i);
        if (fx) { mpe::Performance p; p.part[0].reverb = 0.4f; p.part[0].delay = 0.3f; e.setPerformance(p); }
        std::vector<float> buf(block * 2);
        int64_t t = 1000000000;
        auto send = [&](uint8_t s, uint8_t a, uint8_t b) { uint8_t m[3] = {s, a, b}; in.feed(m, 3, t); };
        for (int f = 1; f <= fingers && f < 16; f++) {
            send(0x90 | f, 48 + 3 * f, 90);
            uint8_t press[2] = {uint8_t(0xD0 | f), 90};                     // 2 bytes: a 3rd is another pressure
            in.feed(press, 2, t);
        }
        for (int done = 0; done < seconds * sr; done += block) {
            e.render(buf.data(), block, t);
            t += int64_t(1e9 * block / sr);
        }
        mpe::Stats s = e.stats();
        printf("%-22s %7.1f%% %7.1f%%%s\n", all[i].id, 100 * s.loadAvg, 100 * s.loadPeak, s.loadPeak > 1 ? "   <-- over real time" : "");
    }
    return 0;
}
