#pragma once

#include "video/video_plane/backends/mediacodec/session/decode_pace_stats.hpp"
#include "video/video_plane/core/pace/pace_stats.hpp"

namespace kivo::video::mediacodec {

/// `PaceSummary` → `DecodePaceStats`。
///
/// 单独成文件的原因是职责边界：节拍器不该知道"证据长什么样"，
/// 证据也不该去 include 直方图实现。转换只有这一处，改口径就改这里。
[[nodiscard]] DecodePaceStats decode_pace_stats_from(const core::pace::PaceSummary& summary,
                                                    std::int64_t snapshot_at_frames);

}  // namespace kivo::video::mediacodec
