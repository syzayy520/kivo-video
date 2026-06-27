// queue_item.hpp — GATE-010: P5A queue item (strong types only)
#pragma once
#include <cstdint>
#include <optional>
#include "queue_item_id.hpp"
#include "video_pipeline_id.hpp"
#include "cancel_token.hpp"
#include "queue_lifecycle_state.hpp"
#include "../stamp/core_video_stamp.hpp"
#include "../failure/failure_correlation_id.hpp"

namespace kivo::video_plane::queue {

using kivo::video_plane::stamp::CoreVideoStamp;
using kivo::video_plane::failure::FailureCorrelationId;

struct QueueItem {
    QueueItemId queue_item_id;
    VideoPipelineId pipeline_id;
    CoreVideoStamp core_video_stamp;        // strong type, NOT uint64_t
    CancelToken cancel_token;               // strong type, NOT std::string
    std::optional<FailureCorrelationId> failure_correlation_id;
    QueueLifecycleState lifecycle_state{QueueLifecycleState::Accepting};
    uint64_t enqueue_time{0};
    uint64_t last_progress_time{0};

    bool operator==(QueueItem const&) const = default;
};

}  // namespace kivo::video_plane::queue
