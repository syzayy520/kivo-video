// dialog_enhancement_policy.hpp — P6A mix: dialog enhancement policy (§9.4)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::mix {
struct DialogEnhancementPolicy {
    ContractMetadata metadata;
    bool enabled{false};
    int32_t gain_db{0};
    bool operator==(const DialogEnhancementPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::mix
