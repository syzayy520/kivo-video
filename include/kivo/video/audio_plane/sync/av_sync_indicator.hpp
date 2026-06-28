// av_sync_indicator.hpp — P6A sync: AvSyncIndicator
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::sync {
enum class AvSyncState {
    InSync = 0,
    AudioEarly = 1,
    AudioLate = 2,
    Unknown = 3,
};
struct AvSyncIndicator {
    ContractMetadata metadata;
    AvSyncState state{AvSyncState::Unknown};
    int64_t drift_ms{0};
    int32_t confidence{0};
    bool operator==(const AvSyncIndicator&) const = default;
};
}  // namespace kivo::video::audio_plane::sync
