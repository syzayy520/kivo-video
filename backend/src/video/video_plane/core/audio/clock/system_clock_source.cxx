#include "video/video_plane/core/audio/clock/system_clock_source.hpp"

#include <chrono>

namespace kivo::video::core::audio {
namespace {

std::int64_t steady_now_ns() noexcept {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

}  // namespace

SystemClockSource::SystemClockSource() noexcept : origin_ns_(steady_now_ns()) {}

AudioClockSnapshot SystemClockSource::now() {
    AudioClockSnapshot snapshot;
    snapshot.position_ns = steady_now_ns() - origin_ns_;
    snapshot.source = AudioClockSourceKind::kSystem;
    // steady_clock 契约上单调，不需要守卫；仍显式写出这两个字段，
    // 免得读代码的人以为"忘了处理"。
    snapshot.stale = false;
    snapshot.monotonic_rejected = false;
    snapshot.jump_clamped = false;
    return snapshot;
}

void SystemClockSource::reset() noexcept { origin_ns_ = steady_now_ns(); }

void SystemClockSource::rebase(std::int64_t media_position_ns) noexcept {
    // 让 `now().position_ns == media_position_ns + 已流逝墙钟`。
    // seek 之后唯一正确的理解：进度应从落点继续，而不是从"播放开始"的墙钟继续（244）。
    origin_ns_ = steady_now_ns() - media_position_ns;
}

}  // namespace kivo::video::core::audio
