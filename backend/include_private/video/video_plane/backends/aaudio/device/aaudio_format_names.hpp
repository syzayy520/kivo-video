#pragma once

#include <cstdint>
#include <string_view>

namespace kivo::video::aaudio {

/// 平台枚举 → 可读名。纯函数，不读设备、不落盘、不查表以外的事。
///
/// 集中成一个单元，是为了让证据 JSON 里的字符串只有一个来源。
/// 以前 mediacodec 那边就是因为命名分散，出现过 AVC Constrained 落成 unknown 的缺口。
[[nodiscard]] std::string_view platform_format_name(std::int32_t format);
[[nodiscard]] std::string_view sharing_mode_name(std::int32_t mode);
[[nodiscard]] std::string_view performance_mode_name(std::int32_t mode);

/// AAudio 结果码 → 可读名。失败原因要能直接进 JSON，不许只留一个数字。
[[nodiscard]] std::string_view result_name(std::int32_t result);

}  // namespace kivo::video::aaudio
