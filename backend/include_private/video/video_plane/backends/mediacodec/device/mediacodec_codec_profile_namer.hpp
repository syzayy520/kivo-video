#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace kivo::video::mediacodec {

/// 把平台 profile 常量翻译成可读名字（"hevc-main10"、"avc-high" 等）。
///
/// 命名约定：`<codec>-<profile>`，全程小写。
/// 未收录的 profile 返回 "unknown(<原始值>)" —— 保留原始值便于回溯，
/// 不允许返回空串或 "unknown" 而丢掉信息。
[[nodiscard]] std::string describe_profile(std::string_view mime, std::int32_t profile);

/// 把平台 level 常量翻译成可读名字（"hevc-high-tier-6.1"、"avc-6" 等）。
///
/// level 的编码按 mime 各自独立，必须带 mime 查询。
[[nodiscard]] std::string describe_level(std::string_view mime, std::int32_t level);

}  // namespace kivo::video::mediacodec
