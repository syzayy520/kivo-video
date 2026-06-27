#pragma once

#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::lifetime {

/// ShutdownQuiescencePolicy defines shutdown quiescence rules.
struct ShutdownQuiescencePolicy
{
    primitives::Milliseconds total_timeout_ms{};
    primitives::Milliseconds destructive_cancel_after_ms{};
    bool allow_quarantine{true};
};

/// ShutdownGuard manages safe shutdown of reader workers.
/// Rules:
/// - ShutdownGuard does NOT accept SessionGeneration.
///   Generation is assigned only by Control Thread via begin_stop_barrier().
class ShutdownGuard
{
public:
    virtual ~ShutdownGuard() = default;

    virtual void request_stop() = 0;
    virtual void force_invalidate_all() = 0;
    virtual void wake_all_waiters() = 0;
    virtual void cancel_all_workers() = 0;
    virtual void quarantine_unresponsive_workers() = 0;
    virtual void destructive_cancel_if_timeout() = 0;
    virtual bool wait_for_quiescence(const ShutdownQuiescencePolicy& policy) = 0;
};

} // namespace kivo::playback::control_plane::lifetime
