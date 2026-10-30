#pragma once

#include <cstdint>

namespace kivo::video::mediacodec {

/// Android `AudioFormat` 的编码常量表（纯数据）。
///
/// 为什么不 include <media/AudioSystem.h> 或 <system/audio.h>：
/// NDK 没有把 `audio_format_t` 暴露给应用侧，`AMEDIAFORMAT_KEY_PCM_ENCODING`
/// 里装的就是这些 `AudioFormat.ENCODING_*` 的整数值。
/// 数值来自 `android.media.AudioFormat` 的公开常量，自 API 21 起未变。
namespace android_audio_encoding {

constexpr std::int32_t kInvalid = 0;
constexpr std::int32_t kDefault = 1;
constexpr std::int32_t kPcm16Bit = 2;
constexpr std::int32_t kPcm8Bit = 3;
constexpr std::int32_t kPcmFloat = 4;
constexpr std::int32_t kPcm24BitPacked = 21;
constexpr std::int32_t kPcm32Bit = 22;

}  // namespace android_audio_encoding

}  // namespace kivo::video::mediacodec
