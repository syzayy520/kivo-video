#include "video/video_plane/backends/aaudio/clock/aaudio_timestamp_reading.hpp"

namespace kivo::video::aaudio {

const char* aaudio_timestamp_status_name(AaudioTimestampStatus status) noexcept {
    switch (status) {
        case AaudioTimestampStatus::kOk:
            return "ok";
        case AaudioTimestampStatus::kNotStarted:
            return "not_started";
        case AaudioTimestampStatus::kUnavailable:
            return "unavailable";
        case AaudioTimestampStatus::kTimeout:
            return "timeout";
        case AaudioTimestampStatus::kDisconnected:
            return "disconnected";
        case AaudioTimestampStatus::kInvalidStream:
            return "invalid_stream";
        case AaudioTimestampStatus::kOther:
            return "other";
    }
    return "unknown";
}

}  // namespace kivo::video::aaudio
