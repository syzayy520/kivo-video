// queue_item_id.hpp — GATE-010: P5A queue item id
#pragma once
#include <cstdint>

namespace kivo::video_plane::queue {

struct QueueItemId {
    uint64_t value{0};
    bool operator==(QueueItemId const&) const = default;
};

}  // namespace kivo::video_plane::queue
