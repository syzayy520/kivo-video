#pragma once

#include <array>
#include "kivo/playback/control_plane/diagnostics/stable_reason_code.hpp"
#include "kivo/playback/control_plane/diagnostics/fixed_window_histogram.hpp"
#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/revision/snapshot_stamp.hpp"
#include "kivo/playback/control_plane/buffer/buffer_snapshot.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::diagnostics {

/// ResourceBudgetSnapshot is an opaque resource budget snapshot reference.
struct ResourceBudgetSnapshot
{
    primitives::Bytes max_buffer_bytes{};
    primitives::Bytes max_memory_pool_bytes{};
};

/// SessionDiagnostics is the full session diagnostics snapshot.
/// Hot path uses StableReasonCode/StableErrorCode, NOT std::string.
struct SessionDiagnostics
{
    revision::SessionId session_id{};
    primitives::Bytes total_bytes_read{};
    primitives::Bytes total_bytes_delivered{};
    primitives::Bytes total_bytes_discarded_as_stale{};
    primitives::BytesPerSecond current_read_bytes_per_second{};
    primitives::BytesPerSecond current_drain_bytes_per_second{};
    // Histograms (no heap)
    FixedWindowHistogram<16, 4> read_latency_ms{};
    FixedWindowHistogram<16, 4> pull_wait_ms{};
    FixedWindowHistogram<16, 4> recovery_duration_ms{};
    // Counters
    primitives::U32 seek_count{0};
    primitives::U32 seek_superseded_count{0};
    primitives::U32 stale_read_discard_count{0};
    primitives::U32 stale_callback_discard_count{0};
    primitives::U32 stale_buffer_push_reject_count{0};
    primitives::U32 cancel_requested_count{0};
    primitives::U32 cancel_success_count{0};
    primitives::U32 destructive_cancel_count{0};
    primitives::U32 buffering_enter_count{0};
    primitives::U32 buffering_churn_count{0};
    primitives::U32 recovery_count{0};
    primitives::U32 fallback_count{0};
    primitives::U32 low_speed_violation_count{0};
    std::array<primitives::U32, 8> recovery_budget_counters{};
    primitives::U32 active_reader_instance_count{0};
    primitives::U32 active_inflight_read_count{0};
    primitives::Milliseconds reader_last_byte_received_age_ms{};
    primitives::Milliseconds reader_last_progress_age_ms{};
    bool reader_stalled{false};
    bool read_expected_to_progress{false};
    primitives::U32 command_queue_depth{0};
    primitives::U32 control_event_queue_depth{0};
    primitives::U32 critical_event_overflow_count{0};
    primitives::U32 memory_pressure{0}; // 0-100
    buffer::BufferSnapshot buffer_snapshot{};
    ResourceBudgetSnapshot resource_budget{};
    revision::SnapshotStamp snapshot_stamp{};
};

} // namespace kivo::playback::control_plane::diagnostics
