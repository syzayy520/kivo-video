// audio_power_state_policy.hpp — P6A threading: power state (§threading)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::threading {

// AudioPowerStatePolicy
// Policy for power state transitions. Compile-only skeleton.
struct AudioPowerStatePolicy {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool invalidate_clock_on_resume{true};
    bool invalidate_qpc_on_resume{true};
    bool drop_stale_by_generation{true};

    bool operator==(const AudioPowerStatePolicy&) const = default;
};

}  // namespace kivo::video::audio_plane::threading
