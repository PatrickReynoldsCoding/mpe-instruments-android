// Single-producer single-consumer ring buffer. Wait-free on both sides, no
// allocation after construction: safe to use between a MIDI thread and the audio thread.
#pragma once
#include <atomic>
#include <cstddef>
#include <vector>

namespace mpe {

template <typename T>
class Spsc {
public:
    explicit Spsc(size_t capacityPow2) : buf_(capacityPow2), mask_(capacityPow2 - 1) {}

    bool push(const T& v) {                      // producer thread only
        size_t h = head_.load(std::memory_order_relaxed);
        if (h - tail_.load(std::memory_order_acquire) >= buf_.size()) return false;   // full
        buf_[h & mask_] = v;
        head_.store(h + 1, std::memory_order_release);
        return true;
    }
    bool pop(T& out) {                           // consumer thread only
        size_t t = tail_.load(std::memory_order_relaxed);
        if (t == head_.load(std::memory_order_acquire)) return false;                 // empty
        out = buf_[t & mask_];
        tail_.store(t + 1, std::memory_order_release);
        return true;
    }

private:
    std::vector<T> buf_;
    size_t mask_;
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
};

}  // namespace mpe
