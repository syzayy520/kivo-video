#pragma once

#include <cstdint>

#include "video/video_plane/backends/aaudio/clock/aaudio_timestamp_reading.hpp"

namespace kivo::video::aaudio {

/// 时钟源的读数接口。
///
/// 存在的唯一理由：让 `AaudioClockSource` 的**回退分级**（三档怎么选）
/// 能在没有 Android 设备的机器上被测到。
///
/// 正常运行时三档回退一次都不会触发 —— 恰恰是这种"平时不跑"的分支
/// 最容易在后续改动里被悄悄写坏，而它一旦坏掉，出问题的那一刻正是没人盯着的时候。
class AaudioClockProbe {
public:
    virtual ~AaudioClockProbe() = default;

    [[nodiscard]] virtual AaudioTimestampReading read_timestamp() const = 0;
    [[nodiscard]] virtual std::int64_t read_frames_read() const = 0;
    [[nodiscard]] virtual std::int64_t read_frames_written() const = 0;
};

}  // namespace kivo::video::aaudio
