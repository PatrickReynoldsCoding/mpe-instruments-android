// One loop: records what is played, then plays it round and round while more is played over it.
// The engine has several (Engine::kLoops), each with its own length and layers.
//
//   press()  Empty: arm. Recording starts with the first note played (so the loop starts on the
//            beat you play), up to kMaxSeconds.
//            Armed: cancel.  Recording the first loop: stop, and that is the loop's length
//            (rounded to whole beats of the tempo if snap is on).
//            Playing: record one more layer, for exactly one pass of the loop, starting now.
//            Recording a layer: stop it early (what was played so far is kept).
//   undo()   drop the last layer (the first loop too, which empties the looper).
//   clear()  drop everything.
//   setPaused(true) holds the loop where it is, silent; false carries on.
//   setMuted(true) keeps it going round, silent (so it comes back in time).
//   setLayer(i, volume, muted)
//
// What is recorded is the whole mix after the effects (so a layer keeps its reverb), and none of
// any loop. Layers are kept as 16-bit audio. The audio thread never allocates: the control
// thread makes each layer's buffer and hands it over, and takes it back to free.
#pragma once
#include <atomic>
#include <cstdint>
#include "mpe/spsc.h"

namespace mpe {

enum class LoopState : int { Empty = 0, Armed = 1, Recording = 2, Playing = 3, Overdubbing = 4 };

struct LoopView {
    LoopState state = LoopState::Empty;
    int length = 0;            // frames; 0 until the first loop is recorded
    int pos = 0;               // frames into the loop (while recording the first: frames recorded)
    int layers = 0;            // finished layers
    bool paused = false, muted = false;
    float level[8] = {};       // each layer's loudness (peak), for the screen
};

class Looper {
public:
    static constexpr int kMaxLayers = 8;
    static constexpr int kMaxSeconds = 60;

    explicit Looper(int sampleRate);
    ~Looper();

    // ---- control thread ----
    void press();
    void undo();
    void clear();
    void setPaused(bool p);
    void setMuted(bool m);
    void setLayer(int i, float volume, bool muted);
    void setSnap(bool beats) { snap_ = beats; }
    LoopView view() const;
    void collect();                           // frees layers the audio thread has let go of

    // ---- audio thread ----
    // inL/inR: what was played live in this block (recorded); outL/outR: the mix, the loop is added to it.
    // noteStarted: a note started in this block. bpm: for snapping the length to beats.
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, bool noteStarted, float bpm);

private:
    struct Layer { int16_t* buf = nullptr; int frames = 0; float vol = 1; bool mute = false; float peak = 0; };
    enum Op { Arm, Cancel, Stop, Overdub, Undo, Clear, Pause, Resume, SetLayer, Mute, Unmute };
    struct Cmd { Op op; int16_t* buf = nullptr; int frames = 0; int layer = 0; float vol = 1; bool mute = false; };

    void endFirst(float bpm);
    void endOverdub();
    void retire(int16_t* b) { if (b && !retired_.push(b)) leaked_++; }

    const int sr_;
    const int fade_;                           // frames of fade at a recording's edges
    Spsc<Cmd> cmds_{64};
    Spsc<int16_t*> retired_{64};
    std::atomic<bool> snap_{true};
    int leaked_ = 0;

    // audio thread
    Layer layers_[kMaxLayers];
    int count_ = 0;                            // finished layers
    LoopState state_ = LoopState::Empty;
    bool paused_ = false, muted_ = false;
    int length_ = 0, pos_ = 0, rec_ = 0;       // rec_: frames recorded into the layer being made
    float recPeak_ = 0;

    // published for the screen and the control thread
    std::atomic<int> pubState_{0}, pubLength_{0}, pubPos_{0}, pubCount_{0};
    std::atomic<bool> pubPaused_{false}, pubMuted_{false};
    std::atomic<float> pubLevel_[kMaxLayers] = {};
};

}  // namespace mpe
