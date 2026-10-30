#pragma once

#include <cstdint>
#include <string>

namespace kivo::p3::execution {

/// CancellationState enumerates cancellation propagation states.
enum class CancellationState
{
    none,
    requested,
    propagating,
    completed
};

/// CancellationSignal propagates cancellation across P3 long-running operations.
/// Every P3 long-running operation must be cancellable.
/// Cancellation must not be mapped to timeout.
/// schema_version: p3.execution.cancellation.v1
struct CancellationSignal
{
    std::string schema_version{"p3.execution.cancellation.v1"};
    CancellationState state{CancellationState::none};
    int64_t requested_at_ns{0};

    bool is_cancelled() const noexcept { return state != CancellationState::none; }
    bool is_requested() const noexcept { return state == CancellationState::requested; }
    bool is_completed() const noexcept { return state == CancellationState::completed; }
    void request(int64_t at_ns) noexcept { state = CancellationState::requested; requested_at_ns = at_ns; }
    void complete() noexcept { state = CancellationState::completed; }
};

} // namespace kivo::p3::execution
