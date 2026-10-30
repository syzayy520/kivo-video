#include "video/video_plane/core/metrics/cpu_time_facts.hpp"

#if !defined(_WIN32)
#include <time.h>
#endif

namespace kivo::video::core::metrics {
namespace {

#if !defined(_WIN32)
[[nodiscard]] bool read_clock(clockid_t id, std::int64_t& out_ns) noexcept {
    timespec value{};
    if (clock_gettime(id, &value) != 0) {
        return false;
    }
    out_ns = static_cast<std::int64_t>(value.tv_sec) * 1'000'000'000 + value.tv_nsec;
    return true;
}
#endif

}  // namespace

CpuTimeSnapshot sample_cpu_time() noexcept {
#if defined(_WIN32)
    // Windows 没有 `CLOCK_THREAD_CPUTIME_ID` / `CLOCK_PROCESS_CPUTIME_ID`。
    // 桌面后端是**参考实现与逻辑验证台**，G2 只在 Android 上判，
    // 所以这里老实返回"未测到"，而不是拿 `GetThreadTimes` 凑一个近似值 ——
    // 一个来源不同的数一旦进了证据，就没人记得它是近似。
    return CpuTimeSnapshot{};
#else
    CpuTimeSnapshot snapshot;
    const bool thread_ok = read_clock(CLOCK_THREAD_CPUTIME_ID, snapshot.thread_ns);
    const bool process_ok = read_clock(CLOCK_PROCESS_CPUTIME_ID, snapshot.process_ns);
    const bool wall_ok = read_clock(CLOCK_MONOTONIC, snapshot.mono_ns);

    // 三个都要：只有 CPU 时间没有墙钟，算不出占比；只有墙钟，算不出 CPU。
    snapshot.valid = thread_ok && process_ok && wall_ok;
    if (!snapshot.valid) {
        snapshot = CpuTimeSnapshot{};
    }
    return snapshot;
#endif
}

CpuTimeFacts make_cpu_time_facts(const CpuTimeSnapshot& begin,
                                 const CpuTimeSnapshot& end) noexcept {
    CpuTimeFacts facts;
    if (!begin.valid || !end.valid) {
        facts.reason = "cpu time snapshot is invalid (clock_gettime unavailable or failed)";
        return facts;
    }

    facts.wall_ns = end.mono_ns - begin.mono_ns;
    facts.thread_cpu_ns = end.thread_ns - begin.thread_ns;
    facts.process_cpu_ns = end.process_ns - begin.process_ns;

    if (facts.wall_ns <= 0) {
        // 墙钟不前进只可能是快照取反了（end 早于 begin）。这时两个比值都是垃圾，
        // 但 CPU 时间本身仍然如实出版 —— 让读者能看出"是分母坏了"。
        facts.reason = "wall clock did not advance between snapshots; ratios are undefined";
        return facts;
    }

    facts.thread_ratio =
        static_cast<double>(facts.thread_cpu_ns) / static_cast<double>(facts.wall_ns);
    facts.process_ratio =
        static_cast<double>(facts.process_cpu_ns) / static_cast<double>(facts.wall_ns);
    facts.valid = true;
    facts.reason = "";
    return facts;
}

}  // namespace kivo::video::core::metrics
