#pragma once

#include <string_view>

namespace kivo::video::mediacodec {

/// 依据 codec 名推断实现来源，作为 API 29+ `isHardwareAccelerated()` 缺失时的回退。
///
/// 返回三者之一：
/// - "software"：AOSP 软实现（c2.android.*、OMX.google.*、*.sw）
/// - "vendor"：厂商实现（qti / qcom / exynos / mtk / kirin）
/// - "unknown"：命名不合约定，无法判断
///
/// 重要：本函数**不**判断是否硬件加速。"vendor" 也可能是厂商的软实现
/// （如 OMX.qti.video.decoder.h263sw）。是否硬解一律以
/// `CodecInventoryRecord::is_hardware` 为准，本函数只描述实现来源。
[[nodiscard]] std::string_view classify_implementation(std::string_view codec_name);

}  // namespace kivo::video::mediacodec
