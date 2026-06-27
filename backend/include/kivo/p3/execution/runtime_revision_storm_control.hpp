#pragma once

#include <cstdint>
#include <string>

namespace kivo::p3::execution {

/// RuntimeRevisionTrigger enumerates revision triggers (priority order).
enum class RuntimeRevisionTrigger
{
    signed_url_expired,
    provider_session_lost,
    probe_downgrade,
    constraint_violation,
    manifest_updated,
    metadata_changed,
    network_changed,
    user_preference,
    session_error
};

/// RuntimeRevisionAction enumerates allowed revision actions.
enum class RuntimeRevisionAction
{
    keep,
    replace,
    downgrade,
    request_processing,
    retry,
    not_playable,
    refresh,
    rerank,
    update_candidates,
    p4_action_hint
};

/// RuntimeRevisionStormControl throttles and prioritizes runtime revisions.
/// max_revision_per_minute, min interval, coalesce window, priority order.
/// schema_version: p3.execution.runtime_revision.v1
struct RuntimeRevisionStormControl
{
    std::string schema_version{"p3.execution.runtime_revision.v1"};
    int32_t max_revision_per_minute{10};
    int64_t min_interval_ns{1'000'000'000};
    int64_t coalesce_window_ns{500'000'000};
    int32_t revisions_this_minute{0};
    int64_t last_revision_ns{0};
    bool rate_limit_evidence_emitted{false};

    bool can_revise(int64_t now_ns) const noexcept
    {
        if (revisions_this_minute >= max_revision_per_minute) return false;
        return (now_ns - last_revision_ns) >= min_interval_ns;
    }
    bool is_throttled(int64_t now_ns) const noexcept
    {
        return !can_revise(now_ns);
    }
};

} // namespace kivo::p3::execution
