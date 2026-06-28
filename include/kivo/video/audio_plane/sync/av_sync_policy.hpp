// av_sync_policy.hpp — P6A sync: AvSyncPolicy
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::sync {
struct AvSyncPolicy {
    ContractMetadata metadata;
    bool p4_owns_authority{true};
    bool p6_proposes_evidence{true};
    bool p4_decides{true};
    bool operator==(const AvSyncPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::sync
