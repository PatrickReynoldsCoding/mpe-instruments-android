// Packages: an instrument package compiled while running (libfaust) sounds like the same
// instrument compiled in, and a controller package (the Tonnetz) plays the notes it should.
#include <algorithm>
#include <cstring>
#include <ctime>
#include "check.h"
#include "mpe/controller.h"
#include "mpe/package.h"
#ifdef MPE_LIBFAUST
#include "mpe/faust_loader.h"
#endif

using namespace mpe;

// One finger plays `note` for a second; the left channel.
static std::vector<float> play(int index, int note) {
    Engine e(kSr);
    MidiInput in(e);
    e.setInstrument(index);
    std::vector<float> out, buf(256);
    int64_t t = 1000000000;
    auto feed = [&](uint8_t a, uint8_t b, uint8_t c, int n) { uint8_t m[3] = {a, b, c}; in.feed(m, n, t); };
    feed(0xD1, 90, 0, 2);
    feed(0x91, uint8_t(note), 20, 3);
    for (int k = 0; k < kSr / 128; k++) {
        e.render(buf.data(), 128, t);
        for (int i = 0; i < 128; i++) out.push_back(buf[2 * i]);
        t += 1000000000LL * 128 / kSr;
    }
    return out;
}

static void instrumentPackage() {
    std::string err;
    auto pkg = Package::read(MPE_TEST_PACKAGES "/flute.odoi", &err);
    CHECK(pkg != nullptr, "flute.odoi reads (%s)", err.c_str());
    if (!pkg) return;
    CHECK(pkg->isInstrument() && pkg->id() == "patrickreynolds/flute", "it is the flute (%s)", pkg->id().c_str());
    CHECK(pkg->manifest["aliases"][size_t(0)].str() == "winds/flute", "its old id is an alias");
    CHECK(pkg->has("lib/stdfaust.lib") && pkg->has("lib/mpe.lib"), "it carries its libraries");
#ifdef MPE_LIBFAUST
    int builtin = -1;
    for (int i = 0; i < instrumentCount(); i++) if (instrumentAt(i).id == "patrickreynolds/flute") { builtin = i; break; }
    int loaded = loadInstrumentPackage(*pkg, MPE_TEST_CACHE, &err);
    CHECK(loaded >= 0, "libfaust compiles the package (%s)", err.c_str());
    if (loaded < 0 || builtin < 0) return;
    CHECK(findInstrument("winds/flute") == loaded, "found by its alias");
    std::vector<float> a = play(builtin, 72), b = play(loaded, 72);
    double diff = 0, level = rms(a, 0, a.size());
    for (size_t i = 0; i < a.size(); i++) diff += double(a[i] - b[i]) * (a[i] - b[i]);
    diff = std::sqrt(diff / double(a.size()));
    CHECK(level > 0.01, "the flute sounds (rms %.3f)", level);
    CHECK(diff < level * 1e-3, "compiled in and compiled while running sound the same (difference %.2g of %.2g)", diff, level);

    auto cello = Package::read(MPE_TEST_PACKAGES "/cello.odoi", &err);
    int c = cello ? loadInstrumentPackage(*cello, MPE_TEST_CACHE, &err) : -1;
    CHECK(c >= 0 && instrumentAt(c).createPost != nullptr, "an instrument with a body (post) loads with it (%s)", err.c_str());
    if (c >= 0) CHECK(rms(play(c, 48), 0, kSr) > 0.005, "the cello sounds");
#else
    printf("(no libfaust: compiling while running not tested)\n");
#endif
}

struct Heard { int ons = 0, offs = 0; std::vector<int> notes; };

static void controllerPackage() {
    std::string err;
    auto pkg = Package::read(MPE_TEST_CONTROLLERS "/tonnetz.odoc", &err);
    CHECK(pkg != nullptr, "tonnetz.odoc reads (%s)", err.c_str());
    if (!pkg) return;
    CHECK(pkg->isController() && pkg->manifest["settings"].size() == 9, "it is the Tonnetz, with its 9 settings");
    Heard h;
    ControllerHost host([&](const uint8_t* m, int n) {
        if (n == 3 && (m[0] & 0xF0) == 0x90 && m[2] > 0) { h.ons++; h.notes.push_back(m[1]); }
        if (n == 3 && (m[0] & 0xF0) == 0x80) h.offs++;
    });
    CHECK(host.load(*pkg, &err), "wasm3 runs it (%s)", err.c_str());
    // 700 x 400 dp at size 7: dx = 100, the key's note (C) in the middle; the middle of the instrument is C4.
    host.init(700, 400, 0);
    host.instrument(0, 60);
    CHECK(host.takeDirty() && host.drawList().size() > 1000, "it draws (%zu bytes)", host.drawList().size());

    host.touch(1, ControllerHost::Down, 350, 200, 0, 0);          // the middle node: C
    CHECK(h.ons == 1 && h.notes.size() == 1 && h.notes[0] % 12 == 0, "a node plays its note (%d notes, first %d)", h.ons,
          h.notes.empty() ? -1 : h.notes[0]);
    host.touch(1, ControllerHost::Up, 350, 200, 0, 0);
    CHECK(h.offs == 1, "lifting ends it");

    // The middle of the up-triangle right of C: C, G and E: C major.
    h = Heard();
    host.touch(2, ControllerHost::Down, 350 + 50, 200 - 29, 0, 0);
    std::vector<int> pcs;
    for (int n : h.notes) pcs.push_back(n % 12);
    std::sort(pcs.begin(), pcs.end());
    CHECK(h.ons == 3 && pcs == std::vector<int>({0, 4, 7}), "a triangle plays its triad (C major: %d notes)", h.ons);
    // Slide to the down-triangle next to it (E minor: E, G, B): C stops, B starts, E and G keep sounding.
    int before = h.ons;
    host.touch(2, ControllerHost::Move, 350 + 100, 200 - 58, 0, 0);
    CHECK(h.offs == 1 && h.ons == before + 1, "sliding to the next triangle changes only the note they don't share");
    host.stop();
    CHECK(h.offs == 4, "stop lets go of the three still sounding (%d note-offs in all)", h.offs);

    // Strum (setting 5 = Slow, 70 ms): the chord's notes come one after another.
    host.setting(5, 2);
    h = Heard();
    host.touch(3, ControllerHost::Down, 400, 171, 0, 0);
    int first = h.ons;
    for (int k = 0; k < 30 && h.ons < 3; k++) { host.tick(); struct timespec ts{0, 10000000}; nanosleep(&ts, nullptr); }
    CHECK(first == 1 && h.ons == 3, "a strum starts one note, then the rest in time (%d, then %d)", first, h.ons);
    CHECK(host.lastError().empty(), "no errors from the WebAssembly (%s)", host.lastError().c_str());
}

int main() {
    instrumentPackage();
    controllerPackage();
    return finish();
}
