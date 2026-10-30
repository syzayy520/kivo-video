#include "video/video_plane/backends/aaudio/clock/aaudio_clock_trace.hpp"
#include <cstddef>

namespace kivo::video::aaudio {

void ClockTraceRing::push(const ClockTraceSample& sample) noexcept {
    ring_[static_cast<std::size_t>(write_index_)] = sample;
    write_index_ = (write_index_ + 1) % kTraceWindow;
    if (filled_ < kTraceWindow) {
        ++filled_;
    }
}

void ClockTraceRing::snapshot_into(ClockTrace& trace, std::int32_t flags) noexcept {
    if (trace.event_count >= kTraceEvents) {
        return;  // 三个槽满了就不再记：再多说明这不是异常
    }
    ClockTraceEvent& event = trace.events[static_cast<std::size_t>(trace.event_count)];
    event.valid = true;
    event.flags = flags;
    event.samples = filled_;
    // 窗口**以事件结尾**，不做"事后 7 个点"：事后补记要等 7 次查询，
    // 而会话随时可能因为 EOS 结束 —— 那时这几个点就永远丢了。
    // 代价是看不到跳变之后的恢复形状；收益是**事件一定能留下来**。
    event.pivot = filled_ > 0 ? filled_ - 1 : -1;

    // 未满时最旧点在 0；满了之后最旧点在 `write_index_`。
    const std::int32_t oldest = (filled_ == kTraceWindow) ? write_index_ : 0;
    for (std::int32_t index = 0; index < filled_; ++index) {
        event.data[static_cast<std::size_t>(index)] =
            ring_[static_cast<std::size_t>((oldest + index) % kTraceWindow)];
    }
    ++trace.event_count;
}

void ClockTraceRing::reset() noexcept {
    write_index_ = 0;
    filled_ = 0;
}

}  // namespace kivo::video::aaudio
