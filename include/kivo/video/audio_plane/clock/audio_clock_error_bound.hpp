// audio_clock_error_bound.hpp — P6A clock: AudioClockErrorBound
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::clock {
struct AudioClockErrorBound {
    ContractMetadata metadata;
    int64_t bound_ms{0};
    bool is_interval_not_additive_bias{true};
    bool conservative_combination{true};
    bool operator==(const AudioClockErrorBound&) const = default;
};
}  // namespace kivo::video::audio_plane::clock
