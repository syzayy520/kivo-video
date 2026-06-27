#pragma once

#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::diagnostics {

/// StableReasonCode is a hot-path stable reason code.
/// Hot path uses StableReasonCode, NOT std::string.
struct StableReasonCode
{
    primitives::U32 value{0};
};

/// StableErrorCode is a hot-path stable error code.
struct StableErrorCode
{
    primitives::U32 value{0};
};

} // namespace kivo::playback::control_plane::diagnostics
