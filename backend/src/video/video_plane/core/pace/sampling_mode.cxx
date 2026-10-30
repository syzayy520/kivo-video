#include "video/video_plane/core/pace/sampling_mode.hpp"

namespace kivo::video::core::pace {

const char* sampling_mode_name(SamplingMode mode) noexcept {
    switch (mode) {
        case SamplingMode::kMediaTimeDecimated:
            return "media_time_decimated";
        case SamplingMode::kPerFrame:
            return "per_frame";
        case SamplingMode::kNotMeasured:
            return "not_measured";
    }
    // 走到这里只能是枚举被加了新值却没更新这张表。返回一个**明显不是合法口径**的
    // 字符串，让它在证据里一眼可见；返回 "unknown" 之类的"看起来像正常值"才是灾难。
    return "<unregistered-sampling-mode>";
}

bool comparable_across_frame_rates(SamplingMode mode) noexcept {
    return mode == SamplingMode::kMediaTimeDecimated;
}

bool decimated_block_is_usable(std::int32_t samples, std::int64_t span_ns) noexcept {
    // 两条都要：样本够说明"采到了不止一次"，跨度够说明"媒体时间真的推进了"。
    //
    // 只看 samples 会漏掉"两帧 pts 相同"（samples 2、span 0）；
    // 只看 span 会漏掉"一个点却报了个跨度"（不该发生，但结构上允许）。
    return samples >= kMinDecimatedSamples && span_ns > 0;
}

const char* decimated_unusable_reason(std::int32_t samples, std::int64_t span_ns) noexcept {
    // 可用的那一半也要有确定的答案：返回空串。
    // 若这里返回 "media time does not advance"，调用方在可用时误调一次，
    // 就会把一句假话写进证据 —— reason 的每一次非空返回都必须是真的。
    if (decimated_block_is_usable(samples, span_ns)) {
        return "";
    }
    if (samples <= 0) {
        return "no decimated sample was accepted";
    }
    if (samples < kMinDecimatedSamples) {
        // 单样本：媒体时刻一次都没跨过格点。缺 pts（全 0）会落在这里，
        // 帧率高于格点也会落在这里 —— **不猜是哪一个**，只陈述观测到的事实。
        return "media time did not reach the second 50 ms grid point (single sample)";
    }
    // 样本够但跨度为 0：pts 在重复，或两次接受落在同一媒体时刻。
    return "media time does not advance across the accepted samples";
}

}  // namespace kivo::video::core::pace
