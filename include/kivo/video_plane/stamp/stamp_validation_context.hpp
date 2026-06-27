#pragma once

#include "kivo/video_plane/stamp/core_video_stamp.hpp"
#include "kivo/video_plane/stamp/full_video_stamp.hpp"

namespace kivo::video_plane::stamp {

struct StampValidationContext {
    FullVideoStamp last_known_full_stamp;
    CoreVideoStamp current_core_stamp;
    uint64_t expected_pipeline_epoch{0};
    bool is_recovery_seek{false};
    bool is_bootstrap{false};
    bool operator==(StampValidationContext const&) const = default;
};

}  // namespace kivo::video_plane::stamp
