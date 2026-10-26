// Reads `aseqdump` text (what captures/*.txt are) into MIDI messages.
//   28:0   Note on                 1, note 60, velocity 80
//   28:0   Pitch bend              1, value -10
//   28:0   Control change          1, controller 74, value 31
//   28:0   Channel aftertouch      1, value 3
#pragma once
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include "mpe/midi.h"

inline std::vector<mpe::Msg> loadAseqdump(const std::string& path) {
    std::vector<mpe::Msg> out;
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        const char* s = line.c_str();
        const char* p;
        int ch = 0, a = 0, b = 0;
        mpe::Msg m;
        if ((p = std::strstr(s, "Note on")) && sscanf(p, "Note on %d, note %d, velocity %d", &ch, &a, &b) == 3)
            { m.status = 0x90 | (ch & 15); m.d1 = a; m.d2 = b; }
        else if ((p = std::strstr(s, "Note off")) && sscanf(p, "Note off %d, note %d, velocity %d", &ch, &a, &b) >= 2)
            { m.status = 0x80 | (ch & 15); m.d1 = a; m.d2 = b; }
        else if ((p = std::strstr(s, "Pitch bend")) && sscanf(p, "Pitch bend %d, value %d", &ch, &a) == 2)
            { int v = a + 8192; m.status = 0xE0 | (ch & 15); m.d1 = v & 127; m.d2 = (v >> 7) & 127; }
        else if ((p = std::strstr(s, "Control change")) && sscanf(p, "Control change %d, controller %d, value %d", &ch, &a, &b) == 3)
            { m.status = 0xB0 | (ch & 15); m.d1 = a; m.d2 = b; }
        else if ((p = std::strstr(s, "Channel aftertouch")) && sscanf(p, "Channel aftertouch %d, value %d", &ch, &a) == 2)
            { m.status = 0xD0 | (ch & 15); m.d1 = a; }
        else continue;
        out.push_back(m);
    }
    return out;
}
