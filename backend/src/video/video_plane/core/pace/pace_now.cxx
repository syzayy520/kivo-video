#include "video/video_plane/core/pace/pace_now.hpp"

#include <chrono>
#include <thread>

namespace kivo::video::core::pace {
namespace {

using SteadyClock = std::chrono::steady_clock;

}  // namespace

std::int64_t monotonic_now_ns() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(SteadyClock::now().time_since_epoch())
        .count();
}

void wait_until_ns(std::int64_t deadline_ns) noexcept {
    const SteadyClock::time_point deadline{std::chrono::nanoseconds{deadline_ns}};
    std::this_thread::sleep_until(deadline);
}

}  // namespace kivo::video::core::pace
