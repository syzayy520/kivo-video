#pragma once

#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::diagnostics {

/// DiagnosticSensitivity enumerates diagnostic string sensitivity levels.
enum class DiagnosticSensitivity
{
    PublicStableCode,
    RedactedUrl,
    HostHash,
    StatusCodeOnly,
    InternalOnly
};

/// SafeDiagnosticString is a cold-path diagnostic string with redaction.
/// Cold path only. Hot path uses StableReasonCode/StableErrorCode.
struct SafeDiagnosticString
{
    const char* value{nullptr}; // cold path table-lookup string
    primitives::U32 length{0};
    DiagnosticSensitivity sensitivity{DiagnosticSensitivity::InternalOnly};
    bool redacted{false};
};

} // namespace kivo::playback::control_plane::diagnostics
