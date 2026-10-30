#include "video/video_plane/backends/aaudio/clock/aaudio_monotonic_now.hpp"

#if defined(_WIN32)
#include <windows.h>
#else
#include <ctime>
#endif

namespace kivo::video::aaudio {

std::int64_t monotonic_now_ns() noexcept {
#if defined(_WIN32)
    // 这个分支只服务于"在桌面上跑时钟单测"，生产路径永远不会走到它
    // （`backends/aaudio` 只由 android 的 CMake 编译）。
    // 用 QPC 而不是 steady_clock，同样是为了拿一把明确的、与平台无歧义的尺子。
    static const std::int64_t kFrequency = []() noexcept {
        LARGE_INTEGER frequency{};
        QueryPerformanceFrequency(&frequency);
        return static_cast<std::int64_t>(frequency.QuadPart);
    }();

    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    const std::int64_t ticks = static_cast<std::int64_t>(counter.QuadPart);

    // 拆成整秒 + 余数，避免 ticks * 1e9 溢出
    return ticks / kFrequency * 1'000'000'000LL +
           (ticks % kFrequency) * 1'000'000'000LL / kFrequency;
#else
    timespec now{};
    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<std::int64_t>(now.tv_sec) * 1'000'000'000LL +
           static_cast<std::int64_t>(now.tv_nsec);
#endif
}

}  // namespace kivo::video::aaudio
