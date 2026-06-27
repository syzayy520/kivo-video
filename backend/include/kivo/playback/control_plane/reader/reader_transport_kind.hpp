#pragma once

namespace kivo::playback::control_plane::reader {

/// ReaderTransportKind describes transport without provider name.
/// Provider name must NOT enter P4 policy.
enum class ReaderTransportKind
{
    LocalSeekableFile,
    HttpRangeLike,
    RemoteObjectRange,
    RemoteStreaming,
    LanRemote,
    UnknownRemote
};

} // namespace kivo::playback::control_plane::reader
