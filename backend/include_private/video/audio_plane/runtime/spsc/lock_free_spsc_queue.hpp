// lock_free_spsc_queue.hpp — P6E: lock-free single-producer single-consumer queue
#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

namespace kivo::video::audio_plane::backend::runtime::spsc {

// LockFreeSpscQueue<T>
// Simple ring-buffer based SPSC queue using atomics.
// Producer: push(). Consumer: pop().
// is_lock_free() returns true (honest — uses atomic load/store).
template <typename T>
class LockFreeSpscQueue {
public:
    explicit LockFreeSpscQueue(int capacity)
        : buffer_(capacity), capacity_(capacity) {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

    bool push(const T& item) noexcept {
        int h = head_.load(std::memory_order_relaxed);
        int next_h = (h + 1) % capacity_;
        if (next_h == tail_.load(std::memory_order_acquire)) {
            return false;  // full
        }
        buffer_[h] = item;
        head_.store(next_h, std::memory_order_release);
        return true;
    }

    bool pop(T& out) noexcept {
        int t = tail_.load(std::memory_order_relaxed);
        if (t == head_.load(std::memory_order_acquire)) {
            return false;  // empty
        }
        out = buffer_[t];
        tail_.store((t + 1) % capacity_, std::memory_order_release);
        return true;
    }

    bool is_lock_free() const noexcept {
        return true;  // honest — uses atomic load/store, no mutex
    }

    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    int size() const noexcept {
        int h = head_.load(std::memory_order_acquire);
        int t = tail_.load(std::memory_order_acquire);
        int diff = h - t;
        return (diff >= 0) ? diff : (diff + capacity_);
    }

    int capacity() const noexcept { return capacity_; }

private:
    std::vector<T> buffer_;
    int capacity_;
    std::atomic<int> head_{0};
    std::atomic<int> tail_{0};
};

}  // namespace kivo::video::audio_plane::backend::runtime::spsc
