// reverse_scrub_policy.hpp — P6A rate: reverse scrub policy (§10.3)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::rate {
struct ReverseScrubPolicy {
    ContractMetadata metadata;
    bool baseline_is_muted_preview{true};
    bool no_reverse_decode_in_baseline{true};
    bool operator==(const ReverseScrubPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::rate
