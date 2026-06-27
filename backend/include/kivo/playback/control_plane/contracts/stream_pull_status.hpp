#pragma once

namespace kivo::playback::control_plane::contracts {

/// StreamPullStatus enumerates stream pull outcomes.
/// Sole authority: contracts/stream_pull_status.hpp
enum class StreamPullStatus
{
    Data,
    WouldBlock,
    EndOfStream,
    LiveEdgeReached,
    Discontinuity,
    Fenced,
    Flushed,
    Cancelled,
    Terminated,
    Error
};

} // namespace kivo::playback::control_plane::contracts
