#pragma once

#include <cstdint>
#include <string>

#include "video/video_plane/backends/mediacodec/session/decode_stage_step.hpp"
#include "video/video_plane/backends/mediacodec/source/sample_view.hpp"
#include "video/video_plane/core/pipeline/submit_sequencer.hpp"

namespace kivo::video::mediacodec {

class MediaExtractorSource;
class MediaCodecVideoDecoder;
class DecodePipelineGate;
class DecodeFailureSlot;
class StageCostMeter;
struct DecodePipelineCounters;

/// 送包段：`read_next_sample` → `submit` → （读完了）`signal_end_of_stream`。
///
/// 它是 `MediaExtractorSource` 的**唯一**触碰者（该对象非线程安全，
/// 见其头注释），也是 `submit` / `signal_end_of_stream` 的唯一调用者。
/// 明确不做：`receive`、`release_frame`、节拍判定 —— 那些属于收帧段。
///
/// 一轮只做一次状态迁移（`pump`），循环由 `run()` 或编排者负责：
/// 这样同一份逻辑既能跑在自己的线程里，也能和收帧段挤在一个线程里做对照。
class DecodeSubmitWorker {
public:
    struct Deps {
        MediaExtractorSource* source = nullptr;
        MediaCodecVideoDecoder* decoder = nullptr;
        DecodePipelineGate* gate = nullptr;
        DecodePipelineCounters* counters = nullptr;
        DecodeFailureSlot* failure = nullptr;
        StageCostMeter* submit_cost = nullptr;
        StageCostMeter* eos_cost = nullptr;
    };

    explicit DecodeSubmitWorker(Deps deps);

    /// 跑一轮。
    ///
    /// @param allow_wait 输入满时是否等空位。**与收帧段同线程时必须给 false**：
    ///        room 只可能由收帧段的 `release` 产生，同线程里等它等于自己等自己。
    [[nodiscard]] StageStep pump(bool allow_wait);

    /// 在自己的线程里循环到收工。等待一律带超时，因此 `stop()` 不会卡满一整轮。
    void run();

private:
    StageStep read_sample();
    StageStep submit_pending(bool allow_wait);
    StageStep signal_eos(bool allow_wait);
    StageStep abort(const std::string& reason);

    Deps deps_;
    core::pipeline::SubmitSequencer sequencer_;

    /// 待送包。数据指向解封装器内部缓冲，**下一次 `read_next_sample` 即失效**，
    /// 所以只要有待送包就绝不能去读下一包 —— 这条由 `SubmitSequencer` 守。
    SampleView pending_{};
};

}  // namespace kivo::video::mediacodec
