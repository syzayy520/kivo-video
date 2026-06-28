// audio_unknown_layout_policy.hpp — P6A channel: unknown layout policy (§8.6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::channel {
struct AudioUnknownLayoutPolicy {
    ContractMetadata metadata;
    bool reject{false};
    bool use_best_effort{true};
    bool emit_evidence{true};
    bool operator==(const AudioUnknownLayoutPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::channel
