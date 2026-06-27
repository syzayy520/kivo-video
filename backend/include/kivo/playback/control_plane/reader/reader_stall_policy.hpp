#pragma once

#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"

namespace kivo::playback::control_plane::reader {

/// ReaderStallPolicy defines reader stall detection thresholds.
/// Rule:
/// - ReaderStall detection only active when: active read in-flight OR
///   ReaderWorker executing blocking read OR downstream_starved AND
///   read_expected_to_progress.
struct ReaderStallPolicy
{
    bool enable_reader_stall_detection{true};
    primitives::Milliseconds max_no_byte_age_ms{};
    primitives::Milliseconds max_no_progress_age_ms{};
    bool escalate_to_recovery{false};
};

} // namespace kivo::playback::control_plane::reader
