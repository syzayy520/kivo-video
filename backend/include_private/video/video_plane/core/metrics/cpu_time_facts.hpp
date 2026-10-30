#pragma once

#include <cstdint>

namespace kivo::video::core::metrics {

/// 一次 CPU 时间快照。三个数同源同刻，缺一个就没法算占比。
struct CpuTimeSnapshot {
    bool valid = false;
    std::int64_t thread_ns = 0;   ///< `CLOCK_THREAD_CPUTIME_ID`（当前线程累计 CPU）
    std::int64_t process_ns = 0;  ///< `CLOCK_PROCESS_CPUTIME_ID`（整个进程累计 CPU）
    std::int64_t mono_ns = 0;     ///< 同一刻的单调墙钟，作为占比的分母
};

/// 读一次快照。读不到时三个数都置 0 且 `valid = false` ——
/// **不用 0 冒充"没花 CPU"**，那会把"没测到"读成"效率极高"。
[[nodiscard]] CpuTimeSnapshot sample_cpu_time() noexcept;

/// 一段会话的 CPU 事实（P14-A / G2a）。
///
/// 只有**占比**，没有"是否达标"：G2 的判据是"≤ 同片源软解的 10%"，
/// 而这台机器上没有软解路径，分母根本不存在（见 `115-p14a-design.md` §二）。
/// 出版绝对占比，比编一个"相对软解"的数诚实。
struct CpuTimeFacts {
    bool valid = false;

    /// `reason` 非空表示没测到。**放在结构体里而不是只给 valid**：
    /// 一个 `valid = false` 旁边没有理由，读者只能猜是"没实现"还是"这轮没采到"。
    const char* reason = "";

    std::int64_t wall_ns = 0;
    std::int64_t thread_cpu_ns = 0;
    std::int64_t process_cpu_ns = 0;

    /// CPU 时间 ÷ 墙钟。可以 > 1（多线程并行），也可以远小于 1（硬解在等 vsync）。
    double thread_ratio = 0.0;
    double process_ratio = 0.0;
};

/// 由首尾两个快照算事实。**纯函数**：同样的输入永远同样的输出，可单测。
[[nodiscard]] CpuTimeFacts make_cpu_time_facts(const CpuTimeSnapshot& begin,
                                               const CpuTimeSnapshot& end) noexcept;

}  // namespace kivo::video::core::metrics
