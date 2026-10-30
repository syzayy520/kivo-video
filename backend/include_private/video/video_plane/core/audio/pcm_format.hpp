#pragma once

#include <cstdint>
#include <string_view>

namespace kivo::video::core::audio {

/// 交错 PCM 的采样编码。
///
/// 只列本播放器真正会碰到的三种，不做成通用音频格式库：
/// Android 的 MediaCodec 音频解码器默认输出 16bit 定点，
/// AAudio 与 iOS 的 AudioUnit 都要 float32，32bit 定点偶见于高解析音轨。
enum class PcmEncoding : std::int32_t {
    kUnknown = 0,
    kInt16 = 1,
    kInt32 = 2,
    kFloat32 = 3,
};

/// PCM 描述。纯数据，**不含任何平台类型** —— 上层判断格式只该看到整数与枚举。
///
/// 为什么单独成一层而不是直接用 AMediaFormat：
/// iOS 那边没有 AMediaFormat，桌面端的 WASAPI 也没有；
/// 一旦让上层拿到平台格式，三端就得各写一套判断。
struct PcmFormat {
    std::int32_t sample_rate = 0;
    std::int32_t channel_count = 0;
    PcmEncoding encoding = PcmEncoding::kUnknown;

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return sample_rate > 0 && channel_count > 0 && encoding != PcmEncoding::kUnknown;
    }

    /// 单个样本占多少字节（一个声道一次采样的字节数）。未知编码返回 0。
    [[nodiscard]] constexpr std::int32_t bytes_per_sample() const noexcept {
        switch (encoding) {
            case PcmEncoding::kInt16:
                return 2;
            case PcmEncoding::kInt32:
            case PcmEncoding::kFloat32:
                return 4;
            case PcmEncoding::kUnknown:
                break;
        }
        return 0;
    }

    /// 一「帧」= 同一时刻所有声道各一个样本。未知编码返回 0。
    [[nodiscard]] constexpr std::int32_t frame_size_bytes() const noexcept {
        return bytes_per_sample() * channel_count;
    }
};

/// 编码 → 可读名。用于证据 JSON 与错误信息，不参与任何判断。
[[nodiscard]] constexpr std::string_view pcm_encoding_name(PcmEncoding encoding) noexcept {
    switch (encoding) {
        case PcmEncoding::kInt16:
            return "pcm_s16le";
        case PcmEncoding::kInt32:
            return "pcm_s32le";
        case PcmEncoding::kFloat32:
            return "pcm_f32le";
        case PcmEncoding::kUnknown:
            break;
    }
    return "unknown";
}

}  // namespace kivo::video::core::audio
