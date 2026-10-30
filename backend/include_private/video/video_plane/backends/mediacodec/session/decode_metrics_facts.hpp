#pragma once

#include "video/video_plane/core/metrics/alloc_probe.hpp"
#include "video/video_plane/core/metrics/buffer_lease_facts.hpp"
#include "video/video_plane/core/metrics/cpu_time_facts.hpp"

namespace kivo::video::mediacodec {

/// 三条"以前没有尺子"的目标的事实合集（P14-A）。
///
/// 为什么把三个无关的量装在一个块里：它们进的是**同一份证据**。
/// 分成三份文件就会被读成三次不同的运行 —— M5 判据（三条必须在同一份 decode
/// 证据里）说的就是这件事。
///
/// 三块各自带 `valid` / `reason` / `hook_installed`：这一族最容易犯的错，
/// 是把"没测到"印成"0"，而 0 看起来永远是个好消息。
struct DecodeMetricsFacts {
    /// G2a：CPU 占比。**不含**"相对软解 10%"那一半 —— 那个分母在这台机器上不存在。
    core::metrics::CpuTimeFacts cpu{};

    /// G3：堆分配调用次数。`hook_installed = false` 时所有计数都不可读。
    core::metrics::AllocFacts alloc{};

    /// G4：输出缓冲归属账。`pool_owner = "codec"` 说明平台池不在我们手里。
    core::metrics::BufferLeaseFacts buffers{};
};

}  // namespace kivo::video::mediacodec
