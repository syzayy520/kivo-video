#pragma once

#include "kivo/video_plane/stamp/core_video_stamp.hpp"
#include "kivo/video_plane/stamp/full_video_stamp.hpp"
#include "kivo/video_plane/stamp/stamp_validation_context.hpp"
#include "kivo/video_plane/stamp/stamp_validation_result.hpp"

namespace kivo::video_plane::stamp {

struct P5StampValidator {
    // Validates CoreVideoStamp and FullVideoStamp against context
    static StampValidationResult validate_core_stamp(
        CoreVideoStamp stamp,
        StampValidationContext const& context);

    static StampValidationResult validate_full_stamp(
        FullVideoStamp stamp,
        StampValidationContext const& context);

    // Returns true if stamp is monotonically advancing
    static bool is_monotonic_advance(
        CoreVideoStamp previous,
        CoreVideoStamp current);

    // Bootstrap: set initial stamp baseline
    static FullVideoStamp create_bootstrap_stamp(
        uint64_t pipeline_epoch);
};

}  // namespace kivo::video_plane::stamp
