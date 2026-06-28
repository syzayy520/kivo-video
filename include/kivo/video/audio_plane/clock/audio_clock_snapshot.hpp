// audio_clock_snapshot.hpp — P6A clock: AudioClockSnapshot
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/clock/audio_clock_candidate.hpp"
namespace kivo::video::audio_plane::clock {
struct AudioClockSnapshot {
    ContractMetadata metadata;
    AudioClockCandidate candidate{AudioClockCandidate::MonotonicFallback};
    int64_t estimate_ms{0};
    int64_t error_bound_ms{0};
    uint64_t engine_epoch{0};
    uint64_t device_generation{0};
    bool is_valid{false};
    bool operator==(const AudioClockSnapshot&) const = default;
};
}  // namespace kivo::video::audio_plane::clock
