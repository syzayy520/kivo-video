#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "video/video_plane/core/audio/pcm_format.hpp"

namespace kivo::video::core::audio {

/// PCM 转换结果。沿用设备层"code + message"的约定：失败必须能说清原因。
struct PcmConversionOutcome {
    enum class Code : std::int32_t {
        kOk = 0,
        kInvalidFormat = 1,     ///< 源格式本身不完整（采样率/声道/编码缺项）
        kPartialFrame = 2,      ///< 字节数不是整帧，继续转会让声道错位
        kOutputTooSmall = 3,    ///< 目标容量不够，宁可整块失败也不截断
        kNullPointer = 4,
    };

    Code code = Code::kOk;
    std::int32_t frames_written = 0;
    std::string message;

    [[nodiscard]] bool ok() const { return code == Code::kOk; }
};

/// 把交错 PCM 转成交错 float32（单位幅度 [-1, 1]）。
///
/// 契约（违反即显式失败，不做"尽力而为"）：
///   - `src_bytes` 必须是 `source.frame_size_bytes()` 的整数倍；
///   - `dst_capacity_frames` 必须 ≥ 实际帧数；
///   - float32 源走整块拷贝，不做逐样本乘法。
///
/// 明确不做：重采样、声道映射、抖动。采样率或声道数不符由调用方判定失败，
/// 在音频路径上"顺手转一下"是最典型的静默降质。
[[nodiscard]] PcmConversionOutcome convert_to_float(const PcmFormat& source, const std::uint8_t* src,
                                                    std::size_t src_bytes, float* dst,
                                                    std::int32_t dst_capacity_frames);

}  // namespace kivo::video::core::audio
