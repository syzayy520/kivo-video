#include "video/video_plane/backends/mediacodec/session/decode_pace_sampler.hpp"

#include "video/video_plane/backends/mediacodec/session/decode_pace_report.hpp"
#include "video/video_plane/core/pace/pace_now.hpp"

namespace kivo::video::mediacodec {

void DecodePaceSampler::publish(const core::pace::PaceSummary& summary, double rate_ppm,
                                std::int64_t phase_error_ns) {
    // 转换放在锁外：它要遍历直方图求分位数，几微秒到几十微秒都有，
    // 锁只该护住"替换快照"这一个赋值。
    // P16-P1B25：从进函数到快照替换完成，量一次真实开销。
    const std::int64_t started_ns = core::pace::monotonic_now_ns();
    DecodePaceStats sampled = decode_pace_stats_from(summary, summary.frames_decided);
    // P16-P1B12：频差观测随摘要一起发布（同线程读 pacer，无竞争）。
    sampled.rate_ppm = rate_ppm;
    sampled.phase_error_ns = phase_error_ns;
    const std::lock_guard<std::mutex> guard(mutex_);
    snapshot_ = sampled;
    snapshot_.publish_cost_ns = core::pace::monotonic_now_ns() - started_ns;
}

DecodePaceStats DecodePaceSampler::read() const {
    const std::lock_guard<std::mutex> guard(mutex_);
    return snapshot_;
}

}  // namespace kivo::video::mediacodec
