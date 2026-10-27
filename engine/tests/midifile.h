// Reads a Standard MIDI File (what `arecordmidi` writes) into channel messages with
// their real time, so a recording can be replayed at the speed it was played.
#pragma once
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "mpe/midi.h"

struct TimedMsg {
    double seconds;
    mpe::Msg msg;
};

inline std::vector<TimedMsg> loadMidiFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::vector<uint8_t> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::vector<TimedMsg> out;
    if (d.size() < 14 || std::string(d.begin(), d.begin() + 4) != "MThd") return out;
    auto be16 = [&](size_t p) { return (d[p] << 8) | d[p + 1]; };
    auto be32 = [&](size_t p) { return (uint32_t(d[p]) << 24) | (d[p + 1] << 16) | (d[p + 2] << 8) | d[p + 3]; };
    const int tracks = be16(10), division = be16(12);
    if (division & 0x8000) return out;              // SMPTE time: not written by arecordmidi
    size_t p = 8 + be32(4);
    // Tempo changes apply to every track; collect (tick, us-per-quarter) first, from all tracks.
    struct Raw { uint64_t tick; mpe::Msg msg; };
    std::vector<Raw> raws;
    std::vector<std::pair<uint64_t, uint32_t>> tempos = {{0, 500000}};
    for (int t = 0; t < tracks && p + 8 <= d.size(); t++) {
        if (std::string(d.begin() + p, d.begin() + p + 4) != "MTrk") break;
        size_t q = p + 8, end = std::min(d.size(), q + be32(p + 4));
        uint64_t tick = 0;
        uint8_t running = 0;
        auto vlq = [&]() { uint32_t v = 0; uint8_t b; do { b = d[q++]; v = (v << 7) | (b & 127); } while ((b & 0x80) && q < end); return v; };
        while (q < end) {
            tick += vlq();
            uint8_t s = d[q];
            if (s == 0xFF) {
                uint8_t type = d[q + 1]; q += 2;
                uint32_t len = vlq();
                if (type == 0x51 && len == 3) tempos.push_back({tick, (uint32_t(d[q]) << 16) | (d[q + 1] << 8) | d[q + 2]});
                q += len;
                continue;
            }
            if (s == 0xF0 || s == 0xF7) { q++; q += vlq(); continue; }
            if (s & 0x80) { running = s; q++; }
            if (!running) break;
            mpe::Msg m;
            m.status = running;
            m.d1 = d[q++];
            uint8_t type = running & 0xF0;
            if (type != mpe::kProgram && type != mpe::kChanPress) m.d2 = d[q++];
            raws.push_back({tick, m});
        }
        p = end;
    }
    std::stable_sort(raws.begin(), raws.end(), [](const Raw& a, const Raw& b) { return a.tick < b.tick; });
    std::stable_sort(tempos.begin(), tempos.end());
    size_t ti = 0;
    double secAtTempo = 0;
    uint64_t tickAtTempo = 0;
    uint32_t us = tempos[0].second;
    for (const Raw& r : raws) {
        while (ti + 1 < tempos.size() && tempos[ti + 1].first <= r.tick) {
            ti++;
            secAtTempo += double(tempos[ti].first - tickAtTempo) * us / 1e6 / division;
            tickAtTempo = tempos[ti].first;
            us = tempos[ti].second;
        }
        out.push_back({secAtTempo + double(r.tick - tickAtTempo) * us / 1e6 / division, r.msg});
    }
    return out;
}
