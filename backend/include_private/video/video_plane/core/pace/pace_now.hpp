#pragma once

#include <cstdint>

namespace kivo::video::core::pace {

/// 单调时间源。返回纳秒，同一进程内单调不减。
///
/// 做成函数指针而不是 `std::function`：热路径每帧调两次，
/// `std::function` 会多一层间接且可能持堆内存；单测里换实现只要换个函数。
using NowFn = std::int64_t (*)();

/// 睡到某个单调时刻。**必须**支持"已经过期立即返回"。
using WaitFn = void (*)(std::int64_t deadline_ns);

/// 默认实现：`std::chrono::steady_clock`。
///
/// 为什么不用 `CLOCK_MONOTONIC` 直调：`steady_clock` 在 libc++ 上就是它，
/// 而 `core/` 不许出现任何平台头（07 §六 第 6 条）。多一层封装不损失精度。
[[nodiscard]] std::int64_t monotonic_now_ns() noexcept;

/// 默认实现：`sleep_until` 绝对时刻，而不是 `sleep_for` 相对时长。
///
/// 相对时长会累积漂移：每帧多睡 0.3ms，30fps 下 10 秒就是 9ms 的系统性偏移
/// （C3 的供给线程踩过这个坑，见 14-p12c3-status.md）。
void wait_until_ns(std::int64_t deadline_ns) noexcept;

}  // namespace kivo::video::core::pace
