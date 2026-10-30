#include "video/video_plane/backends/aaudio/clock/aaudio_timestamp_reader.hpp"

#include <ctime>

namespace kivo::video::aaudio {
namespace {

AaudioTimestampStatus classify(aaudio_result_t result) noexcept {
    switch (result) {
        case AAUDIO_OK:
            return AaudioTimestampStatus::kOk;
        case AAUDIO_ERROR_INVALID_STATE:
            return AaudioTimestampStatus::kNotStarted;
        case AAUDIO_ERROR_UNAVAILABLE:
            return AaudioTimestampStatus::kUnavailable;
        case AAUDIO_ERROR_TIMEOUT:
            return AaudioTimestampStatus::kTimeout;
        case AAUDIO_ERROR_DISCONNECTED:
            return AaudioTimestampStatus::kDisconnected;
        default:
            return AaudioTimestampStatus::kOther;
    }
}

}  // namespace

AaudioTimestampReading AaudioTimestampReader::read_timestamp() const noexcept {
    AaudioTimestampReading reading;

    if (stream_ == nullptr) {
        reading.status = AaudioTimestampStatus::kInvalidStream;
        return reading;
    }

    // NDK 原文："If an error occurs, then the position and time will not be modified."
    // 所以必须先初始化为 0 再传进去，拿到错误码后也不能读这两个出参。
    std::int64_t frame_position = 0;
    std::int64_t time_nanos = 0;

    // clockid 必须与调用方外推时用的 `now` 同源：
    // 这里给 CLOCK_MONOTONIC，调用方就必须用 clock_gettime(CLOCK_MONOTONIC)，
    // 不能用 std::chrono::steady_clock（标准不保证同源）。
    const aaudio_result_t result =
        AAudioStream_getTimestamp(stream_, CLOCK_MONOTONIC, &frame_position, &time_nanos);

    reading.raw_result = static_cast<std::int32_t>(result);
    reading.status = classify(result);

    if (reading.status == AaudioTimestampStatus::kOk) {
        reading.frame_position = frame_position;
        reading.time_nanos = time_nanos;
    }
    return reading;
}

std::int64_t AaudioTimestampReader::read_frames_read() const noexcept {
    if (stream_ == nullptr) {
        return -1;
    }
    return AAudioStream_getFramesRead(stream_);
}

std::int64_t AaudioTimestampReader::read_frames_written() const noexcept {
    if (stream_ == nullptr) {
        return -1;
    }
    return AAudioStream_getFramesWritten(stream_);
}

}  // namespace kivo::video::aaudio
