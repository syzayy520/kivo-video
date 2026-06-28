// audio_clock_conflict_evidence.hpp — P6A clock: AudioClockConflictEvidence
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/clock/audio_clock_candidate.hpp"
namespace kivo::video::audio_plane::clock {
struct AudioClockConflictEvidence {
    ContractMetadata metadata;
    uint64_t evidence_id{0};
    AudioClockCandidate candidate_a{AudioClockCandidate::MonotonicFallback};
    AudioClockCandidate candidate_b{AudioClockCandidate::MonotonicFallback};
    int64_t estimate_a_ms{0};
    int64_t estimate_b_ms{0};
    bool intervals_overlap{false};
    bool operator==(const AudioClockConflictEvidence&) const = default;
};
}  // namespace kivo::video::audio_plane::clock
