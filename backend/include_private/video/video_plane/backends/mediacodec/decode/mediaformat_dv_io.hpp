#pragma once

#include <cstdint>
#include <vector>

#include <media/NdkMediaFormat.h>

#include "video/video_plane/backends/mediacodec/decode/dv_config_facts.hpp"

namespace kivo::video::mediacodec {

/// DV 键的**读数层**（210）。与 `mediaformat_color_io` 同款分工：
/// 这里只负责"从 `AMediaFormat` 里读出来"，键值含义的解读在 `dv_config_facts`。
/// 平台类型 `AMediaFormat` 只在 Android 上存在，所以这一层无法在桌面上单测
/// —— 判定规则全部放在 `dv_config_facts`（纯逻辑）里钉住。
///
/// 键名注记：`profile` / `level` / `bl_compatibility_id` 一律用**字面量** ——
/// `AMEDIAFORMAT_KEY_PROFILE` / `AMEDIAFORMAT_KEY_LEVEL` 是 API 28 才导出的符号，
/// minSdk = 26 编译不过（首版构建撞过一次）；字面量是本工程对 API 28 符号的
/// 既定处理（`"csd-0"` 同款先例）。
[[nodiscard]] DvTrackKeys read_dv_track_keys(const AMediaFormat* track_format);

/// 读 csd-0 全部字节（BL 编码嗅探用）。键缺席或空 → false。
/// `AMediaFormat_getBuffer` 是 API 28 符号：与 `mediaformat_color_io.cxx` 同一条
/// "本路径只支持 API 28+" 的约定，见那里的注释。
[[nodiscard]] bool read_csd0_bytes(const AMediaFormat* track_format,
                                   std::vector<std::uint8_t>& out);

}  // namespace kivo::video::mediacodec
