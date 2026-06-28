// audio_clock_arbitration_policy.hpp — P6A clock: AudioClockArbitrationPolicy
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::clock {
struct AudioClockArbitrationPolicy {
    ContractMetadata metadata;
    bool p4_owns_playback_authority{true};
    bool p6_proposes_clock_evidence{true};
    bool p6_never_directly_reads_p5_video_clock{true};
    bool operator==(const AudioClockArbitrationPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::clock
