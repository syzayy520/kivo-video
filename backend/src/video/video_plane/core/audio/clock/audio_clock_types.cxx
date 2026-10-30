#include "video/video_plane/core/audio/clock/audio_clock_types.hpp"

namespace kivo::video::core::audio {

const char* audio_clock_source_name(AudioClockSourceKind kind) noexcept {
    switch (kind) {
        case AudioClockSourceKind::kAudioTimestamp:
            return "audio_timestamp";
        case AudioClockSourceKind::kAudioFramesRead:
            return "audio_frames_read";
        case AudioClockSourceKind::kAudioFramesWritten:
            return "audio_frames_written";
        case AudioClockSourceKind::kSystem:
            return "system";
    }
    return "unknown";
}

}  // namespace kivo::video::core::audio
