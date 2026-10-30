#pragma once

#include <cstdint>

namespace kivo::video::mediacodec {

/// 轨道格式里的颜色标记（P12-E2）。**纯数据，不含 `AMediaFormat`** ——
/// 取值（读格式 / 写格式）在 `decode/mediaformat_color_io` 做，判定在
/// `decode/video_color_facts` 做。分开的理由与 E1 的 `VideoColorFacts` 同一条：
/// 判定要能在桌面上单测，而 `AMediaFormat` 只在 Android 上存在。
struct SourceColorTags {
    std::int32_t standard = -1;          ///< `color-standard`；-1 = 键不存在
    std::int32_t transfer = -1;          ///< `color-transfer`
    std::int32_t range = -1;             ///< `color-range`
    std::int32_t static_info_bytes = -1;  ///< `hdr-static-info` 字节数；-1 = 键不存在

    /// 有没有任何颜色标记。没有的话"下发"无事可做 ——
    /// 这时造一份显式副本与直接传原格式等价，只是多一次 configure 失败的机会。
    [[nodiscard]] bool any() const {
        return standard >= 0 || transfer >= 0 || range >= 0 || static_info_bytes >= 0;
    }
};

}  // namespace kivo::video::mediacodec
