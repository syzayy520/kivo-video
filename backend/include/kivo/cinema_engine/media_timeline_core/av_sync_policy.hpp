#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// AvSyncPolicy defines how audio and video are synchronized.
struct AvSyncPolicy {
    ContractMetadata metadata;
    std::string policy_id;
    std::string sync_mode;  // "audio_master", "video_master", "external_master"
    double max_drift_ms{40.0};
    double correction_threshold_ms{20.0};
    std::string correction_strategy;  // "skip_frame", "repeat_frame", "adjust_speed"
    bool allow_video_drop{true};
    bool allow_audio_resample{false};
};

}  // namespace kivo::cinema_engine
