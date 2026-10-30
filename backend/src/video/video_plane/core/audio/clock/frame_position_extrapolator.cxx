#include "video/video_plane/core/audio/clock/frame_position_extrapolator.hpp"

namespace kivo::video::core::audio {
namespace {

constexpr std::int64_t kNanosPerSecond = 1'000'000'000;

}  // namespace

FramePositionExtrapolator::FramePositionExtrapolator(std::int32_t sample_rate,
                                                     std::int64_t stale_after_ns) noexcept
    : rate_(sample_rate > 0 ? sample_rate : 48000),
      stale_after_ns_(stale_after_ns > 0 ? stale_after_ns : 8'000'000) {}

FramePositionExtrapolator::Result FramePositionExtrapolator::extrapolate(
    const Sample& sample, std::int64_t now_mono_ns) noexcept {
    Result result;

    if (sample.sample_rate > 0) {
        set_sample_rate(sample.sample_rate);
    }
    last_frame_ = sample.frame_position;

    const std::int64_t sampled_position_ns = position_of_frame(sample.frame_position, rate_);
    result.position_ns = sampled_position_ns;

    // now 早于时间戳：说明两把尺子不同源或调用方传错了。
    // 不外推（外推量取 0），也不判陈旧 —— 判陈旧会让时钟永久冻结，掩盖真正的原因。
    const std::int64_t elapsed_ns = now_mono_ns - sample.time_nanos;
    if (elapsed_ns < 0) {
        return result;
    }

    if (elapsed_ns > stale_after_ns_) {
        // 时间戳两个 burst 都没更新：设备多半没在推进。
        // 此时必须停止外推，否则时钟会无中生有地往前跑，
        // 同步器就去追一个根本不存在的进度。
        ++stale_freezes_;
        result.stale = true;
        return result;
    }

    result.extrapolated_ns = elapsed_ns;
    result.position_ns = sampled_position_ns + elapsed_ns;
    return result;
}

bool FramePositionExtrapolator::set_sample_rate(std::int32_t sample_rate) noexcept {
    if (sample_rate <= 0 || sample_rate == rate_) {
        return false;
    }

    // 先用旧速率把"最后已知帧位置"换算成位置，再把它当作新基准。
    // 顺序不能反：先改 rate_ 再算，就会用新速率解释旧帧数，位置当场跳变。
    const std::int64_t anchor_ns = position_of_frame(last_frame_, rate_);
    base_frame_ = last_frame_;
    base_position_ns_ = anchor_ns;
    rate_ = sample_rate;
    return true;
}

void FramePositionExtrapolator::rebase(std::int64_t media_position_ns) noexcept {
    // 基准锚在"当前流的最后一帧"上：新流的第 0 帧就是 media_position_ns 对应的内容，
    // 所以重设后立刻读到的位置就是 media_position_ns，之后随帧增量前进。
    base_frame_ = last_frame_;
    base_position_ns_ = media_position_ns;
}

void FramePositionExtrapolator::set_stale_after_ns(std::int64_t stale_after_ns) noexcept {
    if (stale_after_ns > 0) {
        stale_after_ns_ = stale_after_ns;
    }
}

std::int64_t FramePositionExtrapolator::position_of_frame(std::int64_t frame,
                                                          std::int32_t rate) const noexcept {
    const std::int64_t delta = frame - base_frame_;

    // 拆成"整秒 + 余数"再乘 1e9：直接写 delta * 1e9 在长片源上会溢出 int64
    // （播放 13 小时左右就到顶），而余数永远小于 rate，乘 1e9 最多 1.9e14，绝不溢出。
    const std::int64_t whole_seconds = delta / rate;
    const std::int64_t remainder = delta % rate;

    return base_position_ns_ + whole_seconds * kNanosPerSecond +
           remainder * kNanosPerSecond / rate;
}

}  // namespace kivo::video::core::audio
