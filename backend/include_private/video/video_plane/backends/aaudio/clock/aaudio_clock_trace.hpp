#pragma once

#include <cstdint>

namespace kivo::video::aaudio {

/// 一次时钟观测的原始三元组（P14-E）。
///
/// 三个数必须一起看：
///   `candidate`（设备给了什么）、`accepted`（守卫让它变成什么）、`mono`（同一刻的墙钟）。
/// 只看 `accepted` 看不出"是设备跳了还是我们算错了" —— 这正是本卡要回答的问题。
struct ClockTraceSample {
    std::int64_t mono_ns = 0;
    std::int64_t candidate_ns = 0;
    std::int64_t accepted_ns = 0;
    /// `AudioClockSourceKind` 的整数值（0=timestamp / 1=frames_read / …）。
    std::int32_t kind = 0;
    /// 1 = 这一次报过 `stale`。陈旧读数不该参与"跳变"归因。
    std::int32_t stale = 0;
};

/// 两条判据的窗口大小：出事前 8 个 + 出事那一个 + 之后 7 个。
///
/// 为什么不是"只存出事那一个"：**跳变是序列性质，不是单点性质**。
/// 一个孤立的候选值只能说明"它不对"，前后各 8 个点才能说明
/// "它是突然跳的，还是慢慢漂过去的、只是在某一帧被守卫判成倒退"。
inline constexpr std::int32_t kTraceWindow = 16;

/// 一次异常的快照。`valid = false` 表示这个槽没用过。
struct ClockTraceEvent {
    bool valid = false;
    /// 事件发生在这条窗口里的第几个样本（0 起）。−1 表示窗口还没填满。
    std::int32_t pivot = -1;
    /// 1 = 倒退被拒，2 = 跳变被钳制，3 = 两者同时。
    std::int32_t flags = 0;
    std::int32_t samples = 0;
    ClockTraceSample data[kTraceWindow] = {};
};

/// 最多记 3 次。为什么是 3：一次运行里同类异常通常 0–3 次，
/// 再多就说明"这不是异常而是常态"，那时候该改的是判据而不是加容量。
inline constexpr std::int32_t kTraceEvents = 3;

struct ClockTrace {
    std::int32_t event_count = 0;
    ClockTraceEvent events[kTraceEvents] = {};
};

/// 滚动窗口。**只负责存**，不判定、不解释。
///
/// 线程模型：只在时钟被查询的那个线程里用（与 `MonotonicClockGuard` 一致）。
class ClockTraceRing {
public:
    void push(const ClockTraceSample& sample) noexcept;

    /// 把当前窗口扣进 `trace` 的下一个空槽。**窗口里有几个点就存几个点** ——
    /// 补零会让读者以为"那时候位置是 0"。
    void snapshot_into(ClockTrace& trace, std::int32_t flags) noexcept;

    void reset() noexcept;

private:
    ClockTraceSample ring_[kTraceWindow] = {};
    std::int32_t write_index_ = 0;
    std::int32_t filled_ = 0;
};

}  // namespace kivo::video::aaudio
