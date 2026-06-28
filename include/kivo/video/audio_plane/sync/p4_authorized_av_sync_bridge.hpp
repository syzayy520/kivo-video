// p4_authorized_av_sync_bridge.hpp — P6A sync: P4AuthorizedAvSyncBridge
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::sync {
struct P4AuthorizedAvSyncBridge {
    ContractMetadata metadata;
    uint64_t p4_authorization_id{0};
    bool sync_intervention_authorized{false};
    bool operator==(const P4AuthorizedAvSyncBridge&) const = default;
};
}  // namespace kivo::video::audio_plane::sync
