#pragma once

namespace kivo::playback::control_plane::buffer {

/// StreamEndKind enumerates how a stream read reached its end.
/// Sole authority: buffer/stream_end_kind.hpp
enum class StreamEndKind
{
    None,
    EndOfFile,
    LiveEdgeReached,
    Stopped,
    Cancelled
};

} // namespace kivo::playback::control_plane::buffer
