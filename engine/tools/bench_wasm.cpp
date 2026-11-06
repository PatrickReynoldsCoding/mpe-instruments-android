// The interpreter experiment: each instrument compiled to native code (in the engine) against the
// same instrument as WebAssembly run by wasm3 (tools/wasm_instruments.py makes those).
//   bench_wasm WASM_DIR [fingers=8] [frames-per-callback=96] [seconds=2] [name ...]
// For each: render time as a share of the audio (100% = real time), native and WebAssembly, and
// the smallest callback each can play the fingers at with room to spare (peak under 70%): the
// buffer, and so the latency, each needs.
#include <sys/stat.h>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "mpe/engine.h"
#include "mpe/wasm_dsp.h"

namespace {
const int kSr = 48000;

std::string slurp(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}
bool exists(const std::string& p) { struct stat st; return stat(p.c_str(), &st) == 0; }

// The left channel of what was played, for comparing the two.
std::vector<float> lastOut;

mpe::Stats run(int index, int fingers, int block, double seconds) {
    mpe::Engine e(kSr);
    mpe::MidiInput in(e);
    e.setInstrument(index);
    std::vector<float> buf(size_t(block) * 2);
    int64_t t = 1000000000;
    for (int f = 1; f <= fingers && f < 16; f++) {
        uint8_t on[3] = {uint8_t(0x90 | f), uint8_t(48 + 3 * f), 90};
        in.feed(on, 3, t);
        uint8_t press[2] = {uint8_t(0xD0 | f), 90};
        in.feed(press, 2, t);
    }
    lastOut.clear();
    for (int done = 0; done < seconds * kSr; done += block) {
        e.render(buf.data(), block, t);
        for (int i = 0; i < block; i++) lastOut.push_back(buf[size_t(2 * i)]);
        t += int64_t(1e9 * block / kSr);
    }
    return e.stats();
}

double rms(const std::vector<float>& x) { double s = 0; for (float v : x) s += double(v) * v; return x.empty() ? 0 : std::sqrt(s / double(x.size())); }

// The smallest callback (from the usual sizes) whose peak load stays under 70%.
int smallest(int index, int fingers, double seconds) {
    for (int b : {32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024})
        if (run(index, fingers, b, seconds).loadPeak < 0.7) return b;
    return -1;
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: bench_wasm WASM_DIR [fingers] [block] [seconds] [name ...]\n"); return 2; }
    std::string dir = argv[1];
    int fingers = argc > 2 ? std::atoi(argv[2]) : 8, block = argc > 3 ? std::atoi(argv[3]) : 96;
    double seconds = argc > 4 ? std::atof(argv[4]) : 2;
    std::vector<std::string> names;
    for (int i = 5; i < argc; i++) names.push_back(argv[i]);
    if (names.empty()) {
        if (DIR* d = opendir(dir.c_str())) {
            while (dirent* e = readdir(d)) {
                std::string n = e->d_name;
                if (n.size() > 5 && n.compare(n.size() - 5, 5, ".wasm") == 0 && n.find(".post.") == std::string::npos)
                    names.push_back(n.substr(0, n.size() - 5));
            }
            closedir(d);
        }
        std::sort(names.begin(), names.end());
    }
    std::printf("%d fingers, %d frames per callback (%.2f ms), %.1f s each, %d Hz\n", fingers, block, 1000.0 * block / kSr, seconds, kSr);
    std::printf("%-16s %9s %9s %9s %9s %7s %13s %13s %10s\n", "instrument", "native", "peak", "wasm", "peak", "x", "native needs", "wasm needs",
                "same sound");
    double sumRatio = 0;
    int counted = 0;
    for (const auto& n : names) {
        int native = mpe::findInstrument(("patrickreynolds/" + n).c_str());
        std::string err, base = dir + "/" + n;
        std::string postW = exists(base + ".post.wasm") ? slurp(base + ".post.wasm") : "";
        std::string postJ = postW.empty() ? "" : slurp(base + ".post.json");
        int wasm = mpe::loadWasmInstrument("wasm/" + n, slurp(base + ".wasm"), slurp(base + ".json"), postW, postJ, &err);
        if (native < 0 || wasm < 0) { std::printf("%-16s %s\n", n.c_str(), native < 0 ? "no native build" : err.c_str()); continue; }
        mpe::Stats a = run(native, fingers, block, seconds);
        std::vector<float> outA = lastOut;
        mpe::Stats b = run(wasm, fingers, block, seconds);
        double diff = 0;
        for (size_t i = 0; i < outA.size() && i < lastOut.size(); i++) diff += double(outA[i] - lastOut[i]) * (outA[i] - lastOut[i]);
        diff = outA.empty() ? 0 : std::sqrt(diff / double(outA.size()));
        double level = rms(outA);
        double ratio = a.loadAvg > 0 ? b.loadAvg / a.loadAvg : 0;
        sumRatio += ratio;
        counted++;
        int na = smallest(native, fingers, seconds / 2), nb = smallest(wasm, fingers, seconds / 2);
        auto ms = [](int b) { char s[24]; if (b < 0) std::snprintf(s, sizeof s, "none"); else std::snprintf(s, sizeof s, "%d (%.1f ms)", b, 1000.0 * b / kSr); return std::string(s); };
        // Same sound: the difference, as a share of the level (float rounding differs a little).
        char same[24];
        std::snprintf(same, sizeof same, level > 1e-5 ? "%.1e" : "silent", diff / std::max(level, 1e-9));
        std::printf("%-16s %8.1f%% %8.1f%% %8.1f%% %8.1f%% %6.1fx %13s %13s %10s\n", n.c_str(), 100 * a.loadAvg, 100 * a.loadPeak,
                    100 * b.loadAvg, 100 * b.loadPeak, ratio, ms(na).c_str(), ms(nb).c_str(), same);
        std::fflush(stdout);
    }
    if (counted) std::printf("\nWebAssembly costs %.1fx native on average over %d instruments.\n", sumRatio / counted, counted);
    return 0;
}
