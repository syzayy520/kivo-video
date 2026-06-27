#pragma once

#include <optional>
#include "kivo/playback/control_plane/recovery/playback_failure_kind.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::recovery {

/// FailureOrigin enumerates failure origin layers.
enum class FailureOrigin
{
    Reader,
    Buffer,
    Pipeline,
    Downstream,
    ControlThread,
    MemoryPool,
    RuntimePolicy
};

/// FailureSeverity enumerates failure severity levels.
enum class FailureSeverity
{
    Info,
    Recoverable,
    UserVisible,
    Fatal
};

/// SafeDiagnosticString is a cold-path diagnostic string with redaction.
struct SafeDiagnosticString
{
    // value + sensitivity stored as opaque; cold path only.
    primitives::U32 sensitivity{0}; // DiagnosticSensitivity value
    bool redacted{false};
};

/// PlaybackFailure is the canonical failure record.
struct PlaybackFailure
{
    PlaybackFailureKind kind{PlaybackFailureKind::Unknown};
    FailureSeverity severity{FailureSeverity::Recoverable};
    FailureOrigin origin{FailureOrigin::Reader};
    primitives::U32 stable_code{0}; // StableErrorCode
    bool recoverable{true};
    bool user_visible{false};
    std::optional<primitives::Milliseconds> retry_after_ms{};
    std::optional<SafeDiagnosticString> diagnostic{};
};

} // namespace kivo::playback::control_plane::recovery
