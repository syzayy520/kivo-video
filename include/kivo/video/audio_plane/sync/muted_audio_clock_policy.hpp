// muted_audio_clock_policy.hpp — P6A sync: MutedAudioClockPolicy
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::sync {
struct MutedAudioClockPolicy {
    ContractMetadata metadata;
    bool may_retain_authority_if_pipeline_active{true};
    bool p4_synthetic_timeline_if_pipeline_inactive{true};
    bool operator==(const MutedAudioClockPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::sync
