#include "video/video_plane/backends/mediacodec/session/decode_submit_worker.hpp"

#include <chrono>

#include "video/video_plane/backends/mediacodec/decode/mediacodec_video_decoder.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_failure_slot.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_pipeline_counters.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_pipeline_gate.hpp"
#include "video/video_plane/backends/mediacodec/session/decode_stage_cost.hpp"
#include "video/video_plane/backends/mediacodec/source/mediaextractor_source.hpp"

namespace kivo::video::mediacodec {
namespace {

using SteadyClock = std::chrono::steady_clock;

/// 输入满之后等空位的上限。取 500us：短到输入一腾出空位就能立刻补上，
/// 长到不至于把 CPU 烧在自旋上（一帧 5.9ms 最多醒 12 次）。
constexpr std::int64_t kRoomWaitUs = 500;

[[nodiscard]] std::int64_t elapsed_us(SteadyClock::time_point from, SteadyClock::time_point to) {
    return std::chrono::duration_cast<std::chrono::microseconds>(to - from).count();
}

}  // namespace

DecodeSubmitWorker::DecodeSubmitWorker(Deps deps) : deps_(deps) {}

StageStep DecodeSubmitWorker::pump(bool allow_wait) {
    switch (sequencer_.next()) {
        case core::pipeline::SubmitStep::kReadSample:
            return read_sample();
        case core::pipeline::SubmitStep::kSubmitPending:
            return submit_pending(allow_wait);
        case core::pipeline::SubmitStep::kSignalEos:
            return signal_eos(allow_wait);
        case core::pipeline::SubmitStep::kFinished:
            return StageStep::kDone;
        case core::pipeline::SubmitStep::kAbort:
            // 原因在终止那一刻已经写进失败槽（见 abort()），这里只负责收工。
            return StageStep::kDone;
    }
    return StageStep::kDone;
}

void DecodeSubmitWorker::run() {
    while (!deps_.gate->stop_requested()) {
        if (pump(true) == StageStep::kDone) {
            return;
        }
    }
}

StageStep DecodeSubmitWorker::read_sample() {
    SampleView sample{};
    const SampleReadStatus status = deps_.source->read_next_sample(sample);

    if (status == SampleReadStatus::kOk) {
        if (sample.is_encrypted) {
            // 加密轨道需要 MediaCrypto，当前未实现。显式拒绝，绝不静默丢包（G9）。
            deps_.counters->samples_rejected.fetch_add(1, std::memory_order_relaxed);
            return abort("encrypted sample encountered; secure playback is not supported");
        }
        pending_ = sample;
        sequencer_.note_sample(sample.presentation_time_us);
        return StageStep::kProgress;
    }

    if (status == SampleReadStatus::kEndOfStream) {
        sequencer_.note_end_of_stream();
        return StageStep::kProgress;
    }

    return abort("extractor read failed");
}

StageStep DecodeSubmitWorker::submit_pending(bool allow_wait) {
    for (;;) {
        const auto begin = SteadyClock::now();
        const DecodeOutcome outcome = deps_.decoder->submit(pending_, 0);
        deps_.submit_cost->record(elapsed_us(begin, SteadyClock::now()));

        if (outcome.ok()) {
            deps_.counters->samples_submitted.fetch_add(1, std::memory_order_relaxed);
            deps_.counters->bytes_submitted.fetch_add(static_cast<std::int64_t>(pending_.size),
                                                      std::memory_order_relaxed);
            sequencer_.note_submitted();
            return StageStep::kProgress;
        }
        if (!outcome.try_again()) {
            return abort(outcome.message);
        }

        // 输入满：**不读下一包**，也不丢这一包。等输出侧腾位置后重送同一包。
        if (!allow_wait) {
            return StageStep::kIdle;
        }
        if (!deps_.gate->wait_for_input_room(kRoomWaitUs)) {
            return StageStep::kIdle;
        }
    }
}

StageStep DecodeSubmitWorker::signal_eos(bool allow_wait) {
    for (;;) {
        const auto begin = SteadyClock::now();
        const DecodeOutcome outcome =
            deps_.decoder->signal_end_of_stream(sequencer_.eos_presentation_us());
        deps_.eos_cost->record(elapsed_us(begin, SteadyClock::now()));

        if (outcome.ok()) {
            sequencer_.note_eos_signaled();
            // 收帧段靠这个标志区分"下一帧还没解出来"和"解码器卡死了"。
            deps_.gate->note_submit_finished();
            return StageStep::kDone;
        }
        if (!outcome.try_again()) {
            return abort(outcome.message);
        }
        if (!allow_wait) {
            return StageStep::kIdle;
        }
        if (!deps_.gate->wait_for_input_room(kRoomWaitUs)) {
            return StageStep::kIdle;
        }
    }
}

StageStep DecodeSubmitWorker::abort(const std::string& reason) {
    deps_.failure->mark(reason);
    deps_.gate->request_stop();
    return StageStep::kDone;
}

}  // namespace kivo::video::mediacodec
