#pragma once

#include <string_view>

#include "video/video_plane/backends/mediacodec/device/mediacodec_codec_record.hpp"

namespace kivo::video::mediacodec {

/// 解析一条 TAB 分隔记录（字段顺序见 CodecInventoryRecord）。
///
/// 契约：
/// - 字段数不符 → false
/// - 任一数值字段非法或溢出 → false
/// - 不抛异常，不修改 out 之外的状态
///
/// 返回 false 的调用方必须计入 rejected 并保留证据，不许静默跳过（G9）。
[[nodiscard]] bool parse_codec_record(std::string_view raw, CodecInventoryRecord& out);

}  // namespace kivo::video::mediacodec
