#include "video/video_plane/core/pace/pace_worst_episode.hpp"

namespace kivo::video::core::pace {

DriftExtreme WorstEpisode::position() const noexcept {
    DriftExtreme out;
    out.valid = valid;
    if (valid) {
        out.index = index;
        // `of` 由调用方回填：本类数的是"喂进来的帧"，与 `lateness.count()` 是同一个数，
        // 但两边各数一次迟早分家（P13-F 的教训），所以这里只给位置，不给总数。
        out.of = of;
        out.media_ns = media_ns;
        out.lateness_ns = lateness_ns;
    }
    return out;
}

void WorstEpisodeRecorder::note(const EpisodeSample& sample) noexcept {
    const std::int64_t lateness_ns = sample.lateness_ns;
    const std::int32_t position = samples_;

    // ① 先填"后邻帧"。
    //
    // 只在**已经存在最差帧、且这一帧确实在它之后**时才填：新最差会走下面的重置分支，
    // 把 `after_count` 清零 —— 若在这里先塞进去再由重置擦掉，等于白写一次，
    // 而"白写"这种无害的错误一旦被人抄成"先填再判"，就会真的错。
    if (episode_.valid && position > episode_.index &&
        episode_.after_count < WorstEpisode::kAfter) {
        episode_.after_ns[episode_.after_count] = lateness_ns;
        ++episode_.after_count;
    }

    // ② 判新最差。有符号比较、严格大于 ⇒ 并列时保留**时间上最早**的那个，
    //    与 `PaceStats` 里原来的逐帧极值同口径（判据 F1 依赖这一点）。
    if (!episode_.valid || lateness_ns > episode_.lateness_ns) {
        episode_.valid = true;
        episode_.index = position;
        episode_.media_ns = sample.media_ns;
        episode_.lateness_ns = lateness_ns;
        episode_.due_step_ns = sample.due_step_ns;
        episode_.cumulative_present_step_ns = sample.cumulative_present_step_ns;
        episode_.cumulative_due_step_ns = sample.cumulative_due_step_ns;
        episode_.lateness_at_first_ns = sample.lateness_at_first_ns;
        episode_.clock_mono_ns = sample.clock_mono_ns;
        episode_.clock_elapsed_ns = sample.clock_elapsed_ns;
        episode_.anchor_pts_ns = sample.anchor_pts_ns;
        episode_.due_at_ns = sample.due_at_ns;
        episode_.before_count = recent_count_;
        for (std::int32_t i = 0; i < recent_count_; ++i) {
            episode_.before_ns[i] = recent_[i];
        }
        // 新最差自己的"后邻帧"从下一帧开始，旧的尾巴属于另一个尖峰，必须丢掉。
        episode_.after_count = 0;
    }

    // ③ 最后才把本帧推进前邻环 —— 它不能成为自己的"前一帧"。
    //    `kBefore` 很小（2），直接左移比取模清楚，也不会有人把环下标算错。
    if (recent_count_ < WorstEpisode::kBefore) {
        recent_[recent_count_] = lateness_ns;
        ++recent_count_;
    } else {
        for (std::int32_t i = 1; i < WorstEpisode::kBefore; ++i) {
            recent_[i - 1] = recent_[i];
        }
        recent_[WorstEpisode::kBefore - 1] = lateness_ns;
    }
    ++samples_;
}

}  // namespace kivo::video::core::pace
