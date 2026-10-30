#include "video/video_plane/backends/mediacodec/session/decode_failure_slot.hpp"

namespace kivo::video::mediacodec {

void DecodeFailureSlot::mark(const std::string& reason) {
    const std::lock_guard<std::mutex> guard(mutex_);
    if (!reason_.empty()) {
        return;  // 首次写入为准
    }
    // 空原因不许变成"写了等于没写"：留一句占位，至少让证据里能看到"这里失败过"。
    reason_ = reason.empty() ? "unspecified failure (no reason given)" : reason;
}

bool DecodeFailureSlot::failed() const {
    const std::lock_guard<std::mutex> guard(mutex_);
    return !reason_.empty();
}

std::string DecodeFailureSlot::text() const {
    const std::lock_guard<std::mutex> guard(mutex_);
    return reason_;
}

}  // namespace kivo::video::mediacodec
