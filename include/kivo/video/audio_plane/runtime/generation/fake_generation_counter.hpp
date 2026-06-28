// fake_generation_counter.hpp — P6B runtime: fake generation counter with wrap detection
// Header-only fake implementation. Platform-neutral: no WASAPI/FFmpeg/Qt/Win32/COM.
#pragma once

#include <cstdint>
#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"
#include "kivo/video/audio_plane/generation/audio_engine_epoch.hpp"
#include "kivo/video/audio_plane/generation/audio_pipeline_generation.hpp"

namespace kivo::video::audio_plane::runtime::generation {

// Wrap detected when increment would produce 0 (invalid generation).
enum class WrapDetected : int32_t {
    NoWrap = 0,
    WrapToZero = 1,
};

// Fake in-memory generation counter.
// - value 0 is invalid (never returned as a valid generation)
// - increment that would reach 0 reports WrapDetected and stays at max valid
class FakeGenerationCounter {
public:
    FakeGenerationCounter() = default;

    // Returns the next generation. If value+1 == 0, returns WrapDetected
    // and leaves the counter unchanged (caller must trigger epoch reset).
    kivo::Expected<uint64_t, WrapDetected> increment() {
        if (value_ == UINT64_MAX) {
            return kivo::Expected<uint64_t, WrapDetected>{WrapDetected::WrapToZero};
        }
        value_ += 1;
        return kivo::Expected<uint64_t, WrapDetected>{value_};
    }

    // Current generation value. 0 means "never incremented".
    uint64_t current() const noexcept { return value_; }

    // Freshness check: equality only. NO cross-epoch ordering (< or >).
    // A snapshot is fresh iff its epoch and generation match the live values.
    bool is_fresh(
        uint64_t snapshot_epoch,
        uint64_t snapshot_gen,
        uint64_t current_epoch,
        uint64_t current_gen) const noexcept {
        if (snapshot_epoch != current_epoch) {
            return false;
        }
        return snapshot_gen == current_gen;
    }

    // Reset to the initial valid generation (1). 0 is reserved invalid.
    void reset() noexcept { value_ = 1; }

private:
    uint64_t value_{0};
};

}  // namespace kivo::video::audio_plane::runtime::generation
