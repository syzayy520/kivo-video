#pragma once

#include <cstdint>
#include <string>

namespace kivo::p3::execution {

/// EventOverflowPolicy defines event loss behavior on backpressure.
enum class EventOverflowPolicy
{
    coalesce,
    fail_on_overflow,
    must_not_drop
};

/// EventStreamBackpressure defines event loss behavior.
/// Critical events must not silently drop. SessionErrorSignal must not silently drop.
/// schema_version: p3.execution.event_stream.v1
struct EventStreamBackpressure
{
    std::string schema_version{"p3.execution.event_stream.v1"};
    EventOverflowPolicy progress_policy{EventOverflowPolicy::coalesce};
    EventOverflowPolicy critical_policy{EventOverflowPolicy::fail_on_overflow};
    EventOverflowPolicy session_error_policy{EventOverflowPolicy::must_not_drop};
    int32_t max_buffered{1024};
    int32_t current_buffered{0};

    bool has_backpressure() const noexcept { return current_buffered >= max_buffered; }
    bool critical_must_not_drop() const noexcept { return session_error_policy == EventOverflowPolicy::must_not_drop; }
};

} // namespace kivo::p3::execution
