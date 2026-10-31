#include "video/video_plane/core/pace/pace_lead_trace.hpp"

namespace kivo::video::core::pace {
namespace {

/// 帧数不足两帧：一帧只有基准，没有"变化"这种东西。
constexpr const char* kTooFewSamples =
    "fewer than two frames carried a clock reading, so the lead has no trajectory to read";

/// 调用方没给帧长：判"这一步超没超一帧"没有尺子。
///
/// 退回一个绝对毫秒数是错的：24/30/60 fps 的一帧差 2.5 倍，
/// 写死 20 ms 会让 60 fps 上的一次真跳变落进容差里。
constexpr const char* kNoFrameDuration =
    "frame duration was not supplied, so a single step cannot be told apart from a slow ramp";

/// 基准与末帧落在同一个墙钟时刻（跨度 0）：`rate_ppm` 没有分母。
constexpr const char* kNoSpan =
    "the baseline and the last frame share one wall-clock instant, so no rate can be derived";

constexpr const char* kStepped =
    "the lead moved by more than one frame between two consecutive presentations: the clock jumped";

constexpr const char* kRamped =
    "the lead grew past one frame without any single step: the clock is running at a different rate";

constexpr const char* kSteady =
    "the lead stayed within one frame of its baseline for the whole run";

[[nodiscard]] constexpr std::int64_t abs_of(std::int64_t value) noexcept {
    return value < 0 ? -value : value;
}

}  // namespace

std::int64_t LeadTraceRecorder::note(std::int64_t elapsed_ns, std::int64_t mono_ns) noexcept {
    // 首帧无条件立基准并返回 0。用"第一个带读数的帧"而不是"会话开始"：
    // 会话开始时音频可能还没起跑，`elapsed` 还没有意义（那是 P12-F2 的老坑）。
    if (!has_baseline_) {
        has_baseline_ = true;
        baseline_elapsed_ns_ = elapsed_ns;
        baseline_mono_ns_ = mono_ns;
        last_mono_ns_ = mono_ns;
        last_lead_ns_ = 0;
        max_lead_ns_ = 0;
        min_lead_ns_ = 0;
        samples_ = 1;
        return 0;
    }

    const std::int64_t lead =
        elapsed_ns - baseline_elapsed_ns_ - (mono_ns - baseline_mono_ns_);

    ++samples_;

    // 步长只在**相邻两帧**之间量。首帧没有前一帧，所以不计步 ——
    // 把"从 0 到第一帧"算成一步，会让每一轮都凭空多一次 step。
    if (has_previous_) {
        const std::int64_t step = abs_of(lead - previous_lead_ns_);
        if (step > max_step_ns_) {
            max_step_ns_ = step;
        }
        // 尺子没给就不数。**不退回默认阈值** —— 一个"看起来合理"的默认值
        // 会让 `step_frames` 在缺参数时给出一个可信的错数，比给 0 更糟。
        if (frame_duration_ns_ > 0 && step > frame_duration_ns_) {
            ++step_frames_;
        }
    }

    previous_lead_ns_ = lead;
    has_previous_ = true;
    last_lead_ns_ = lead;
    last_mono_ns_ = mono_ns;
    if (lead > max_lead_ns_) {
        max_lead_ns_ = lead;
    }
    if (lead < min_lead_ns_) {
        min_lead_ns_ = lead;
    }
    return lead;
}

LeadTrace LeadTraceRecorder::summary() const noexcept {
    LeadTrace out;
    out.samples = samples_;
    out.frame_duration_ns = frame_duration_ns_;
    out.baseline_elapsed_ns = baseline_elapsed_ns_;
    out.baseline_mono_ns = baseline_mono_ns_;
    out.last_ns = last_lead_ns_;
    out.max_ns = max_lead_ns_;
    out.min_ns = min_lead_ns_;
    out.max_step_ns = max_step_ns_;
    out.step_frames = step_frames_;
    out.span_ns = has_baseline_ ? (last_mono_ns_ - baseline_mono_ns_) : 0;
    if (out.span_ns > 0) {
        out.rate_ppm = static_cast<double>(last_lead_ns_) * 1.0e6 /
                       static_cast<double>(out.span_ns);
    }

    // 判定顺序即优先级：**能不能判**永远排在**判成什么**前面。
    // 反过来的话，缺帧长的轮次会因为 `step_frames == 0` 而落到 `kRamped` ——
    // 一个缺参数导致的假结论，且方向指向"钟在匀速跑偏"（要修硬件），
    // 而真相是"这次没量"。
    if (samples_ < 2) {
        out.verdict = LeadVerdict::kUnknown;
        out.verdict_reason = kTooFewSamples;
    } else if (frame_duration_ns_ <= 0) {
        out.verdict = LeadVerdict::kUnknown;
        out.verdict_reason = kNoFrameDuration;
    } else if (out.span_ns <= 0) {
        out.verdict = LeadVerdict::kUnknown;
        out.verdict_reason = kNoSpan;
    } else if (step_frames_ > 0) {
        out.verdict = LeadVerdict::kStepped;
        out.verdict_reason = kStepped;
    } else if (abs_of(last_lead_ns_) > frame_duration_ns_) {
        out.verdict = LeadVerdict::kRamped;
        out.verdict_reason = kRamped;
    } else {
        out.verdict = LeadVerdict::kSteady;
        out.verdict_reason = kSteady;
    }

    // 可测性与 verdict 同源，不在这里重判一遍 —— 重判会让
    // "verdict 说 steady、measured 说 false" 这种组合成为可能。
    out.measured = (out.verdict == LeadVerdict::kUnknown)
                       ? measured_missing(out.verdict_reason)
                       : measured_ok();
    return out;
}

}  // namespace kivo::video::core::pace
