// odo for computers: a basic MESH. It plays instrument packages (.odoi, compiled with libfaust)
// with controller packages (.odoc, run in wasm3) on a touch screen, from the same files as odo
// on Android. Linux and Windows (SDL3).
//
//   odo [--library DIR]... [--controller ID] [--sound ID] [--log-midi] [--no-update] [--version] [--self-update]
//       [--selftest out.bmp [--touch X,Y]... [--move DX,DY] [--expect N] [--size WxH]]
//
// Library folders hold .odoi and .odoc files (default: the library folder beside odo, and
// ~/.local/share/odo/library). It checks GitHub for a newer odo when it opens (--no-update: not);
// the update row at the bottom of the rail installs it. The rail on
// the left picks the sound, the controller and its settings; the rest of the window is the
// controller. Keys: Tab hides the rail, F11 full screen, Esc quits.
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "draw.h"
#include "update.h"
#include "mpe/controller.h"
#include "mpe/engine.h"
#include "mpe/faust_loader.h"
#include "mpe/package.h"

namespace fs = std::filesystem;
using draw::rgba;

namespace {

constexpr int kRate = 48000;           // instruments are tuned at 48 kHz: SDL converts if the device differs
constexpr float kRail = 230;           // the rail's width, points
constexpr float kRow = 46;

std::string home() { const char* h = std::getenv("HOME"); return h ? h : "."; }

// The folder odo runs from: updates go there, and its own library is beside it.
std::string exeDir() {
    std::error_code ec;
    fs::path p = fs::read_symlink("/proc/self/exe", ec);
    return ec ? std::string(".") : p.parent_path().string();
}

// ---- the library -----------------------------------------------------------------------------

struct Instrument {
    std::unique_ptr<mpe::Package> pkg;
    int index = -1;                     // in the engine's table, once compiled
    std::string error;
};

struct Library {
    std::vector<Instrument> instruments;
    std::vector<std::unique_ptr<mpe::Package>> controllers;

    void scan(const std::string& dir) {
        std::error_code ec;
        if (!fs::is_directory(dir, ec)) { std::fprintf(stderr, "odo: no folder %s\n", dir.c_str()); return; }
        std::vector<fs::path> files;
        for (const auto& e : fs::directory_iterator(dir, ec)) files.push_back(e.path());
        std::sort(files.begin(), files.end());
        for (const auto& f : files) {
            std::string ext = f.extension().string(), err;
            if (ext != ".odoi" && ext != ".odoc") continue;
            auto pkg = mpe::Package::read(f.string(), &err);
            if (!pkg) { std::fprintf(stderr, "odo: %s: %s\n", f.c_str(), err.c_str()); continue; }
            // The same package in two library folders: the first one found is used.
            auto same = [&](const mpe::Package& o) { return o.id() == pkg->id() && o.version() == pkg->version(); };
            if (std::any_of(instruments.begin(), instruments.end(), [&](const Instrument& i) { return same(*i.pkg); }) ||
                std::any_of(controllers.begin(), controllers.end(), [&](const auto& c) { return same(*c); })) continue;
            if (pkg->isInstrument()) instruments.push_back({std::move(pkg), -1, ""});
            else controllers.push_back(std::move(pkg));
        }
    }
};

// The middle of an instrument's range, as a MIDI note (as odo's Native.Instrument.centerNote).
float centerNote(const mpe::Package& p) {
    const mpe::Json& r = p.manifest["range"];
    double lo = r[size_t(0)].num(), hi = r[size_t(1)].num();
    if (lo <= 0 || hi <= 0) return 60;
    return float(69 + 12 * std::log2(std::sqrt(lo * hi) / 440));
}

// ---- the app -----------------------------------------------------------------------------------

struct App {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_AudioStream* audio = nullptr;
    std::unique_ptr<mpe::Engine> engine;
    std::unique_ptr<mpe::MidiInput> midi;
    std::unique_ptr<mpe::ControllerHost> host;
    Library lib;
    std::string cache = home() + "/.cache/odo/instruments";

    int sound = 0, controller = 0;
    std::vector<double> settings;       // the controller's, by position
    bool railHidden = false, eink = false, logMidi = false;
    float surfaceW = 0, surfaceH = 0;

    // Compiling an instrument takes a moment: done on a worker, handed back to the main thread.
    std::thread worker;
    std::atomic<bool> compiling{false};
    std::mutex doneLock;
    int doneSound = -1, doneIndex = -1;
    std::string doneError;
    std::string status;

