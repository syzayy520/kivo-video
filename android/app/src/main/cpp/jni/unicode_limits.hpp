#pragma once

#include <cstdint>

// Unicode 边界与替换字符（250 D8）。
//
// 单独成文件的理由：两个转换方向都要用这组常量。各写一份就会出现"一边改了、
// 另一边没改"的真相分裂 —— 这种分裂在编码问题里最难查，因为两边单看都对。

namespace kivo::jni_bridge {

/** 替换字符 U+FFFD，UTF-16 侧表示。 */
constexpr char16_t kReplacementUnit = u'\uFFFD';

/** 替换字符 U+FFFD，码点表示。 */
constexpr std::uint32_t kReplacementCodePoint = 0xFFFD;

/** Unicode 码点上限。 */
constexpr std::uint32_t kMaxCodePoint = 0x10FFFF;

/** 代理区首尾。落在这里的码点不是合法字符，UTF-16 的代理对另有含义。 */
constexpr std::uint32_t kSurrogateFirst = 0xD800;
constexpr std::uint32_t kSurrogateLast = 0xDFFF;

}  // namespace kivo::jni_bridge
