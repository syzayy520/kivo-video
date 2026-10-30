#include "video/video_plane/core/audio/pcm_format_converter.hpp"

#include <cstring>

namespace kivo::video::core::audio {
namespace {

// 定点 → 单位浮点的分母用 2 的幂而不是 32767 / 2147483647：
// 前者可逆且两端对称（-32768 → -1.0，+32767 → 0.99997），
// 后者会让 +满幅被夹到 1.0 而在回程产生削波。
constexpr float kInt16Scale = 1.0F / 32768.0F;
constexpr float kInt32Scale = 1.0F / 2147483648.0F;

/// 逐样本用 memcpy 而不是 reinterpret_cast：
/// 解码器给出的缓冲不保证按样本宽度对齐，直接解引用在 ARM 上会 UB
/// （非对齐 int16 访问被允许但结果依赖实现，int32/float 则可能直接异常）。
/// 两个字节的 memcpy 在 aarch64 与 x86 上都会被编译成一条 load。
[[nodiscard]] std::int16_t load_int16(const std::uint8_t* at) noexcept {
    std::int16_t value = 0;
    std::memcpy(&value, at, sizeof(value));
    return value;
}

[[nodiscard]] std::int32_t load_int32(const std::uint8_t* at) noexcept {
    std::int32_t value = 0;
    std::memcpy(&value, at, sizeof(value));
    return value;
}

}  // namespace

PcmConversionOutcome convert_to_float(const PcmFormat& source, const std::uint8_t* src,
                                      std::size_t src_bytes, float* dst,
                                      std::int32_t dst_capacity_frames) {
    if (!source.is_valid()) {
        return {PcmConversionOutcome::Code::kInvalidFormat, 0,
                "source pcm format is incomplete (rate=" + std::to_string(source.sample_rate) +
                    " channels=" + std::to_string(source.channel_count) +
                    " encoding=" + std::string(pcm_encoding_name(source.encoding)) + ")"};
    }

    const auto frame_size = static_cast<std::size_t>(source.frame_size_bytes());
    if (frame_size == 0) {
        return {PcmConversionOutcome::Code::kInvalidFormat, 0,
                "frame size resolved to zero for a valid-looking format"};
    }
    if (src_bytes % frame_size != 0) {
        return {PcmConversionOutcome::Code::kPartialFrame, 0,
                "source byte count " + std::to_string(src_bytes) + " is not a multiple of frame size " +
                    std::to_string(frame_size)};
    }

    const auto frames = static_cast<std::int32_t>(src_bytes / frame_size);
    if (frames > dst_capacity_frames) {
        return {PcmConversionOutcome::Code::kOutputTooSmall, 0,
                "destination holds " + std::to_string(dst_capacity_frames) + " frames, need " +
                    std::to_string(frames)};
    }
    if (frames == 0) {
        return {};
    }
    if (src == nullptr || dst == nullptr) {
        return {PcmConversionOutcome::Code::kNullPointer, 0, "null sample buffer"};
    }

    const auto sample_count = static_cast<std::size_t>(frames) * static_cast<std::size_t>(source.channel_count);

    switch (source.encoding) {
        case PcmEncoding::kFloat32:
            std::memcpy(dst, src, sample_count * sizeof(float));
            break;

        case PcmEncoding::kInt16: {
            const std::uint8_t* cursor = src;
            for (std::size_t i = 0; i < sample_count; ++i, cursor += sizeof(std::int16_t)) {
                dst[i] = static_cast<float>(load_int16(cursor)) * kInt16Scale;
            }
            break;
        }

        case PcmEncoding::kInt32: {
            const std::uint8_t* cursor = src;
            for (std::size_t i = 0; i < sample_count; ++i, cursor += sizeof(std::int32_t)) {
                dst[i] = static_cast<float>(load_int32(cursor)) * kInt32Scale;
            }
            break;
        }

        case PcmEncoding::kUnknown:
            // is_valid() 已拦掉，这里只是让 switch 完整，不留 -Wswitch 警告。
            return {PcmConversionOutcome::Code::kInvalidFormat, 0, "unknown source encoding"};
    }

    return {PcmConversionOutcome::Code::kOk, frames, {}};
}

}  // namespace kivo::video::core::audio
