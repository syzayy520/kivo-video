// queue_timeout_defaults.hpp — GATE-010: P5A queue timeout defaults (7 constexpr values)
#pragma once
#include <cstdint>

namespace kivo::video_plane::queue {

namespace QueueTimeoutDefaults {
    constexpr uint64_t kEnqueueTimeoutMs = 5000;            // max time to enqueue
    constexpr uint64_t kProcessTimeoutMs = 30000;           // max time to process one item
    constexpr uint64_t kCancelTimeoutMs = 10000;            // max time to cancel
    constexpr uint64_t kDrainTimeoutMs = 15000;             // max time to drain
    constexpr uint64_t kIdleTimeoutMs = 60000;              // idle before standby
    constexpr uint64_t kHeartbeatIntervalMs = 1000;         // heartbeat interval
    constexpr uint64_t kMaxItemAgeMs = 300000;              // max item age in queue (5 min)
}  // namespace QueueTimeoutDefaults

}  // namespace kivo::video_plane::queue
