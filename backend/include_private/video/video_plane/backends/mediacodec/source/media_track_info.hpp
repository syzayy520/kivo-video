#pragma once

#include <cstdint>
#include <string>

namespace kivo::video::mediacodec {

/// 选中视频轨的静态信息。纯数据，不含平台句柄。
///
/// 之所以从 AMediaFormat 抽出来：上层（会话、日志、证据、后续的 A/V 同步）
/// 判断格式时只该看到 std::string / int，不该碰平台类型。
struct VideoTrackInfo {
    std::string mime;                 ///< 如 "video/hevc"
    std::int32_t width = 0;           ///< 编码宽（非显示宽）
    std::int32_t height = 0;
    std::int64_t duration_us = 0;     ///< 容器时长，可能为 -1（未知）
    std::int32_t rotation_degrees = 0;///< 0/90/180/270，来自容器元数据
};

}  // namespace kivo::video::mediacodec
