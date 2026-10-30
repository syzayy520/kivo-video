#pragma once

#include <cstdint>

#include "video/video_plane/core/audio/pcm_format.hpp"

#include <media/NdkMediaFormat.h>

namespace kivo::video::mediacodec {

/// 平台 PCM 编码整数 → 中立编码。无法识别时返回 `kUnknown`（由调用方显式失败）。
[[nodiscard]] core::audio::PcmEncoding map_audio_encoding(std::int32_t platform_encoding);

/// 从 `AMediaCodec` 的**输出**格式里读出 PCM 描述。**纯函数**，不持有任何句柄。
///
/// 关键约定：`pcm-encoding` 缺失时按 **16bit 定点**处理。
/// 这是 MediaCodec 音频解码器的既定默认值（文档明写），不是猜测；
/// 若把它当"未知"处理，AAC 播放会在每台设备上都失败。
[[nodiscard]] core::audio::PcmFormat read_output_pcm_format(const AMediaFormat* format);

}  // namespace kivo::video::mediacodec
