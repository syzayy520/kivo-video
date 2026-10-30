#pragma once

#include <string>

#include <media/NdkMediaCodec.h>

namespace kivo::video::mediacodec {

/// 取平台实际选中的 codec 名。
///
/// `AMediaCodec_getName` 是 API 28 才引入的，而我们的 minSdk 是 26。
/// 因此这里走**运行时符号解析**而不是直接调用：
/// - API >= 28：返回真实名字（如 "OMX.qcom.video.decoder.hevc"）
/// - API 26/27：返回空串，调用方自行降级（不抛异常、不崩）
///
/// 为什么值得单独成单元：这个值只用于日志与证据，**不得参与任何逻辑判断**
/// （见 10-p12a-evidence.md 铁律 1：绝不按 codec 名选解码器）。
/// 把它隔离出来，等于在代码结构上标出「这里的信息仅供参考」。
[[nodiscard]] std::string resolve_codec_name(AMediaCodec* codec);

}  // namespace kivo::video::mediacodec
