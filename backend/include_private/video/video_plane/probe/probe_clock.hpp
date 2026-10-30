#pragma once

#include <chrono>
#include <cstdint>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace kivo::video::video_plane::probe {

// Monotonic wall clock in seconds. Used for fps and latency, never for dating
// evidence files.
[[nodiscard]] inline auto steady_seconds() noexcept -> double {
    using clock = std::chrono::steady_clock;
    static const auto origin = clock::now();
    return std::chrono::duration<double>(clock::now() - origin).count();
}

// Total CPU cycles burned by every thread of this process since it started.
//
// Why not GetProcessTimes: on this machine the FILETIME counters come back
// quantised in 15.6 ms steps and clearly under-report threaded work (a 3.1 s
// software decode reported 62 ms of CPU). Cycle counts are monotonic and exact,
// and because the hardware/software comparison is a *ratio*, the CPU frequency
// cancels out -- no calibration needed.
[[nodiscard]] inline auto process_cycle_count() noexcept -> unsigned long long {
#ifdef _WIN32
    unsigned long long cycles = 0;
    if (::QueryProcessCycleTime(::GetCurrentProcess(), &cycles) == 0) {
        return 0ULL;
    }
    return cycles;
#else
    return 0ULL;
#endif
}

// CPU time consumed by this process so far, in seconds.
// Kept for portability; prefer process_cycle_count() on Windows because the
// wall-clock timer is more reliable than this value in sandboxed sessions.
[[nodiscard]] inline auto process_cpu_seconds() noexcept -> double {
#ifdef _WIN32
    FILETIME creation{};
    FILETIME exit{};
    FILETIME kernel{};
    FILETIME user{};
    if (::GetProcessTimes(::GetCurrentProcess(), &creation, &exit, &kernel, &user) == 0) {
        return 0.0;
    }
    const auto to_seconds = [](const FILETIME& ft) noexcept -> double {
        ULARGE_INTEGER value{};
        value.LowPart = ft.dwLowDateTime;
        value.HighPart = ft.dwHighDateTime;
        // FILETIME counts 100 ns ticks.
        return static_cast<double>(value.QuadPart) * 1.0e-7;
    };
    return to_seconds(kernel) + to_seconds(user);
#else
    return 0.0;
#endif
}

}  // namespace kivo::video::video_plane::probe
