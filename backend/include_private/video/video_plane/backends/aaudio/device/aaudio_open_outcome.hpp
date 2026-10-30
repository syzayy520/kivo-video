#pragma once

#include <cstdint>
#include <string>

namespace kivo::video::aaudio {

/// 一次开流的真实结果。
///
/// 与 mediacodec 的 code+message 约定保持一致：失败必须能说清原因（G9）。
///
/// `downgrade_reason` 是这里特有的字段：降级**仍然是成功**，但必须留下痕迹。
/// 否则「我申请了独占却没拿到」这件事会永远消失在黑箱里，
/// 而独占与共享的延迟差正是 G7 能不能达标的前提 —— 不记录就等于假装没发生。
struct AaudioOpenOutcome {
    enum class Code : std::int32_t {
        kOk = 0,
        kInvalidConfig = 1,
        kBuilderFailed = 2,
        kOpenFailed = 3,
        kStreamInvalid = 4,
    };

    Code code = Code::kOk;
    std::string message;          ///< code != kOk 时非空
    std::string downgrade_reason; ///< 成功但发生降级时非空

    // ---- 实际协商结果（平台枚举原值，可读名见 aaudio_format_names）----
    std::int32_t sample_rate = 0;
    std::int32_t channel_count = 0;
    std::int32_t platform_format = 0;      ///< aaudio_format_t 原值
    std::int32_t platform_sharing_mode = 0;///< aaudio_sharing_mode_t 原值
    std::int32_t platform_performance_mode = 0; ///< aaudio_performance_mode_t 原值
    std::int32_t frames_per_burst = 0;
    std::int32_t buffer_capacity_frames = 0;

    [[nodiscard]] bool ok() const { return code == Code::kOk; }
    [[nodiscard]] bool downgraded() const { return !downgrade_reason.empty(); }
};

}  // namespace kivo::video::aaudio
