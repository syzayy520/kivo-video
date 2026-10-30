#pragma once

#include "kivo/playback/control_plane/recovery/recovery_context.hpp"
#include "kivo/playback/control_plane/recovery/playback_failure_kind.hpp"

namespace kivo::playback::control_plane::recovery {

/// DrmFailurePolicy defines DRM/license downstream failure path.
/// Rules:
/// - P4 does NOT acquire license.
/// - P4 does NOT decrypt.
/// - P4 stops scheduling new read if policy requires.
/// - P4 publishes ErrorRaised.
/// - RecoveryDecision default = HardTerminateToUpperLayer.
/// - If RecoveryPlan has authorized LicenseRefreshed fallback, P4 may switch fallback.
///
/// P4 must NOT contain:
/// - license_request, license_server_call, decrypt_sample, DRM key material
struct DrmFailurePolicy
{
    bool stop_scheduling_new_read{true};
    bool publish_error_raised{true};
    RecoveryDecisionKind default_recovery_decision{RecoveryDecisionKind::HardTerminateToUpperLayer};
    bool allow_license_refreshed_fallback{false};
};

} // namespace kivo::playback::control_plane::recovery
