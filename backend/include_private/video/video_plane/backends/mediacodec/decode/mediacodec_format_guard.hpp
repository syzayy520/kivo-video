#pragma once

#include <string>

#include <media/NdkMediaFormat.h>

namespace kivo::video::mediacodec {

struct DecoderFormatOutcome {
    enum class Code {
        kOk = 0,
        kNullFormat = 1,
        kMissingMime = 2,
        kMissingGeometry = 3,
        kMissingCodecConfig = 4,
    };

    Code code = Code::kOk;
    std::string message;

    [[nodiscard]] bool ok() const { return code == Code::kOk; }
};

/// 在把轨道格式交给 AMediaCodec_configure 之前做一次把关。
///
/// **不做拷贝。** NDK 的 AMediaFormat_copy 是 API 29 才有的，minSdk 26 用不了；
/// 而 configure 本身不接管格式所有权，直接用解封装器给的格式就是正确做法。
/// 所以本单元只做两件事：校验必需字段、把失败原因说清楚。
///
/// 校验项：
/// - mime 存在且以 "video/" 开头
/// - width / height 为正
/// - csd-0 存在（H.264/HEVC 没有 SPS/PPS 时 configure 之后的报错极难定位，
///   必须在进平台层之前拦住）
[[nodiscard]] DecoderFormatOutcome validate_decoder_format(const AMediaFormat* format);

/// 把格式渲染成一行可读文本，用于日志与证据。
///
/// 基于平台自己的 AMediaFormat_toString，因此看到的就是平台真实认知的字段，
/// 不会因为我们漏读某个键而失真。
[[nodiscard]] std::string describe_decoder_format(const AMediaFormat* format);

}  // namespace kivo::video::mediacodec