    std::unique_ptr<Updater> updater;
    char** argv = nullptr;                 // to start again after an update

    std::map<SDL_FingerID, int> fingers;   // touch screen fingers -> the controller's touch ids
    int nextFinger = 1;
    bool mouseDown = false;

    // Before SDL_Quit: the audio stream belongs to SDL.
    void shutdown() {
        if (worker.joinable()) worker.join();
        if (host) host->stop();
        host.reset();
        if (audio) SDL_DestroyAudioStream(audio);
        audio = nullptr;
    }
    ~App() { if (worker.joinable()) worker.join(); }

    float railW() const { return railHidden ? 0 : kRail; }

    static void SDLCALL audioCallback(void* self, SDL_AudioStream* stream, int additional, int) {
        App* a = static_cast<App*>(self);
        int frames = additional / int(2 * sizeof(float));
        if (frames <= 0) return;
        static thread_local std::vector<float> buf;
        buf.resize(size_t(frames) * 2);
        a->engine->render(buf.data(), frames, mpe::steadyNowNs());
        SDL_PutAudioStreamData(stream, buf.data(), int(buf.size() * sizeof(float)));
    }

    bool startAudio() {
        SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, "128");   // small buffers: about 3 ms each
        SDL_AudioSpec spec{SDL_AUDIO_F32, 2, kRate};
        audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &App::audioCallback, this);
        if (!audio) { std::fprintf(stderr, "odo: no audio: %s\n", SDL_GetError()); return false; }
        SDL_ResumeAudioStreamDevice(audio);
        return true;
    }

    // ---- sounds ---------------------------------------------------------------------------------

    void applySound(int k) {
        const mpe::Json& s = lib.instruments[size_t(k)].pkg->manifest["sound"];
        mpe::Performance p;
        mpe::PartConfig& c = p.part[0];
        c.reverb = float(s["reverb"].num(0.15));
        c.delay = float(s["delay"].num(0));
        c.brightness = float(s["brightness"].num(1));
        c.drive = float(s["drive"].num(0));
        c.pressureCurve = float(s["pressureCurve"].num(1));
        c.release = float(s["release"].num(0));
        p.reverbSize = float(s["reverbSize"].num(0.5));
        p.reverbDamp = float(s["reverbDamp"].num(0.4));
        engine->setPerformance(p);
        engine->setInstrument(0, lib.instruments[size_t(k)].index);
        if (host) host->instrument(0, centerNote(*lib.instruments[size_t(k)].pkg));
        status.clear();
    }

    void chooseSound(int k) {
        if (lib.instruments.empty()) return;
        k = (k % int(lib.instruments.size()) + int(lib.instruments.size())) % int(lib.instruments.size());
        sound = k;
        Instrument& in = lib.instruments[size_t(k)];
        if (in.index >= 0) { applySound(k); return; }
        if (compiling) return;                       // one at a time; picked again when it is done
        compiling = true;
        status = "Loading " + in.pkg->name() + "...";
        if (worker.joinable()) worker.join();
        worker = std::thread([this, k] {
            std::string err;
            int idx = mpe::loadInstrumentPackage(*lib.instruments[size_t(k)].pkg, cache, &err);
            std::lock_guard<std::mutex> g(doneLock);
            doneSound = k; doneIndex = idx; doneError = err;
            compiling = false;
        });
    }

    void collectCompiled() {
        std::lock_guard<std::mutex> g(doneLock);
        if (doneSound < 0) return;
        Instrument& in = lib.instruments[size_t(doneSound)];
        in.index = doneIndex;
        in.error = doneError;
        if (doneIndex < 0) status = "Could not load " + in.pkg->name() + ": " + doneError;
        else if (doneSound == sound) applySound(sound);
        int again = doneSound != sound ? sound : -1;   // the player moved on while it compiled
        doneSound = -1;
        if (again >= 0) { doneLock.unlock(); chooseSound(again); doneLock.lock(); }
    }

    // ---- controllers ----------------------------------------------------------------------------

    void chooseController(int k) {
        if (lib.controllers.empty()) return;
        k = (k % int(lib.controllers.size()) + int(lib.controllers.size())) % int(lib.controllers.size());
        controller = k;
        fingers.clear();
        host = std::make_unique<mpe::ControllerHost>([this](const uint8_t* b, int n) {
            if (logMidi) std::printf("midi %02x %02x %02x\n", b[0], n > 1 ? b[1] : 0, n > 2 ? b[2] : 0);
            midi->feed(b, n, mpe::steadyNowNs());
        });
        std::string err;
        if (!host->load(*lib.controllers[size_t(k)], &err)) { status = err; host.reset(); return; }
        settings.clear();
        const mpe::Json& s = host->manifest()["settings"];
        for (size_t i = 0; i < s.size(); i++) settings.push_back(s[i]["default"].num());
        surfaceW = surfaceH = 0;                     // init() at the next frame, with the surface's size
        if (!lib.instruments.empty() && lib.instruments[size_t(sound)].index >= 0)
            host->instrument(0, centerNote(*lib.instruments[size_t(sound)].pkg));
    }

    void changeSetting(size_t i, int dir) {
        const mpe::Json& s = host->manifest()["settings"][i];
        std::string type = s["type"].str();
        double v = settings[i];
        if (type == "switch") v = v != 0 ? 0 : 1;
        else if (type == "choice") {
            double n = double(s["options"].size());
            v = std::fmod(v + dir + n, n);
        } else {
            double step = s["step"].num(1);
            v = std::clamp(v + dir * (step > 0 ? step : 1), s["min"].num(0), s["max"].num(1));
        }
        settings[i] = v;
        host->setting(int(i), v);
    }

    std::string settingValue(size_t i) const {
        const mpe::Json& s = host->manifest()["settings"][i];
        std::string type = s["type"].str();
        double v = settings[i];
        if (type == "switch") return v != 0 ? "on" : "off";
        if (type == "choice") return s["options"][size_t(v)].str();
        char b[32];
        std::snprintf(b, sizeof b, "%g", v);
        return b;
    }

    // ---- input ----------------------------------------------------------------------------------

    // A tap on the rail: which row, and whether on its left (back) or right (on) half. The update
    // row is at the bottom.
    void railTap(float x, float y) {
        int ww, wh;
        SDL_GetWindowSize(window, &ww, &wh);
        if (updater && y > float(wh) - 76 && y < float(wh) - 40) {
            if (updater->state() == Updater::State::Available) updater->install();
            else if (updater->state() == Updater::State::Installed) { shutdown(); SDL_Quit(); updater->restart(argv); }
            else if (updater->state() == Updater::State::Failed || updater->state() == Updater::State::UpToDate) updater->check();
            return;
        }
        int row = int(y / kRow);
        int dir = x < kRail / 2 ? -1 : 1;
        if (row == 1) chooseController(controller + dir);
        else if (row == 2) chooseSound(sound + dir);
        else if (host && row >= 4) {
            size_t i = size_t(row - 4);
            if (i < settings.size()) changeSetting(i, dir);
        }
    }

    void surfaceTouch(int id, mpe::ControllerHost::Phase ph, float x, float y, float force) {
        if (host) host->touch(id, ph, x - railW(), y, 0, force);
    }

    void handle(const SDL_Event& e) {
        int ww, wh;
        SDL_GetWindowSize(window, &ww, &wh);
        switch (e.type) {
            case SDL_EVENT_FINGER_DOWN:
            case SDL_EVENT_FINGER_MOTION:
            case SDL_EVENT_FINGER_UP:
            case SDL_EVENT_FINGER_CANCELED: {
                float x = e.tfinger.x * float(ww), y = e.tfinger.y * float(wh);
                if (e.type == SDL_EVENT_FINGER_DOWN) {
                    if (x < railW()) { railTap(x, y); return; }
                    int id = nextFinger++;
                    fingers[e.tfinger.fingerID] = id;
                    surfaceTouch(id, mpe::ControllerHost::Down, x, y, e.tfinger.pressure);
                    return;
                }
                auto it = fingers.find(e.tfinger.fingerID);
                if (it == fingers.end()) return;
                if (e.type == SDL_EVENT_FINGER_MOTION) surfaceTouch(it->second, mpe::ControllerHost::Move, x, y, e.tfinger.pressure);
                else {
                    surfaceTouch(it->second, e.type == SDL_EVENT_FINGER_UP ? mpe::ControllerHost::Up : mpe::ControllerHost::Cancel,
                                 x, y, 0);
                    fingers.erase(it);
                }
                return;
            }
            // The mouse is one finger (id 0). Touches also arrive as mouse events: those are skipped.
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (e.button.which == SDL_TOUCH_MOUSEID || e.button.button != SDL_BUTTON_LEFT) return;
                if (e.button.x < railW()) { railTap(e.button.x, e.button.y); return; }
                mouseDown = true;
                surfaceTouch(0, mpe::ControllerHost::Down, e.button.x, e.button.y, 0.8f);
                return;
            case SDL_EVENT_MOUSE_MOTION:
                if (e.motion.which == SDL_TOUCH_MOUSEID || !mouseDown) return;
                surfaceTouch(0, mpe::ControllerHost::Move, e.motion.x, e.motion.y, 0.8f);
                return;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (e.button.which == SDL_TOUCH_MOUSEID || !mouseDown) return;
                mouseDown = false;
                surfaceTouch(0, mpe::ControllerHost::Up, e.button.x, e.button.y, 0);
                return;
            case SDL_EVENT_KEY_DOWN:
                if (e.key.key == SDLK_TAB) { railHidden = !railHidden; surfaceW = 0; }
                else if (e.key.key == SDLK_F11) SDL_SetWindowFullscreen(window, !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN));
                else if (e.key.key == SDLK_RIGHT) chooseSound(sound + 1);
                else if (e.key.key == SDLK_LEFT) chooseSound(sound - 1);
                return;
            default: return;
        }
    }

    // ---- drawing --------------------------------------------------------------------------------

    void drawRail(float h) {
        draw::Color bg = rgba(0x07080BFF), text = rgba(0xFFFFFFD9), dim = rgba(0x9AA2B4FF), accent = rgba(0xFF8A65FF);
        draw::fillRect(renderer, 0, 0, kRail, h, bg);
        auto row = [&](int k, const std::string& label, const std::string& value, bool arrows) {
            float y = float(k) * kRow;
            draw::text(renderer, 12, y + 19, 11, dim, label);
            draw::text(renderer, 12, y + 37, 13, text, value);
            if (arrows) {
                draw::text(renderer, kRail - 34, y + 34, 13, dim, "<");
                draw::text(renderer, kRail - 16, y + 34, 13, dim, ">");
            }
        };
        draw::text(renderer, 12, 30, 18, accent, "odo", 0, true);
        draw::text(renderer, 64, 30, 10, dim, "a MESH");
        row(1, "Controller", host ? lib.controllers[size_t(controller)]->name() : "none", true);
        std::string snd = lib.instruments.empty() ? "none" : lib.instruments[size_t(sound)].pkg->name();
        row(2, "Sound", compiling ? snd + "..." : snd, true);
        draw::line(renderer, 12, 3.5f * kRow, kRail - 12, 3.5f * kRow, rgba(0x9AA2B460), 1);
        if (host) {
            const mpe::Json& s = host->manifest()["settings"];
            for (size_t i = 0; i < settings.size(); i++) row(int(4 + i), s[i]["name"].str(), settingValue(i), true);
        }
        if (updater) {
            Updater::State us = updater->state();
            std::string line = "odo " + updater->current();
            std::string msg = us == Updater::State::Checking ? "checking for updates" : updater->message();
            bool act = us == Updater::State::Available || us == Updater::State::Installed;
            draw::text(renderer, 12, h - 62, 10, dim, line);
            draw::text(renderer, 12, h - 46, act ? 12 : 10, act ? accent : dim, msg, 0, act);
        }
        mpe::Stats st = engine->stats();
        char b[96];
        std::snprintf(b, sizeof b, "Latency %.1f ms (max %.1f)", st.eventAgeAvgMs, st.eventAgeMaxMs);
        draw::text(renderer, 12, h - 30, 10, dim, b);
        std::snprintf(b, sizeof b, "Load %.0f%%  Voices %d", 100 * st.loadAvg, engine->activeVoices());
        draw::text(renderer, 12, h - 14, 10, dim, b);
    }

    void frame() {
        collectCompiled();
        int ww, wh;
        SDL_GetWindowSize(window, &ww, &wh);
        float sw = float(ww) - railW(), sh = float(wh);
        if (host && (sw != surfaceW || sh != surfaceH)) {
            surfaceW = sw; surfaceH = sh;
            host->init(sw, sh, eink ? 1 : 0);
            for (size_t i = 0; i < settings.size(); i++) host->setting(int(i), settings[i]);
            if (!lib.instruments.empty() && lib.instruments[size_t(sound)].index >= 0)
                host->instrument(0, centerNote(*lib.instruments[size_t(sound)].pkg));
        }
        if (host) host->tick();
        SDL_SetRenderViewport(renderer, nullptr);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_Rect area{int(railW()), 0, int(sw), int(sh)};
        SDL_SetRenderViewport(renderer, &area);
        SDL_SetRenderClipRect(renderer, nullptr);
        if (host) draw::list(renderer, host->drawList(), sw, sh);
        else draw::text(renderer, 20, 40, 14, rgba(0xFFFFFFFF), "No controller packages (.odoc) in the library.");
        if (!status.empty()) draw::text(renderer, 16, sh - 16, 12, rgba(0xFFC857FF), status);
        SDL_SetRenderViewport(renderer, nullptr);
        if (!railHidden) drawRail(sh);
        SDL_RenderPresent(renderer);
    }
};

