#pragma once

#include <string>

#include "video/video_plane/backends/avsync/session/av_sync_stats.hpp"

namespace kivo::video::avsync {

/// 纯函数：`AvSyncStats` → `kivo.p12c.sync/1` 证据 JSON。
///
/// 为什么是纯函数：同一份输入在任何时刻都必须产出逐字节一致的文本，
/// 这样证据本身可以被 golden 测试压住，而不是"跑一次看一次"。
/// `generated_at_unix_ms` 由调用方填进 `stats`，渲染层不读时钟。
///
/// **独立复算契约**：`av_drift` 与 `present_jitter` 两个对象里除了分位数，
/// 还带完整分布（溢出计数 + 精确极值 + 非空桶计数 + 桶映射常量）。
/// 复算方据此重放 `SignedHistogram::percentile_ns()`，应当得到**逐位相同**的整数；
/// 实现见 `tools/verify_p12c_sync.py` 与单测 `C8-4`。
/// 只给汇总值的话，"可复算"就只是句口号 —— 读者只能选择相信。
[[nodiscard]] std::string render_av_sync_json(const AvSyncStats& stats);

}  // namespace kivo::video::avsync
