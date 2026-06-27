#pragma once

#include <optional>
#include "kivo/playback/control_plane/contracts/stream_pull_status.hpp"
#include "kivo/playback/control_plane/revision/revision_master_model.hpp"
#include "kivo/playback/control_plane/revision/revision_stamp.hpp"
#include "kivo/playback/control_plane/memory/read_only_memory_chain.hpp"
#include "kivo/playback/control_plane/buffer/buffer_item.hpp"
#include "kivo/playback/control_plane/buffer/stream_end_kind.hpp"
#include "kivo/playback/control_plane/primitives/time_types.hpp"
#include "kivo/playback/control_plane/primitives/value_types.hpp"

namespace kivo::playback::control_plane::contracts {

/// StreamPullFailure describes a pull-side failure.
struct StreamPullFailure
{
    primitives::U32 stable_code{0};
    bool recoverable{false};
};

/// StreamPullResult is the result of IMediaStreamPipeline::pull with legal matrix.
/// Legal combinations (enforced by is_legal):
/// - Data: chunk required, wait_hint optional, others forbidden.
/// - WouldBlock: wait_hint required, others forbidden.
/// - EndOfStream: end_marker required (kind=EndOfFile), others forbidden.
/// - LiveEdgeReached: end_marker required (kind=LiveEdgeReached), wait_hint optional, others forbidden.
/// - Discontinuity: discontinuity required, wait_hint optional, others forbidden.
/// - Fenced: wait_hint/discontinuity optional, others forbidden.
/// - Flushed: wait_hint/discontinuity optional, others forbidden.
/// - Cancelled: end_marker optional (kind=Cancelled), failure optional, others forbidden.
/// - Terminated: end_marker optional (kind=Stopped), failure optional, others forbidden.
/// - Error: failure required, wait_hint optional, others forbidden.
struct StreamPullResult
{
    StreamPullStatus status{StreamPullStatus::WouldBlock};
    std::optional<memory::ReadOnlyMemoryChain> chunk{};
    std::optional<primitives::Milliseconds> wait_hint{};
    std::optional<buffer::BufferDiscontinuity> discontinuity{};
    std::optional<buffer::BufferEndMarker> end_marker{};
    revision::StreamRevision stream_revision{};
    revision::TimelineRevision timeline_revision{};
    std::optional<StreamPullFailure> failure{};

    constexpr bool is_legal() const noexcept
    {
        switch (status)
        {
        case StreamPullStatus::Data:
            return chunk.has_value() && !end_marker.has_value() && !discontinuity.has_value() && !failure.has_value();
        case StreamPullStatus::WouldBlock:
            return wait_hint.has_value() && !chunk.has_value() && !end_marker.has_value() && !discontinuity.has_value() && !failure.has_value();
        case StreamPullStatus::EndOfStream:
            return end_marker.has_value() && end_marker->kind == buffer::StreamEndKind::EndOfFile
                && !chunk.has_value() && !wait_hint.has_value() && !discontinuity.has_value() && !failure.has_value();
        case StreamPullStatus::LiveEdgeReached:
            return end_marker.has_value() && end_marker->kind == buffer::StreamEndKind::LiveEdgeReached
                && !chunk.has_value() && !discontinuity.has_value() && !failure.has_value();
        case StreamPullStatus::Discontinuity:
            return discontinuity.has_value() && !chunk.has_value() && !end_marker.has_value() && !failure.has_value();
        case StreamPullStatus::Fenced:
        case StreamPullStatus::Flushed:
            return !chunk.has_value() && !end_marker.has_value() && !failure.has_value();
        case StreamPullStatus::Cancelled:
            return !chunk.has_value()
                && (!end_marker.has_value() || end_marker->kind == buffer::StreamEndKind::Cancelled);
        case StreamPullStatus::Terminated:
            return !chunk.has_value()
                && (!end_marker.has_value() || end_marker->kind == buffer::StreamEndKind::Stopped);
        case StreamPullStatus::Error:
            return failure.has_value() && !chunk.has_value() && !end_marker.has_value() && !discontinuity.has_value();
        }
        return false;
    }
};

} // namespace kivo::playback::control_plane::contracts
