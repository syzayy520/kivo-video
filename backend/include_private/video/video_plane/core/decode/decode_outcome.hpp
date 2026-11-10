#pragma once

#include <cstdint>
#include <string>

namespace kivo::video::core::decode {

/// 解码动作的结果。submit / receive / configure 等动作共用。
///
/// 单独成文件而不是挂在某个解码器头里：视频解码器与音频解码器都要用它，
/// 让音频去 include 视频解码器的头，依赖关系就变成"音频依赖视频"，
/// 与家族树的语义完全不符。
struct DecodeOutcome {
    enum class Code {
        kOk = 0,
        kTryAgain = 1,      ///< 平台暂时不收/不吐，调用方应稍后重试（不是错误）
        kEndOfStream = 2,   ///< 解码器已吐完
        kFailed = 3,
    };

    Code code = Code::kOk;
    std::string message;

    [[nodiscard]] bool ok() const { return code == Code::kOk; }
    [[nodiscard]] bool try_again() const { return code == Code::kTryAgain; }
    [[nodiscard]] bool end_of_stream() const { return code == Code::kEndOfStream; }
};

}  // namespace kivo::video::core::decode
