#pragma once

#include <cstdint>

namespace kivo::video::playback_graph {

enum class SnapshotQueryError {
    None,
    AttachmentNotCreated,
    AttachmentClosed,
    DeviceLost,
    ReadUnstable,
    Unsupported,
    StaleGeneration,
    Timeout
};

struct AudioQueueSnapshot {
    std::uint32_t queued_duration_ms{0};
    std::uint32_t queued_bytes{0};
    bool selected{false};
};

struct VideoQueueSnapshot {
    std::uint32_t queued_frames{0};
    std::uint32_t queued_duration_ms{0};
    bool selected{false};
};

struct ClockSnapshot {
    std::int64_t estimate_ms{0};
    std::int64_t error_bound_ms{0};
    bool valid{false};
};

template <typename T>
struct SnapshotQueryResult {
    T value{};
    SnapshotQueryError error{SnapshotQueryError::None};

    [[nodiscard]] bool ok() const noexcept {
        return error == SnapshotQueryError::None;
    }
};

}  // namespace kivo::video::playback_graph
