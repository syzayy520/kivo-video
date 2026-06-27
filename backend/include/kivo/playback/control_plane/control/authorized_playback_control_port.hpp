#pragma once

#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/runtime/runtime_policy_update.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::control {

/// TrackSwitchPlan is an authorized track-switch plan.
/// Only P3/P5 can authorize a plan; P8 must NOT construct this.
/// P4 does NOT reverse-call P3 for provider selection.
struct TrackSwitchPlan
{
    primitives::U64 plan_id{0};
    bool authorized_by_p3{false};
    bool authorized_by_p5{false};
    primitives::U64 target_track_hint{0};
};

/// AuthorizedPlaybackControlPort is the sole entry point for authorized
/// track-switch and runtime-policy updates.
///
/// Rules:
/// - P8 does NOT directly hold this port.
/// - Only P3/P5 authorized plans can enter this port.
/// - P4 does NOT reverse-call P3 for provider selection.
/// - Port calls must enter ControlThread serialization.
class AuthorizedPlaybackControlPort
{
public:
    virtual ~AuthorizedPlaybackControlPort() = default;

    /// Executes an authorized track-switch plan.
    /// Returns false if the plan lacks P3/P5 authorization.
    virtual bool execute_track_switch(const TrackSwitchPlan& plan) = 0;

    /// Applies a runtime policy update.
    /// Returns false if the update is stale or unauthorized.
    virtual bool apply_runtime_policy_update(
        const runtime::RuntimePolicyUpdate& update) = 0;
};

} // namespace kivo::playback::control_plane::control
