#pragma once

#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"
#include "kivo/playback/control_plane/primitives/source_range.hpp"

namespace kivo::playback::control_plane::reader {

/// ReaderReadPayloadKind enumerates read result payload shapes.
/// Sole authority: reader/reader_read_payload_kind.hpp
enum class ReaderReadPayloadKind
{
    DataOnly,
    DataAndEndMarker,
    EndMarkerOnly,
    WouldBlock,
    Cancelled,
    Timeout,
    LowSpeedViolation,
    Stalled,
    Failure
};

} // namespace kivo::playback::control_plane::reader
