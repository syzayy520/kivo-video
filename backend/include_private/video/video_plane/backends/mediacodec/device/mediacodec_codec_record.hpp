#pragma once

#include <cstdint>
#include <string>

namespace kivo::video::mediacodec {

/// Java 侧 `MediaCodecInventory` 摊平后的一条记录。
///
/// 字段顺序由 `MediaCodecInventory.buildRecord` 保证，改动需两侧同步。
/// 本结构只承载数据，不含任何解析或判断逻辑 —— 那是 parser 与各分类器的职责。
struct CodecInventoryRecord {
    std::string name;               ///< 平台 codec 名，如 "c2.qti.avc.decoder"
    std::string mime;               ///< 如 "video/hevc"
    bool is_encoder = false;
    std::int32_t profile = -1;      ///< -1 表示平台未提供
    std::int32_t level = -1;        ///< -1 表示平台未提供
    std::int32_t max_width = -1;    ///< -1 表示平台未提供
    std::int32_t max_height = -1;   ///< -1 表示平台未提供
    std::int32_t is_hardware = -1;  ///< 1 硬解 / 0 非硬解 / -1 平台未提供
    std::int32_t is_software_only = -1;
};

}  // namespace kivo::video::mediacodec
