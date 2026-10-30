#pragma once

namespace kivo::playback::control_plane::primitives {

/// StreamType identifies stream kind.
/// Sole authority: primitives/stream_type.hpp
enum class StreamType
{
    Video,
    Audio,
    Subtitle,
    Data,
    Attachment,
    Unknown
};

} // namespace kivo::playback::control_plane::primitives
