#pragma once

#include <cstdint>

namespace kivo::video::playback_graph {

enum class QueueBudgetDimensionMode {
    Disabled,
    Bounded,
    UnlimitedForTestOnly
};

struct VideoQueueBudgetPolicy {
    QueueBudgetDimensionMode frames_mode{QueueBudgetDimensionMode::Bounded};
    std::uint32_t frames_limit{3};
    QueueBudgetDimensionMode bytes_mode{QueueBudgetDimensionMode::Disabled};
    std::uint64_t bytes_limit{0};
    QueueBudgetDimensionMode duration_mode{QueueBudgetDimensionMode::Disabled};
    std::uint32_t duration_ms_limit{0};
};

struct AudioQueueBudgetPolicy {
    QueueBudgetDimensionMode bytes_mode{QueueBudgetDimensionMode::Bounded};
    std::uint64_t bytes_limit{262144};
    QueueBudgetDimensionMode duration_mode{QueueBudgetDimensionMode::Bounded};
    std::uint32_t duration_ms_limit{250};
};

struct GraphTuningParameters {
    std::uint32_t buffering_timeout_ms{5000};
    std::uint32_t rebuffering_timeout_ms{5000};
    std::uint32_t stalled_timeout_ms{3000};
    std::uint32_t source_seek_timeout_ms{3000};
    std::uint32_t demux_seek_timeout_ms{3000};
    std::uint32_t audio_flush_timeout_ms{1000};
    std::uint32_t video_flush_timeout_ms{1000};
    std::uint32_t seek_timeout_ms{8000};
    std::uint32_t drain_timeout_ms{5000};
    std::uint32_t last_frame_presented_timeout_ms{1000};
    std::uint32_t output_drained_timeout_ms{1000};
    std::uint32_t source_resume_verify_timeout_ms{1000};
    std::uint32_t source_reconnect_timeout_ms{3000};
    std::uint32_t source_reopen_timeout_ms{3000};
    std::uint32_t attachment_completion_timeout_ms{2000};
    std::uint32_t close_timeout_ms{3000};
    std::uint32_t physical_retire_enqueue_timeout_ms{1000};
    std::uint32_t physical_recycle_timeout_ms{3000};
    std::uint32_t seqlock_max_attempt_count{3};
};

struct PlaybackGraphPolicy {
    VideoQueueBudgetPolicy video_queue_budget{};
    AudioQueueBudgetPolicy audio_queue_budget{};
    GraphTuningParameters tuning{};
    bool allow_closed_subscription{true};
};

}  // namespace kivo::video::playback_graph
