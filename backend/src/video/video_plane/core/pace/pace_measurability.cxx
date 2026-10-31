#include "video/video_plane/core/pace/pace_measurability.hpp"

namespace kivo::video::core::pace {

MeasuredCounter measured_missing(const char* reason) noexcept {
    // 空理由 / 空指针一律换成那句证词，而不是留空。
    //
    // 留空等于「没量到，但我不说为什么」—— 读者只能猜，而猜出来的通常是
    // 「大概是没触发吧」。判据 P11 专门抓这句话，所以它一定会被看见，
    // 而不是像空白一样被跳过。
    if (reason == nullptr || reason[0] == '\0') {
        return MeasuredCounter{false, kUnspecifiedReason};
    }
    return MeasuredCounter{false, reason};
}

}  // namespace kivo::video::core::pace
