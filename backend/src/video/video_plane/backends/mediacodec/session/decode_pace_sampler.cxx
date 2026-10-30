#include "video/video_plane/backends/mediacodec/session/decode_pace_sampler.hpp"

#include "video/video_plane/backends/mediacodec/session/decode_pace_report.hpp"

namespace kivo::video::mediacodec {

void DecodePaceSampler::publish(const core::pace::PaceSummary& summary) {
    // 转换放在锁外：它要遍历直方图求分位数，几微秒到几十微秒都有，
    // 锁只该护住"替换快照"这一个赋值。
    const DecodePaceStats sampled = decode_pace_stats_from(summary, summary.frames_decided);
    const std::lock_guard<std::mutex> guard(mutex_);
    snapshot_ = sampled;
}

DecodePaceStats DecodePaceSampler::read() const {
    const std::lock_guard<std::mutex> guard(mutex_);
    return snapshot_;
}

}  // namespace kivo::video::mediacodec
