// audio_drc_policy.hpp — P6A mix: dynamic range compression (DRC) policy (§9.5)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::mix {
struct AudioDrcPolicy {
    ContractMetadata metadata;
    bool enabled{false};
    int32_t metadata_source{0};
    bool applying_breaks_bit_perfect{true};
    bool passthrough_no_pcm_drc{true};
    bool operator==(const AudioDrcPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::mix
