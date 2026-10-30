#include "video/video_plane/core/pipeline/submit_sequencer.hpp"

namespace kivo::video::core::pipeline {

void SubmitSequencer::note_sample(std::int64_t pts_us) noexcept {
    has_pending_ = true;
    pending_pts_us_ = pts_us;
}

void SubmitSequencer::note_submitted() noexcept {
    if (has_pending_ && pending_pts_us_ > max_submitted_pts_us_) {
        max_submitted_pts_us_ = pending_pts_us_;
    }
    has_pending_ = false;
}

void SubmitSequencer::note_end_of_stream() noexcept {
    eos_ready_ = true;
}

void SubmitSequencer::note_eos_signaled() noexcept {
    eos_signaled_ = true;
}

void SubmitSequencer::note_aborted(const char* reason) noexcept {
    aborted_ = true;
    abort_reason_ = (reason != nullptr && reason[0] != '\0') ? reason : "unspecified";
}

SubmitStep SubmitSequencer::next() const noexcept {
    // 判定顺序：终止 → 有待送包 → 已发 EOS → 该发 EOS → 该读包。
    //
    // 有待送包排在 EOS 之前是刻意的：输入满时既"读完了"又"还没送出去"，
    // 此时必须先把手上的包送掉，否则 EOS 会插到它前面 —— 那等于把最后一帧丢了。
    if (aborted_) {
        return SubmitStep::kAbort;
    }
    if (has_pending_) {
        return SubmitStep::kSubmitPending;
    }
    if (eos_signaled_) {
        return SubmitStep::kFinished;
    }
    if (eos_ready_) {
        return SubmitStep::kSignalEos;
    }
    return SubmitStep::kReadSample;
}

}  // namespace kivo::video::core::pipeline
