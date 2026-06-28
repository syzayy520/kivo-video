// av_sync_threshold.hpp — P6A sync: AvSyncThreshold (value type)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::sync {
struct AvSyncThreshold {
    int64_t early_ms{50};
    int64_t late_ms{50};
    bool operator==(const AvSyncThreshold&) const = default;
};
}  // namespace kivo::video::audio_plane::sync
