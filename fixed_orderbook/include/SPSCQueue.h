#pragma once
#include <atomic>
#include <array>
#include <cstddef>

// ─── SPSCQueue ────────────────────────────────────────────────────────────────
// Single-Producer Single-Consumer lock-free ring buffer.
// Cache-line aligned to prevent false sharing.
// Used to pass events from the feed thread → processing thread.
// ─────────────────────────────────────────────────────────────────────────────
template<typename T, size_t Capacity>
class SPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0,
                  "Capacity must be a power of 2");
public:
    bool push(const T& item) {
        const size_t w = writePos_.load(std::memory_order_relaxed);
        const size_t next = (w + 1) & (Capacity - 1);
        if (next == readPos_.load(std::memory_order_acquire)) return false; // full
        buffer_[w] = item;
        writePos_.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        const size_t r = readPos_.load(std::memory_order_relaxed);
        if (r == writePos_.load(std::memory_order_acquire)) return false;  // empty
        item = buffer_[r];
        readPos_.store((r + 1) & (Capacity - 1), std::memory_order_release);
        return true;
    }

    bool empty() const {
        return readPos_.load(std::memory_order_acquire) ==
               writePos_.load(std::memory_order_acquire);
    }

private:
    alignas(64) std::array<T, Capacity> buffer_;
    alignas(64) std::atomic<size_t> writePos_ { 0 };
    alignas(64) std::atomic<size_t> readPos_  { 0 };
};
