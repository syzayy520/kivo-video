#pragma once

#include <cstdint>

#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"

namespace kivo::video::video_plane::runtime::local_media {

/// fd 版的 FFmpeg 输入回调（211 软解兜底）。
///
/// Android 的 JNI 入口只给 `fd + offset + length`（与 `MediaExtractorSource::open_fd`
/// 同款约束：scoped storage 下路径不可依赖，fd 才是凭据）。桌面三件套的输入
/// 本来就是回调 IO（`FfmpegAdapter::open_custom_io`），这里只补一个 fd 实现。
///
/// **fd 的所有权留在调用方**（JNI 护栏负责关闭），本单元只借读；
/// `FfFdSource` 由调用方保活——`FfmpegInputCallbacks.user_data` 指向它。
///
/// **单一读取者约定**：AVIOContext 的读写是单线程的（demux 线程），
/// 实现用 lseek+read 维护游标而不是 pread，前提就是这个约定成立——
/// 若将来软解 demux 拆多线程，这里必须改成 pred 语义并单独立卡（C2 的教训同款）。
struct FfFdSource {
    int fd = -1;              ///< 借用的文件描述符；-1 = 未打开
    std::int64_t offset = 0;  ///< 数据起点（fd 内偏移，fd 本身可能是整包）
    std::int64_t length = -1; ///< 数据长度；-1 = 未知（Size 查询将返回 -1）
    std::int64_t cursor = 0;  ///< 相对 offset 的逻辑游标（回调维护）
};

/// 组装回调。`source` 的生命周期由调用方负责（必须活过整个 demux 会话）。
[[nodiscard]] kivo::cinema_engine::FfmpegInputCallbacks make_ff_fd_callbacks(
    FfFdSource* source);

}  // namespace kivo::video::video_plane::runtime::local_media
