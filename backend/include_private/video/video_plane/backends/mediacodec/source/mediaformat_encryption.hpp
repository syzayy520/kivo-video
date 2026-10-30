#pragma once

#include "video/video_plane/backends/mediacodec/source/encryption_facts.hpp"

struct AMediaFormat;

namespace kivo::video::mediacodec {

/// 从轨道格式里读加密事实（P12-F3B）。
///
/// 唯一碰平台类型的一层，所以它无法在桌面上单测（`AMediaFormat` 只在 Android 上存在）。
/// 判定的部分在 `assess_encryption()`，那一层有单测。
///
/// **只用 `int32` 的键**：`crypto-key` 是 data 类型，读它要 `AMediaFormat_getBuffer`，
/// 那是 API 28，而 minSdk = 26 —— 直接调会在 26/27 上加载期解析失败（"一装就崩"，
/// 不是"少个功能"）。所以 `crypto-mode` 作主判据，见 `51-p12f3b-design.md` §二。
[[nodiscard]] TrackEncryption read_track_encryption(const AMediaFormat* format);

}  // namespace kivo::video::mediacodec
