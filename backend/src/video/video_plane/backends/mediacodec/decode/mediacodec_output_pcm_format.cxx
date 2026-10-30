#include "video/video_plane/backends/mediacodec/decode/mediacodec_output_pcm_format.hpp"

#include "video/video_plane/backends/mediacodec/decode/android_audio_encoding_ids.hpp"

namespace kivo::video::mediacodec {
namespace {

// `AMEDIAFORMAT_KEY_PCM_ENCODING` 是 API 28 才导出的常量，而 minSdk 是 26。
// 键名自 API 21 起就稳定，直接用字面量（与 "rotation" 同一处理方式）。
constexpr const char* kKeyPcmEncoding = "pcm-encoding";

/// NDK 的 getInt32/getInt64 接的是非 const 指针（只读操作，签名却是可写的），
/// 这里集中转换一次，不给每个调用点散落 const_cast。
[[nodiscard]] std::int32_t read_int32(const AMediaFormat* format, const char* key,
                                      std::int32_t fallback) {
    std::int32_t value = 0;
    if (!AMediaFormat_getInt32(const_cast<AMediaFormat*>(format), key, &value)) {
        return fallback;
    }
    return value;
}

}  // namespace

core::audio::PcmEncoding map_audio_encoding(std::int32_t platform_encoding) {
    switch (platform_encoding) {
        case android_audio_encoding::kPcm16Bit:
            return core::audio::PcmEncoding::kInt16;
        case android_audio_encoding::kPcmFloat:
            return core::audio::PcmEncoding::kFloat32;
        case android_audio_encoding::kPcm32Bit:
            return core::audio::PcmEncoding::kInt32;
        // 8bit 与 24bit packed 不在本播放器的支持范围：前者动态范围不足，
        // 后者本机 AAC 解码器不会输出。返回 kUnknown 让调用方显式失败，
        // 而不是塞进一个"看起来能播"的转换分支。
        case android_audio_encoding::kPcm8Bit:
        case android_audio_encoding::kPcm24BitPacked:
        case android_audio_encoding::kInvalid:
        case android_audio_encoding::kDefault:
            break;
    }
    return core::audio::PcmEncoding::kUnknown;
}

core::audio::PcmFormat read_output_pcm_format(const AMediaFormat* format) {
    core::audio::PcmFormat pcm;
    if (format == nullptr) {
        return pcm;
    }

    pcm.sample_rate = read_int32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, 0);
    pcm.channel_count = read_int32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, 0);

    std::int32_t encoding = 0;
    if (AMediaFormat_getInt32(const_cast<AMediaFormat*>(format), kKeyPcmEncoding, &encoding)) {
        pcm.encoding = map_audio_encoding(encoding);
    } else {
        // 键缺失 = 解码器的默认输出 = 16bit 定点。这是文档行为，不是兜底猜测。
        pcm.encoding = core::audio::PcmEncoding::kInt16;
    }
    return pcm;
}

}  // namespace kivo::video::mediacodec
