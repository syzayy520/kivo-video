// audio_clock_candidate_mapping.hpp — P6A clock: AudioClockCandidateMapping
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/clock/audio_clock_candidate.hpp"
namespace kivo::video::audio_plane::clock {
struct AudioClockCandidateMapping {
    ContractMetadata metadata;
    AudioClockCandidate candidate{AudioClockCandidate::MonotonicFallback};
    int32_t backend_clock_source{0};
    bool is_authority{false};
    bool operator==(const AudioClockCandidateMapping&) const = default;
};
}  // namespace kivo::video::audio_plane::clock
