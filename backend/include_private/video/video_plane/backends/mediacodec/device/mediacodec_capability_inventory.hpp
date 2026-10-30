#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "video/video_plane/backends/mediacodec/device/mediacodec_codec_record.hpp"

namespace kivo::video::mediacodec {

/// 去重、排序并按 mime 归并后的解码器清单。
///
/// 纯数据聚合结果：只承载事实与计数，不参与任何选择决策。
/// 选哪个解码器是 decode 阶段的事，这里不预判。
struct CapabilityInventory {
    std::vector<CodecInventoryRecord> records;  ///< 已按 (mime, name, profile) 排序
    std::size_t raw_record_count = 0;           ///< Java 侧传入的原始条数（未去重）
    std::size_t rejected_records = 0;           ///< 解析失败条数，必须保留而非丢弃
    std::size_t decoder_records = 0;            ///< 去重后的视频解码器条目数
    std::size_t hardware_decoder_records = 0;   ///< 其中 is_hardware == 1 的条目数
    std::size_t tenbit_decoder_records = 0;     ///< 其中位深 >= 10 的条目数

    /// 出现过的 mime 列表（去重，升序）。用 vector 而非 set：JSON 要求有序输出，
    /// 且调用方只遍历不查找。
    std::vector<std::string> mimes;
};

/// 由 Java 侧摊平的原始记录构建清单。
///
/// 去重键为 (name, mime, profile, level) —— 同一 codec 对不同 level 是不同能力点，
/// 不能合并，否则会丢掉能力明细。
[[nodiscard]] CapabilityInventory build_capability_inventory(
    const std::vector<std::string>& raw_records);

}  // namespace kivo::video::mediacodec
