// audio_pts_wrap_policy.hpp — P6A time: AudioPtsWrapPolicy
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::time {
struct AudioPtsWrapPolicy {
    ContractMetadata metadata;
    bool explicit_wrap_detection_by_timebase{true};
    bool seek_flush_resets_reorder_and_drift{true};
    bool eos_marker_not_valid_sample{true};
    bool operator==(const AudioPtsWrapPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::time
