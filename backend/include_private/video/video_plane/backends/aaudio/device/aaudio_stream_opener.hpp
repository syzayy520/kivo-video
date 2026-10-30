#pragma once

#include <aaudio/AAudio.h>

#include "video/video_plane/backends/aaudio/device/aaudio_open_outcome.hpp"
#include "video/video_plane/backends/aaudio/device/aaudio_stream_config.hpp"

namespace kivo::video::aaudio {

/// builder → open 的唯一入口，含 EXCLUSIVE → SHARED 降级链。
///
/// 为什么单独成单元：开流有四种失败方式（参数非法、builder 建不出来、
/// 独占被拒、设备被占），每种处置不同。塞进 session 会变成一坨 if，
/// 而且 session 的职责是"启停与统计"，不该知道独占比共享延迟低这件事。
class AaudioStreamOpener {
public:
    struct Request {
        AaudioStreamConfig config;
        AAudioStream_dataCallback data_callback = nullptr; ///< 为空则退化为阻塞写模式
        void* user_data = nullptr;
    };

    /// 成功时 `*out_stream` 非空，所有权归调用方（须 `AAudioStream_close`）。
    /// 失败时 `*out_stream` 置空，`outcome.message` 非空。
    ///
    /// 降级不算失败：独占被拒会改共享重开，但 `outcome.downgrade_reason` 一定非空。
    [[nodiscard]] static AaudioOpenOutcome open(const Request& request, AAudioStream** out_stream);
};

}  // namespace kivo::video::aaudio
