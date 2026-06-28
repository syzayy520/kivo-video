// scrub_audio_policy.hpp — P6A rate: scrub audio policy (§10.2)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::rate {
struct ScrubAudioPolicy {
    ContractMetadata metadata;
    bool explicit_command_required{true};
    bool preview_rate_above_4_allowed_only_in_scrub{true};
    bool operator==(const ScrubAudioPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::rate
