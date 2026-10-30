#pragma once

#include <cstdint>
#include <string>

#include "video/video_plane/backends/mediacodec/session/decode_stats.hpp"

namespace kivo::video::mediacodec {

/// 把会话统计渲染成 JSON（schema: kivo.p12b.decode/1）。
///
/// 纯函数：不读时钟、不读环境，同一份 stats 必然渲染出同一份文本。
[[nodiscard]] std::string render_decode_stats_json(const DecodeStats& stats,
                                                   std::int64_t generated_at_unix_ms);

[[nodiscard]] const char* decode_schema_id();

}  // namespace kivo::video::mediacodec
