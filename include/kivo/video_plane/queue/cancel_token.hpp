// cancel_token.hpp — GATE-010: P5A cancel token
#pragma once
#include <cstdint>

namespace kivo::video_plane::queue {

struct CancelToken {
    uint64_t value{0};
    bool is_valid() const { return value != 0; }
    bool operator==(CancelToken const&) const = default;
};

}  // namespace kivo::video_plane::queue
