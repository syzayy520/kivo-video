#pragma once

#include <cstdint>
#include <mutex>

#include "video/video_plane/backends/mediacodec/session/decode_pace_stats.hpp"
#include "video/video_plane/core/pace/pace_stats.hpp"

namespace kivo::video::mediacodec {

/// 节拍摘要的跨线程发布：收帧段每 N 帧写一次，`snapshot()` 从任意线程读一次。
///
/// 为什么要经过它而不是让 `snapshot()` 直接读 `FramePacer`：
/// 节拍器不加锁（热路径每帧都动），跨线程读它就是数据竞争。
/// 这里把"拷一份量化后的摘要"这件事收成一处，锁只在这几微秒里出现。
class DecodePaceSampler {
public:
    /// 由收帧段调用。内部把 `PaceSummary` 转成证据形状后再上锁。
    void publish(const core::pace::PaceSummary& summary);

    /// 由 `DecodeSession::snapshot()` 调用。
    [[nodiscard]] DecodePaceStats read() const;

private:
    mutable std::mutex mutex_;
    DecodePaceStats snapshot_{};
};

}  // namespace kivo::video::mediacodec
