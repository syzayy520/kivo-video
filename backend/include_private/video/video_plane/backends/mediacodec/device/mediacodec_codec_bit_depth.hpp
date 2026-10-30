#pragma once

#include <cstdint>
#include <string_view>

namespace kivo::video::mediacodec {

/// 由 (mime, profile) 推断该组合的每分量位深。
///
/// 返回值语义：
/// - 8 / 10 / 12：确定位深
/// - 0：无法从 profile 判断（例如 profile 未知、或该 profile 不规定位深）
///
/// 0 不等于「8 位」。调用方必须把 0 当作「未知」处理，
/// 不许当成 8 位去做能力判断（违反 G9「不许静默降级」）。
[[nodiscard]] std::int32_t infer_bit_depth(std::string_view mime, std::int32_t profile);

}  // namespace kivo::video::mediacodec
