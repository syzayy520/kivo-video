#pragma once

#include <cstdint>
#include <string>

namespace kivo::video::mediacodec {

/// 解封装结果码。与设备层的 code+message 约定保持一致：
/// 失败必须能说清原因，不许只有 false（G9）。
struct ExtractOutcome {
    enum class Code : std::int32_t {
        kOk = 0,
        kOpenFailed = 1,
        kNoVideoTrack = 2,
        kFormatUnreadable = 3,
        kTrackSelectFailed = 4,
        kNoAudioTrack = 5,
    };

    Code code = Code::kOk;
    std::string message;

    [[nodiscard]] bool ok() const { return code == Code::kOk; }
};

}  // namespace kivo::video::mediacodec
