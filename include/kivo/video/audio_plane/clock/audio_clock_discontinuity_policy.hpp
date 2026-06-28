// audio_clock_discontinuity_policy.hpp — P6A clock: AudioClockDiscontinuityPolicy
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::clock {
struct AudioClockDiscontinuityPolicy {
    ContractMetadata metadata;
    bool device_position_rollback_invalidate{true};
    bool device_position_stall_beyond_3_periods_invalidate{true};
    bool device_position_jump_invalidate{true};
    bool qpc_mapping_invalid_invalidate{true};
    bool device_generation_change_invalidate{true};
    bool operator==(const AudioClockDiscontinuityPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::clock
