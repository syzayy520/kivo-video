// audio_dual_mono_policy.hpp — P6A channel: dual-mono policy (§8.7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::channel {
struct AudioDualMonoPolicy {
    ContractMetadata metadata;
    bool detect{true};
    bool duplicate_to_stereo{true};
    bool operator==(const AudioDualMonoPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::channel
