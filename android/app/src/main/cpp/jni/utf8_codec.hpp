#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "unicode_limits.hpp"

// 标准 UTF-8 → UTF-16（250 D8）。纯算法：不依赖 JNI、不抛异常。
//
// 这是 **libsmb2 → Java** 的方向：SMB 协议里的文件名是标准 UTF-8，
// 而 JNI 的 `NewStringUTF` 要的是 modified UTF-8（BMP 外字符编成 CESU-8 代理对）。
// 直接把标准 UTF-8 喂给 `NewStringUTF`，含 emoji 的文件名在 CheckJNI 下会直接 abort。
// 中间隔一层真解码，Java 侧就永远拿到合法的 UTF-16。
//
// 宽容策略：非法序列替换成 U+FFFD，不抛、不截断。文件名里有一个坏字节，
// 不该让整个目录列举失败 —— 用户要的是"看到其余文件"，不是"收到一个异常"。

namespace kivo::jni_bridge {

namespace utf8_detail {

/// 从 [in] 的 [i] 处解一个码点，成功则前移并返回 true；失败只前移 1 字节。
///
/// 拒绝四类非法编码：非法首字节、缺续字节、overlong、代理区/越界码点。
/// 失败只前移 1 字节而不是跳过"看起来的长度"，是为了让后续字节有机会重新同步 ——
/// 跳过整段会把后面可能合法的字符一起吃掉。
inline bool decode_one(std::string_view in, std::size_t& i, std::uint32_t& cp) {
    const auto lead = static_cast<unsigned char>(in[i]);
    if (lead < 0x80) {
        cp = lead;
        ++i;
        return true;
    }

    int extra = 0;
    std::uint32_t value = 0;
    std::uint32_t minimum = 0;
    if ((lead & 0xE0) == 0xC0) {
        extra = 1;
        value = lead & 0x1Fu;
        minimum = 0x80;
    } else if ((lead & 0xF0) == 0xE0) {
        extra = 2;
        value = lead & 0x0Fu;
        minimum = 0x800;
    } else if ((lead & 0xF8) == 0xF0) {
        extra = 3;
        value = lead & 0x07u;
        minimum = 0x10000;
    } else {
        ++i;
        return false;
    }

    if (i + static_cast<std::size_t>(extra) >= in.size()) {
        ++i;
        return false;
    }
    for (int k = 1; k <= extra; ++k) {
        const auto cont = static_cast<unsigned char>(in[i + static_cast<std::size_t>(k)]);
        if ((cont & 0xC0) != 0x80) {
            ++i;
            return false;
        }
        value = (value << 6) | (cont & 0x3Fu);
    }
    if (value < minimum || value > kMaxCodePoint ||
        (value >= kSurrogateFirst && value <= kSurrogateLast)) {
        ++i;
        return false;
    }
    cp = value;
    i += static_cast<std::size_t>(extra) + 1;
    return true;
}

/// 码点 → UTF-16，BMP 外拆成代理对。
inline void append_utf16(std::u16string& out, std::uint32_t cp) {
    if (cp <= 0xFFFF) {
        out.push_back(static_cast<char16_t>(cp));
        return;
    }
    const std::uint32_t offset = cp - 0x10000;
    out.push_back(static_cast<char16_t>(0xD800 + (offset >> 10)));
    out.push_back(static_cast<char16_t>(0xDC00 + (offset & 0x3FF)));
}

}  // namespace utf8_detail

/// 标准 UTF-8 → UTF-16。非法字节替换为 U+FFFD。
[[nodiscard]] inline std::u16string utf8_to_utf16(std::string_view utf8) {
    std::u16string out;
    out.reserve(utf8.size());
    std::size_t i = 0;
    while (i < utf8.size()) {
        std::uint32_t cp = 0;
        if (utf8_detail::decode_one(utf8, i, cp)) {
            utf8_detail::append_utf16(out, cp);
        } else {
            out.push_back(kReplacementUnit);
        }
    }
    return out;
}

}  // namespace kivo::jni_bridge
