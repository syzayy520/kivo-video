// audio_absent_clock_policy.hpp — P6A sync: AudioAbsentClockPolicy
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::sync {
struct AudioAbsentClockPolicy {
    ContractMetadata metadata;
    bool p4_synthetic_timeline_owns_authority{true};
    bool p6_has_no_clock_authority{true};
    bool operator==(const AudioAbsentClockPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::sync
