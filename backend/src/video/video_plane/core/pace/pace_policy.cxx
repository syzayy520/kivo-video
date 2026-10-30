#include "video/video_plane/core/pace/pace_policy.hpp"

namespace kivo::video::core::pace {

bool PacePolicy::sane() const noexcept {
    if (early_margin_ns <= 0 || late_drop_ns <= early_margin_ns || catch_up_run <= 0) {
        return false;
    }
    if (jump_limit_ns <= 0 || stale_extrapolate_limit_ns <= 0 || max_single_wait_ns <= 0) {
        return false;
    }
    if (min_frame_duration_ns <= 0 || max_frame_duration_ns <= min_frame_duration_ns ||
        default_frame_duration_ns < min_frame_duration_ns ||
        default_frame_duration_ns > max_frame_duration_ns) {
        return false;
    }
    return true;
}

PaceVerdict decide(const PaceInput& input, const PacePolicy& policy) noexcept {
    PaceVerdict verdict;

    // 追帧中：只要还没追上就继续丢。追上（落后不超过一帧）立即退出。
    // 退出条件用 `late_drop_ns` 的一半而不是 0：追到"还差半帧"就够，
    // 否则一旦时钟略快就永远差一点点，追帧模式退不出来。
    if (input.catching_up) {
        if (input.lateness_ns <= policy.late_drop_ns / 2) {
            verdict.leave_catch_up = true;
            verdict.action = PaceAction::kPresent;
            // 退出当帧不再丢：这一帧就是"刚刚好"的那一帧。
        } else {
            verdict.action = PaceAction::kCatchUp;
        }
        return verdict;
    }

    // 早到：睡。注意是 `<` 而非 `<=`，正好落在 early_margin 边界上时不睡。
    if (input.lateness_ns < -policy.early_margin_ns) {
        verdict.action = PaceAction::kWait;
        return verdict;
    }

    // 落后超过阈值：丢。
    if (input.lateness_ns > policy.late_drop_ns) {
        if (!policy.allow_drop) {
            // 明确要求"宁可晚也不丢"：照样呈现，迟到量由直方图如实记录。
            return verdict;  // action 保持 kPresent
        }
        if (input.consecutive_late + 1 > policy.catch_up_run) {
            verdict.enter_catch_up = true;
            verdict.action = PaceAction::kCatchUp;
        } else {
            verdict.action = PaceAction::kDrop;
        }
        return verdict;
    }

    verdict.action = PaceAction::kPresent;
    return verdict;
}

}  // namespace kivo::video::core::pace
