#pragma once

#include <cstdint>

namespace kivo::video_plane::stamp {

struct CoreVideoStamp {
    uint64_t value{0};  // monotonic, P5-assigned at pipeline ingest
    bool operator==(CoreVideoStamp const&) const = default;
};

}  // namespace kivo::video_plane::stamp
