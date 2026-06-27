#pragma once

#include "kivo/video_plane/stamp/core_video_stamp.hpp"

namespace kivo::video_plane::stamp {

struct FullVideoStamp {
    uint64_t pipeline_epoch{0};   // P4 session revision
    uint64_t input_sequence{0};   // P5 input bridge increment
    CoreVideoStamp core_stamp;    // P5 core stamp (subordinate)
    bool operator==(FullVideoStamp const&) const = default;
};

}  // namespace kivo::video_plane::stamp
