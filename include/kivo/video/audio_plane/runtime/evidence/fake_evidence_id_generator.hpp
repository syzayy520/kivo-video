// fake_evidence_id_generator.hpp — P6B runtime: fake atomic evidence ID generator
// Header-only fake implementation. Platform-neutral.
#pragma once

#include <atomic>
#include <cstdint>

namespace kivo::video::audio_plane::runtime::evidence {

// Fake monotonic evidence ID generator backed by std::atomic<uint64_t>.
// - 0 is reserved as the invalid sentinel; first next_id() returns 1.
// - When the counter would wrap to 0, next_id() returns 0 (invalid) and
//   sets the wrap flag. Callers must stop emitting evidence and reset.
class FakeEvidenceIdGenerator {
public:
    FakeEvidenceIdGenerator() = default;

    // Returns the next evidence ID. Returns 0 (invalid) on wrap.
    // Uses compare_exchange to detect wrap atomically: if counter is at
    // UINT64_MAX, sets the wrap flag and returns 0 without advancing.
    uint64_t next_id() noexcept {
        uint64_t expected = counter_.load(std::memory_order_relaxed);
        for (;;) {
            if (expected == UINT64_MAX) {
                wrap_flag_.store(true, std::memory_order_relaxed);
                return 0;
            }
            uint64_t desired = expected + 1;
            if (counter_.compare_exchange_weak(
                    expected, desired,
                    std::memory_order_relaxed,
                    std::memory_order_relaxed)) {
                return desired;
            }
            // expected reloaded by compare_exchange_weak; retry.
        }
    }

    // True once the generator has exhausted the 64-bit space.
    bool is_wrapped() const noexcept {
        return wrap_flag_.load(std::memory_order_relaxed);
    }

    // Reset the counter to 0 and clear the wrap flag. Next next_id() returns 1.
    void reset() noexcept {
        counter_.store(0, std::memory_order_relaxed);
        wrap_flag_.store(false, std::memory_order_relaxed);
    }

private:
    std::atomic<uint64_t> counter_{0};
    std::atomic<bool> wrap_flag_{false};
};

}  // namespace kivo::video::audio_plane::runtime::evidence
