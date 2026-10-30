#pragma once

#include <cstdint>

namespace kivo::video::mediacodec {

/// 当前 UTC 毫秒时间戳。
///
/// 单独成文件是为了让渲染层保持纯函数：renderer 接收时间戳而不是自己去读时钟，
/// 这样同一份清单在任何时刻都能渲染出逐字节一致的 JSON（可直接做 golden 测试）。
[[nodiscard]] std::int64_t now_unix_millis();

}  // namespace kivo::video::mediacodec
