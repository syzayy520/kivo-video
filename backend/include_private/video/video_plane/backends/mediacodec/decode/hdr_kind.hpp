#pragma once

#include <cstdint>

namespace kivo::video::mediacodec {

/// 片源的 HDR 类型。判定只看色彩键，不看 codec profile ——
/// profile 是编码侧的声明，这里要判的是**显示侧该按什么处理**。
///
/// 归属说明（P12-E2）：这个枚举原本写在 `present/surface_dataspace.hpp` 里，
/// 但它的产生者是 `decode/` 的颜色判定，消费者才是 `present/` 的 dataspace。
/// 放在 `present/` 会逼 `decode/` 反向依赖上屏层，所以提到 `decode/` 这一层。
/// `present/surface_dataspace.hpp` 仍然 include 它，API 不变。
enum class HdrKind : std::int32_t {
    kSdr = 0,
    kHdr10 = 1,  ///< transfer = ST2084 (PQ)，standard = BT2020
    kHlg = 2,    ///< transfer = HLG，standard = BT2020
};

[[nodiscard]] inline const char* hdr_kind_name(HdrKind kind) noexcept {
    switch (kind) {
        case HdrKind::kSdr:
            return "sdr";
        case HdrKind::kHdr10:
            return "hdr10";
        case HdrKind::kHlg:
            return "hlg";
    }
    return "unknown";
}

}  // namespace kivo::video::mediacodec
