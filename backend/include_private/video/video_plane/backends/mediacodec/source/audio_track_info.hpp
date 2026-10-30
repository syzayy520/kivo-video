#pragma once

#include <cstdint>
#include <string>

namespace kivo::video::mediacodec {

/// 选中音频轨的静态信息。纯数据，不含平台句柄。
///
/// `sample_rate` / `channel_count` 取自**容器里的轨道格式**，不是解码后的输出格式。
/// 两者在 HE-AAC 这类隐式 SBR 的编码上可能不同（核心采样率是输出的一半），
/// 因此解码器收到第一块 PCM 后必须再读一次输出格式并与之核对，
/// 不一致要显式失败而不是"能用就行"——采样率错了，音高与时长全错。
struct AudioTrackInfo {
    std::string mime;              ///< 如 "audio/mp4a-latm"
    std::int32_t sample_rate = 0;  ///< 轨道声明的采样率
    std::int32_t channel_count = 0;
    std::int64_t duration_us = -1; ///< 容器时长，-1 表示未知
};

}  // namespace kivo::video::mediacodec
