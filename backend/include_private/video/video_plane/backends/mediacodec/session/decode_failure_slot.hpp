#pragma once

#include <mutex>
#include <string>

namespace kivo::video::mediacodec {

/// 失败原因槽：流水线里任何一段出事都往这里写，会话层只负责读。
///
/// **首次写入为准**，后到的不覆盖。理由：两段可能几乎同时失败
/// （一段的失败往往让另一段的平台调用也返回错误），而"最后那句话"
/// 通常是连带反应（`dequeueOutputBuffer returned -1`），根因会被它盖掉。
///
/// 字符串只在这条冷路径上出现，热路径一次都不碰。
class DecodeFailureSlot {
public:
    void mark(const std::string& reason);

    /// 一旦失败就为 true，此后不再变回 false。
    [[nodiscard]] bool failed() const;

    /// 空串表示没失败。G9 要求失败必须带可读原因，所以这里不允许写空串。
    [[nodiscard]] std::string text() const;

private:
    mutable std::mutex mutex_;
    std::string reason_;
};

}  // namespace kivo::video::mediacodec
