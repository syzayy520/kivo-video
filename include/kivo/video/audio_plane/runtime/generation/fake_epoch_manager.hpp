// fake_epoch_manager.hpp — P6B runtime: fake engine epoch manager
// Header-only fake implementation. Platform-neutral.
#pragma once

#include <cstdint>
#include "kivo/video/audio_plane/generation/audio_engine_epoch.hpp"
#include "kivo/video/audio_plane/generation/audio_pipeline_generation.hpp"

namespace kivo::video::audio_plane::runtime::generation {

// Staleness result for cross-epoch generation checks.
enum class StalenessResult : int32_t {
    Fresh = 0,       // snapshot_epoch == current_epoch AND snapshot_gen == current_gen
    Stale = 1,       // snapshot_epoch == current_epoch AND snapshot_gen != current_gen
    CrossEpoch = 2,  // snapshot_epoch != current_epoch (no ordering assumed)
};

// Fake in-memory engine epoch manager.
// - epoch starts at 1 (0 is invalid per AudioEngineEpoch)
// - reset_epoch() increments the epoch and conceptually resets all generation
//   counters (callers are responsible for resetting their own counters)
class FakeEpochManager {
public:
    FakeEpochManager() = default;

    // Current engine epoch. value is never 0 after construction.
    kivo::video::audio_plane::generation::AudioEngineEpoch
    current_epoch() const noexcept {
        kivo::video::audio_plane::generation::AudioEngineEpoch e;
        e.value = epoch_;
        return e;
    }

    // Increment the epoch and signal callers to reset generation counters.
    // Returns the new epoch. Epoch 0 is never produced (overflow stays at max).
    kivo::video::audio_plane::generation::AudioEngineEpoch
    reset_epoch() noexcept {
        if (epoch_ != UINT64_MAX) {
            epoch_ += 1;
        }
        kivo::video::audio_plane::generation::AudioEngineEpoch e;
        e.value = epoch_;
        return e;
    }

    // Staleness check comparing a snapshot's epoch/generation to the live
    // epoch/generation. Equality-only: no cross-epoch < or > comparison.
    StalenessResult staleness_check(
        uint64_t snapshot_epoch,
        uint64_t snapshot_gen,
        uint64_t current_gen) const noexcept {
        if (snapshot_epoch != epoch_) {
            return StalenessResult::CrossEpoch;
        }
        if (snapshot_gen == current_gen) {
            return StalenessResult::Fresh;
        }
        return StalenessResult::Stale;
    }

private:
    uint64_t epoch_{1};
};

}  // namespace kivo::video::audio_plane::runtime::generation
