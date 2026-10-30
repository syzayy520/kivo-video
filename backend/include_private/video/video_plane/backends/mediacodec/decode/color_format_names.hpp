#pragma once

#include <cstdint>
#include <string>

namespace kivo::video::mediacodec {

/// 解码器输出颜色格式的可读名与位深。
///
/// **为什么需要这个文件**：NDK 不导出 `COLOR_Format*` 常量 ——
/// 它们只存在于 Java 的 `MediaCodecInfo`。C++ 侧要用只能自己写数值，
/// 而数值写错不会有任何编译错误，只会让日志和证据说瞎话。
/// 所以数值集中在这里一处，并由单测钉住（见 `p12e1_color_tests`）。
///
/// 数值来源：AOSP `MediaCodecInfo.java`（API 21 起稳定，未变过）。
namespace color_format {

/// `COLOR_FormatYUV420Planar` —— I420，8 bit，三平面。
inline constexpr std::int32_t kYuv420Planar = 19;
/// `COLOR_FormatYUV420SemiPlanar` —— NV12，8 bit，两平面。
inline constexpr std::int32_t kYuv420SemiPlanar = 21;
/// `COLOR_FormatYUVP010` —— P010，**10 bit**，两平面（高位在 16bit 字里）。
/// 这张卡（P12-E1）问的就是它能不能协商到。
inline constexpr std::int32_t kYuvP010 = 54;
/// `COLOR_FormatSurface` —— 输出直送 Surface，**不携带位深**。
inline constexpr std::int32_t kSurface = 0x7F000789;
/// `COLOR_FormatYUV420Flexible` —— 由 `getOutputImage()` 再问具体排布。
inline constexpr std::int32_t kYuv420Flexible = 0x7F420888;

/// 厂商私有格式区（QCOM 占 `0x7FA30C00`–`0x7FA30CFF`）。
///
/// **实测（K40 / Android 13）**：8 bit 的 AVC 片源与 10 bit 的 HEVC 片源
/// 在 Surface 输出下**给出完全相同的 `0x7FA30C06`**。也就是说这个值
/// 连"8bit 还是 10bit"都不区分，更谈不上"确认拿到了 10bit"。
/// 具体排布（tile 与否、平面数）没有权威文档，所以只记成"厂商私有"，不猜名字。
inline constexpr std::int32_t kVendorQcomTile = 0x7FA30C06;

/// 是否落在厂商私有区。判定用它来把理由写具体（"厂商私有值不表达位深"），
/// 而不是笼统地说"未知格式"。
[[nodiscard]] bool is_vendor_private(std::int32_t format) noexcept;

/// 可读名。未知值返回 `unknown(0x…)` —— **不返回空串**：
/// 空串在证据里读起来像"没设置"，而未知值恰恰是需要被看见的东西。
[[nodiscard]] std::string format_name(std::int32_t format);

/// 位深：10 / 8 / 0（未知）。`kSurface` 与 `kYuv420Flexible` 都返回 0 ——
/// 它们只说明"输出到哪"或"要再问一层"，不说明位深。
[[nodiscard]] std::int32_t format_bit_depth(std::int32_t format);

}  // namespace color_format
}  // namespace kivo::video::mediacodec
