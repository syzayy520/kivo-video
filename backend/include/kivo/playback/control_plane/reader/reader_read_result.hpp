#pragma once

#include <optional>
#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/reader/reader_read_payload_kind.hpp"
#include "kivo/playback/control_plane/buffer/stream_end_kind.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"
#include "kivo/playback/control_plane/primitives/source_range.hpp"

namespace kivo::playback::control_plane::reader {

/// ReaderDataPayload carries read bytes and offset/timing metadata.
struct ReaderDataPayload
{
    primitives::SourceOffset start_offset{};
    primitives::Bytes bytes_read{};
    const void* memory{nullptr}; // ReadOnlyMemoryChain reference (filled by impl)
    std::optional<primitives::SourceRange> estimated_time_range{};
};

/// StreamEndMarker marks stream end with kind and projection.
struct StreamEndMarker
{
    buffer::StreamEndKind kind{buffer::StreamEndKind::None};
    revision::RevisionStamp stamp{};
    std::optional<primitives::SourceOffset> final_offset{};
    std::optional<primitives::MediaTimeMs> projected_position_ms{};
    primitives::MonotonicTimestamp observed_at{};
};

/// ReaderFailure describes a reader-side failure.
struct ReaderFailure
{
    primitives::U32 stable_code{0};
    bool recoverable{false};
};

/// ReaderReadResult is the reader read result with Data+EndMarker legal matrix.
/// Legal combinations (enforced by ReaderReadResult::is_legal):
/// - DataOnly: data required, end_marker forbidden, failure forbidden.
/// - DataAndEndMarker: data required, end_marker required, failure forbidden.
/// - EndMarkerOnly: data forbidden, end_marker required, failure forbidden.
/// - WouldBlock: all forbidden.
/// - Cancelled: data forbidden, end_marker optional (kind=Cancelled), failure optional.
/// - Timeout: failure required, others forbidden.
/// - LowSpeedViolation: failure required, others forbidden.
/// - Stalled: failure required, others forbidden.
/// - Failure: failure required, others forbidden.
struct ReaderReadResult
{
    revision::RevisionStamp stamp{};
    ReaderReadPayloadKind payload_kind{ReaderReadPayloadKind::WouldBlock};
    std::optional<ReaderDataPayload> data{};
    std::optional<StreamEndMarker> end_marker{};
    std::optional<ReaderFailure> failure{};

    constexpr bool is_legal() const noexcept
    {
        switch (payload_kind)
        {
        case ReaderReadPayloadKind::DataOnly:
            return data.has_value() && !end_marker.has_value() && !failure.has_value();
        case ReaderReadPayloadKind::DataAndEndMarker:
            return data.has_value() && end_marker.has_value() && !failure.has_value();
        case ReaderReadPayloadKind::EndMarkerOnly:
            return !data.has_value() && end_marker.has_value() && !failure.has_value();
        case ReaderReadPayloadKind::WouldBlock:
            return !data.has_value() && !end_marker.has_value() && !failure.has_value();
        case ReaderReadPayloadKind::Cancelled:
            return !data.has_value()
                && (!end_marker.has_value() || end_marker->kind == buffer::StreamEndKind::Cancelled);
        case ReaderReadPayloadKind::Timeout:
        case ReaderReadPayloadKind::LowSpeedViolation:
        case ReaderReadPayloadKind::Stalled:
        case ReaderReadPayloadKind::Failure:
            return failure.has_value() && !data.has_value() && !end_marker.has_value();
        }
        return false;
    }
};

} // namespace kivo::playback::control_plane::reader
