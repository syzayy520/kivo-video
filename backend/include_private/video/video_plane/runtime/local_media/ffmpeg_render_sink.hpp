#pragma once

#include <android/native_window.h>

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/decode_core/decoded_frame.hpp"
#include "video/video_plane/runtime/local_media/ffmpeg_frame_sink.hpp"

namespace kivo::video::video_plane::runtime::local_media {

/// RGBA 帧直写 ANativeWindow（211 设计稿 D3 路 A）。
///
/// 单一职责：把一帧 `rgba` 数据按窗口 stride 拷进 Surface 缓冲并上屏。
/// **这条路径天然是 CPU 写入**（软解的定义），不是零拷贝直出——证据里
/// `render.sink = "anative_window_cpu"` 说了实话，读者不会误读成硬解路径。
///
/// 每帧成本 = 一次 lock + 一次按行 memcpy + 一次 post；
/// 窗口尺寸变化时重设一次 buffer geometry（其余帧直接写）。
class FfmpegWindowSink final : public IFfFrameSink {
public:
    /// 构造时 `ANativeWindow_acquire`，析构时 release（所有权在本类）。
    explicit FfmpegWindowSink(ANativeWindow* window);
    ~FfmpegWindowSink() override;
    FfmpegWindowSink(const FfmpegWindowSink&) = delete;
    FfmpegWindowSink& operator=(const FfmpegWindowSink&) = delete;

    /// 渲染一帧。frame 必须是 rgba（`decode_video_rgba` 的输出）；
    /// 尺寸与窗口不符时自动重设 geometry。失败原因写入 `error`。
    bool render(const kivo::cinema_engine::DecodedFrame& frame,
                std::string& error) override;

    [[nodiscard]] std::int64_t rendered_frames() const noexcept override
    {
        return rendered_;
    }
    [[nodiscard]] const char* name() const noexcept override
    {
        return "anative_window_cpu";
    }

private:
    bool ensure_geometry(int width, int height, std::string& error);

    ANativeWindow* window_;
    std::int64_t rendered_ = 0;
    int configured_width_ = 0;
    int configured_height_ = 0;
};

}  // namespace kivo::video::video_plane::runtime::local_media
