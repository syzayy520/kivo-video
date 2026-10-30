#pragma once

#include "kivo/playback/control_plane/recovery/playback_failure.hpp"
#include "kivo/playback/control_plane/recovery/playback_failure_kind.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::recovery {

/// DiagnosticSensitivity enumerates diagnostic string sensitivity levels.
enum class DiagnosticSensitivity
{
    PublicStableCode,
    RedactedUrl,
    HostHash,
    StatusCodeOnly,
    InternalOnly
};

/// FailureClassificationRule maps a kind to severity/recoverable/visible/code.
struct FailureClassificationRule
{
    PlaybackFailureKind kind{PlaybackFailureKind::Unknown};
    FailureSeverity severity{FailureSeverity::Recoverable};
    bool recoverable{true};
    bool user_visible{false};
    primitives::U32 stable_code_prefix{0};
};

/// RuleTableRef is a fixed reference to a classification rule table.
/// Hot path does NOT use std::vector dynamic allocation.
struct RuleTableRef
{
    const FailureClassificationRule* rules{nullptr};
    primitives::U32 count{0};
};

/// FailureTaxonomyPolicy defines failure classification policy.
/// Hot path does NOT use std::vector dynamic allocation.
/// Rule table uses fixed table reference.
struct FailureTaxonomyPolicy
{
    bool expose_network_errors_to_user{false};
    bool classify_tls_failure_as_fatal{false};
    bool classify_drm_failure_as_user_visible{true};
    bool classify_low_speed_as_recoverable{true};
    FailureSeverity default_severity_for_unknown{FailureSeverity::Recoverable};
    RuleTableRef rules{};
};

} // namespace kivo::playback::control_plane::recovery
