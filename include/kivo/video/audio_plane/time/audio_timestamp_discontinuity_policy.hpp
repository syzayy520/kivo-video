// audio_timestamp_discontinuity_policy.hpp — P6A time: AudioTimestampDiscontinuityPolicy
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::time {
struct AudioTimestampDiscontinuityPolicy {
    ContractMetadata metadata;
    bool backward_timestamp_with_monotonic_seq_requires_evidence{true};
    bool seek_resets_reorder_window{true};
    bool flush_resets_drift_window{true};
    bool operator==(const AudioTimestampDiscontinuityPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::time
