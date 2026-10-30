#include "video/video_plane/backends/mediacodec/session/decode_stage_cost.hpp"

namespace kivo::video::mediacodec {

void StageCostMeter::record(std::int64_t duration_us) noexcept {
    calls_.fetch_add(1, std::memory_order_relaxed);
    total_us_.fetch_add(duration_us, std::memory_order_relaxed);

    // 峰值用 CAS 而不是「先读再写」：两个 worker 都可能记同一段（merged 模式下它们同线程，
    // 但 split 模式下 pause/flush 之类的路径仍可能跨线程），丢一次峰值就等于谎报。
    std::int64_t observed = max_us_.load(std::memory_order_relaxed);
    while (duration_us > observed &&
           !max_us_.compare_exchange_weak(observed, duration_us, std::memory_order_relaxed)) {
        // observed 已被 CAS 更新为当前值，继续比较
    }
}

StageCost StageCostMeter::load() const noexcept {
    StageCost cost;
    cost.calls = calls_.load(std::memory_order_relaxed);
    cost.total_us = total_us_.load(std::memory_order_relaxed);
    cost.max_us = max_us_.load(std::memory_order_relaxed);
    return cost;
}

}  // namespace kivo::video::mediacodec
