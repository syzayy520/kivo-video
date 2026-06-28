// audio_user_sync_offset_policy.hpp — P6A sync: AudioUserSyncOffsetPolicy
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::sync {
struct AudioUserSyncOffsetPolicy {
    ContractMetadata metadata;
    int64_t user_offset_ms{0};
    int64_t min_offset_ms{-500};
    int64_t max_offset_ms{500};
    bool profile_may_allow_wider{true};
    bool offset_changes_emit_evidence{true};
    bool offset_invalidate_sync_snapshot{true};
    bool offset_does_not_modify_media_timestamps{true};
    bool operator==(const AudioUserSyncOffsetPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::sync
