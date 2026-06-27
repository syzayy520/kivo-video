#pragma once

#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::runtime {

/// ResourceBudgetEventKind enumerates resource budget event kinds.
enum class ResourceBudgetEventKind
{
    Updated,
    ShrinkStarted,
    ShrinkBlocked,
    ShrinkCompleted
};

/// ResourceBudget defines all resource max limits.
/// ResourceBudget must NOT lack any max limit field.
struct ResourceBudget
{
    primitives::Count max_inflight_read_count{};
    primitives::Count max_inflight_seek_count{};
    primitives::Count max_reader_instance_count{};
    primitives::Bytes max_inflight_read_bytes{};
    primitives::Bytes max_buffer_bytes{};
    primitives::Bytes max_memory_pool_bytes{};
    primitives::Milliseconds max_command_queue_delay_ms{};
    primitives::Count max_control_event_backlog{};
    primitives::Milliseconds max_adaptive_window_ms{};
    primitives::Bytes max_adaptive_window_bytes{};
};

/// ResourceBudgetDiagnostics tracks current vs effective budget.
struct ResourceBudgetDiagnostics
{
    ResourceBudget current_resource_budget{};
    ResourceBudget effective_resource_budget{};
    bool resource_budget_shrink_pending{false};
};

} // namespace kivo::playback::control_plane::runtime
