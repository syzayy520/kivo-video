// av_sync_threshold_profile.hpp — P6A sync: AvSyncThresholdProfile
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/sync/av_sync_threshold.hpp"
namespace kivo::video::audio_plane::sync {
struct AvSyncThresholdProfile {
    ContractMetadata metadata;
    AvSyncThreshold normal;
    AvSyncThreshold live;
    AvSyncThreshold low_latency;
    bool operator==(const AvSyncThresholdProfile&) const = default;
};
}  // namespace kivo::video::audio_plane::sync