// Plays the controller by itself for a moment and saves what was on screen: a check that runs
// without a touch screen (SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy). Fingers go down at
// `touches` (fractions of the controller's surface; by default two places on the Tonnetz),
// optionally move by `move` (fractions), and stay down while the screenshot is taken. Passes if
// at least `expect` voices sound.
struct Selftest {
    std::vector<std::pair<float, float>> touches;
    float moveX = 0, moveY = 0;
    int expect = -1;
};

int selftest(App& app, const char* out, Selftest t) {
    auto wait = [&](int ms) {
        Uint64 end = SDL_GetTicks() + Uint64(ms);
        while (SDL_GetTicks() < end) { SDL_Event e; while (SDL_PollEvent(&e)) {} app.frame(); SDL_Delay(5); }
    };
    while (app.compiling || app.lib.instruments.empty() || app.lib.instruments[size_t(app.sound)].index < 0) {
        if (!app.compiling && !app.lib.instruments.empty() && app.lib.instruments[size_t(app.sound)].index < 0 &&
            !app.lib.instruments[size_t(app.sound)].error.empty()) { std::fprintf(stderr, "selftest: %s\n", app.status.c_str()); return 1; }
        wait(20);
    }
    wait(100);
    if (t.touches.empty()) {
        // The Tonnetz at size 7: the C major triangle, and a node.
        float dx = 1.f / 7, dy = app.surfaceW * dx / app.surfaceH;
        t.touches = {{0.5f + dx / 2, 0.5f - dy * 0.29f}, {0.5f - dx, 0.5f}};
        if (t.expect < 0) t.expect = 4;
    }
    if (t.expect < 0) t.expect = 1;
    auto at = [&](size_t k, float ox, float oy) {
        return std::make_pair(app.railW() + (t.touches[k].first + ox) * app.surfaceW, (t.touches[k].second + oy) * app.surfaceH);
    };
    for (size_t k = 0; k < t.touches.size(); k++) {
        auto p = at(k, 0, 0);
        app.surfaceTouch(int(k + 1), mpe::ControllerHost::Down, p.first, p.second, 0.8f);
        wait(30);
    }
    if (t.moveX != 0 || t.moveY != 0)
        for (int step = 1; step <= 10; step++) {
            for (size_t k = 0; k < t.touches.size(); k++) {
                auto p = at(k, t.moveX * float(step) / 10, t.moveY * float(step) / 10);
                app.surfaceTouch(int(k + 1), mpe::ControllerHost::Move, p.first, p.second, 0.8f);
            }
            wait(20);
        }
    wait(400);
    int voices = app.engine->activeVoices();
    mpe::Stats st = app.engine->stats();
    SDL_Surface* shot = SDL_RenderReadPixels(app.renderer, nullptr);
    bool saved = shot && SDL_SaveBMP(shot, out);
    if (shot) SDL_DestroySurface(shot);
    for (size_t k = 0; k < t.touches.size(); k++) {
        auto p = at(k, t.moveX, t.moveY);
        app.surfaceTouch(int(k + 1), mpe::ControllerHost::Up, p.first, p.second, 0);
    }
    wait(100);
    std::printf("selftest: %d voices sounding (want at least %d), %llu events, latency avg %.2f ms, controller errors: %s, screenshot %s\n",
                voices, t.expect, (unsigned long long)st.events, st.eventAgeAvgMs,
                app.host && !app.host->lastError().empty() ? app.host->lastError().c_str() : "none", saved ? out : "not saved");
    return voices >= t.expect && (!app.host || app.host->lastError().empty()) ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
    std::vector<std::string> dirs;
    const char* selftestOut = nullptr;
    std::string controllerId, soundId;
    Selftest test;
    int width = 1280, height = 720;
    bool logMidi = false, checkUpdates = true;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        bool more = i + 1 < argc;
        float x = 0, y = 0;
        if (a == "--library" && more) dirs.push_back(argv[++i]);
        else if (a == "--selftest" && more) selftestOut = argv[++i];
        else if (a == "--controller" && more) controllerId = argv[++i];
        else if (a == "--sound" && more) soundId = argv[++i];
        else if (a == "--log-midi") logMidi = true;
        else if (a == "--no-update") checkUpdates = false;
        else if (a == "--version") { std::printf("odo %s\n", ODO_DESKTOP_VERSION); return 0; }
        else if (a == "--self-update") {
            // Without the window: check, install if newer, and say what happened.
            Updater u(ODO_DESKTOP_VERSION, exeDir());
            auto settle = [&] { while (u.state() == Updater::State::Checking || u.state() == Updater::State::Downloading) SDL_Delay(50); };
            u.check(); settle();
            if (u.state() == Updater::State::Available) { u.install(); settle(); }
            std::printf("odo %s: %s\n", ODO_DESKTOP_VERSION, u.message().c_str());
            return u.state() == Updater::State::Failed ? 1 : 0;
        }
        else if (a == "--touch" && more && std::sscanf(argv[i + 1], "%f,%f", &x, &y) == 2) { test.touches.push_back({x, y}); i++; }
        else if (a == "--move" && more && std::sscanf(argv[i + 1], "%f,%f", &x, &y) == 2) { test.moveX = x; test.moveY = y; i++; }
        else if (a == "--expect" && more) test.expect = std::atoi(argv[++i]);
        else if (a == "--size" && more && std::sscanf(argv[i + 1], "%dx%d", &width, &height) == 2) i++;
        else {
            std::fprintf(stderr, "usage: odo [--library DIR]... [--controller ID] [--sound ID] [--log-midi] [--no-update] [--version]\n"
                                 "           [--selftest out.bmp [--touch X,Y]... [--move DX,DY] [--expect N] [--size WxH]]\n");
            return 2;
        }
    }
    if (dirs.empty()) {
        std::error_code ec;
        if (fs::is_directory(exeDir() + "/library", ec)) dirs.push_back(exeDir() + "/library");
        dirs.push_back(home() + "/.local/share/odo/library");
    }

    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");   // a touch is a finger, not also a mouse
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) { std::fprintf(stderr, "odo: %s\n", SDL_GetError()); return 1; }
    App app;
    for (const auto& d : dirs) app.lib.scan(d);
    std::printf("odo: %zu instruments, %zu controllers\n", app.lib.instruments.size(), app.lib.controllers.size());

    app.engine = std::make_unique<mpe::Engine>(kRate);
    app.engine->setVolume(0.8f);
    app.midi = std::make_unique<mpe::MidiInput>(*app.engine);
    app.logMidi = logMidi;
    app.argv = argv;
    if (checkUpdates && !selftestOut) {
        app.updater = std::make_unique<Updater>(ODO_DESKTOP_VERSION, exeDir());
        app.updater->check();
    }
    if (!SDL_CreateWindowAndRenderer("odo", width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, &app.window, &app.renderer)) {
        std::fprintf(stderr, "odo: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetRenderVSync(app.renderer, 1);
    SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
    float density = SDL_GetWindowDisplayScale(app.window);
    SDL_SetRenderScale(app.renderer, density > 0 ? density : 1, density > 0 ? density : 1);
    app.startAudio();

    // Start on the flute (or --sound) and the first controller (or --controller).
    if (soundId.empty()) soundId = "patrickreynolds/flute";
    int sound = 0, ctl = 0;
    for (size_t i = 0; i < app.lib.instruments.size(); i++) {
        const auto& p = *app.lib.instruments[i].pkg;
        if (p.id() == soundId || p.manifest["aliases"][size_t(0)].str() == soundId) sound = int(i);
    }
    for (size_t i = 0; i < app.lib.controllers.size(); i++) if (app.lib.controllers[i]->id() == controllerId) ctl = int(i);
    if (!controllerId.empty() && (app.lib.controllers.empty() || app.lib.controllers[size_t(ctl)]->id() != controllerId))
        std::fprintf(stderr, "odo: no controller %s in the library\n", controllerId.c_str());
    app.chooseController(ctl);
    app.chooseSound(sound);

    if (selftestOut) { int rc = selftest(app, selftestOut, test); app.shutdown(); SDL_Quit(); return rc; }
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT || (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)) running = false;
            else app.handle(e);
        }
        app.frame();
    }
    app.shutdown();
    SDL_Quit();
    return 0;
}
