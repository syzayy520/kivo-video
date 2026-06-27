#pragma once

#include <optional>
#include <string>
#include "kivo/playback/control_plane/runtime/runtime_policy_kind.hpp"
#include "kivo/playback/control_plane/revision/revision_master_model.hpp"

namespace kivo::playback::control_plane::runtime {

/// RuntimePolicyPayload must be typed, one-kind-one-payload.
/// Multiple payloads present simultaneously = FAIL.
/// Payload missing = FAIL.
struct RuntimePolicyPayload
{
    RuntimePolicyKind kind{RuntimePolicyKind::ResourceBudget};
    // Each optional corresponds to exactly one kind. Only one may be set.
    std::optional<bool> buffer_requirement_updated{};
    std::optional<bool> read_throttle_updated{};
    std::optional<bool> low_speed_updated{};
    std::optional<bool> reader_stall_updated{};
    std::optional<bool> pause_buffer_updated{};
    std::optional<bool> playback_rate_updated{};
    std::optional<bool> live_stream_updated{};
    std::optional<bool> track_switch_updated{};
    std::optional<bool> idle_timeout_updated{};
    std::optional<bool> adaptive_backpressure_updated{};
    std::optional<bool> resource_budget_updated{};
    std::optional<bool> recovery_plan_updated{};

    bool is_valid() const noexcept
    {
        int set_count = 0;
        if (buffer_requirement_updated) ++set_count;
        if (read_throttle_updated) ++set_count;
        if (low_speed_updated) ++set_count;
        if (reader_stall_updated) ++set_count;
        if (pause_buffer_updated) ++set_count;
        if (playback_rate_updated) ++set_count;
        if (live_stream_updated) ++set_count;
        if (track_switch_updated) ++set_count;
        if (idle_timeout_updated) ++set_count;
        if (adaptive_backpressure_updated) ++set_count;
        if (resource_budget_updated) ++set_count;
        if (recovery_plan_updated) ++set_count;
        return set_count == 1;
    }
};

/// RuntimePolicyUpdate must enter Control Thread.
/// Revision lower than current = reject, record stale update.
/// Update must NOT change provider selection.
/// Update must NOT refresh token/license.
/// Update must NOT directly modify P3 evidence.
struct RuntimePolicyUpdate
{
    revision::P4RuntimePolicyRevision revision{};
    RuntimePolicyKind kind{RuntimePolicyKind::ResourceBudget};
    RuntimePolicyPayload payload{};
};

/// RuntimeRevisionHook defines initial revision and allowed update kinds.
struct RuntimeRevisionHook
{
    revision::P4RuntimePolicyRevision initial_revision{};
    bool allow_runtime_policy_update{false};
    bool allow_resource_budget_update{false};
    bool allow_recovery_plan_update{false};
};

} // namespace kivo::playback::control_plane::runtime
