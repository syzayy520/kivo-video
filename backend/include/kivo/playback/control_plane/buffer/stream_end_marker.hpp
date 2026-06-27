#pragma once

#include "kivo/playback/control_plane/buffer/stream_end_kind.hpp"

namespace kivo::playback::control_plane::buffer {

/// EndProjectionKind distinguishes true EOF from live-edge-projected-as-EOF.
/// Rules:
/// - LiveEdgeReached is never Recovery.
/// - Internal: LiveEdgeReached is not EOF.
/// - Policy: if treat_live_edge_as_eof=true, SessionEvent may publish
///   EndReached(live_edge_projected_as_eof), but StreamEndKind remains
///   LiveEdgeReached.
enum class EndProjectionKind
{
    TrueEof,
    LiveEdgeProjectedAsEof
};

} // namespace kivo::playback::control_plane::buffer
