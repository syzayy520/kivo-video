#pragma once

#include <cstdint>

namespace kivo::video::mediacodec {

/// 呈现排程由谁负责（P12-C7）。
///
/// 两者都保证"该什么时候显示"，区别在于**谁来等**：
/// - `kCpuPaced`：CPU 在解码线程里睡到 `due_at`，醒了立刻 `releaseOutputBuffer`。
///   等的是人（线程调度），系统一忙就等过头。
/// - `kSurfaceAtTime`：不睡，把 `due_at` 作为时间戳交给 `releaseOutputBufferAtTime`，
///   由 SurfaceFlinger 在合成时按时间显示。等的是合成器，对线程调度免疫。
///
/// **默认是 `kSurfaceAtTime`**（C7 实测结论，见 `23-p12c7-status.md`）：
/// 加负载后 `kCpuPaced` 的 `lateness` p99 从 0.125 ms 涨到 0.375–0.875 ms、
/// max 从 0.18 ms 涨到 2.12 ms（跨轮极差 0.86 ms），而 `kSurfaceAtTime` 的提交提前量
/// 三轮 + 两档负载共 12 次全部是 **spread = 0**、p50 恒为 −99.875 ms。
///
/// `kCpuPaced` 保留为回退路径：`AMediaCodec_releaseOutputBufferAtTime` 的排程语义
/// 依赖厂商实现，本项目的真机验证只覆盖 K40 一台。换机出现帧数不闭合时先退它。
///
/// 单独成文件而不是塞进 `decode_session.hpp`：后者已经 include 了
/// `decode_receive_worker.hpp`，而这个枚举收帧段也要用，放回去就成环。
enum class PresentSchedule : std::int32_t {
    kCpuPaced = 0,       ///< C5 结构：睡到 due_at 再交（回退路径）
    kSurfaceAtTime = 1,  ///< C7：不睡，交时间戳给合成器（默认）
};

[[nodiscard]] const char* present_schedule_name(PresentSchedule schedule) noexcept;

}  // namespace kivo::video::mediacodec
