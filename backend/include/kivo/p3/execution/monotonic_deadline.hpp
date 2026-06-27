#pragma once

#include <cstdint>

namespace kivo::p3::execution {

/// MonotonicDeadline enforces monotonic time for all timeout decisions.
/// Wall clock is only for evidence display, never for control.
/// schema_version: p3.execution.monotonic_deadline.v1
struct MonotonicDeadline
{
    std::string schema_version{"p3.execution.monotonic_deadline.v1"};
    int64_t deadline_monotonic_ns{0};
    int64_t now_monotonic_ns{0};

    bool is_expired() const noexcept { return now_monotonic_ns >= deadline_monotonic_ns; }
    int64_t remaining_ns() const noexcept { return deadline_monotonic_ns - now_monotonic_ns; }
    bool has_remaining() const noexcept { return remaining_ns() > 0; }
};

} // namespace kivo::p3::execution
