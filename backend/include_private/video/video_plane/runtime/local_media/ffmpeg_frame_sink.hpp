#pragma once

#include "kivo/cinema_engine/decode_core/decoded_frame.hpp"

namespace kivo::video::video_plane::runtime::local_media {

/// 呈现接缝：会话只认这个接口。
/// 桌面测试注入计数 sink；Android 注入 `FfmpegWindowSink`（ANativeWindow 直写）。
/// 接口保持可移植——**这里不许出现任何 Android 类型**（P12 架构红线同款纪律）。
/// 单独成头：会话与 sink 两个家族都要 include 它，塞在谁家里都会造成反向依赖。
class IFfFrameSink {
public:
    virtual ~IFfFrameSink() = default;
    virtual bool render(const kivo::cinema_engine::DecodedFrame& frame,
                        std::string& error) = 0;
    [[nodiscard]] virtual std::int64_t rendered_frames() const = 0;
    /// 证据出版用：sink 自报身份（如 "anative_window_cpu" / "counting"）。
    [[nodiscard]] virtual const char* name() const = 0;
};

}  // namespace kivo::video::video_plane::runtime::local_media
