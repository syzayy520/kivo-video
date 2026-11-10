#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "unicode_limits.hpp"

// UTF-16 → 标准 UTF-8（250 D8）。纯算法：不依赖 JNI、不抛异常。
//
// 这是 **Java → libsmb2** 的方向：Kotlin 传下来的路径先被 JNI 编成 UTF-16，
// 再由这里编成标准 UTF-8 交给 `smb2_open`。
//
// 为什么不能用 `GetStringUTFChars`：它给的是 modified UTF-8 —— BMP 外的字符
// （emoji、部分符号）会被编成 CESU-8 的六字节代理对，服务器按标准 UTF-8 解码
// 就是另外两个字，于是"文件明明在那儿却打不开"。
//
// 宽容策略：孤立代理项替换为 U+FFFD。宁可显示一个 �，也不要产出非法字节流。

namespace kivo::jni_bridge {

namespace utf16_detail {

/// 码点 → UTF-8。1/2/3/4 字节四档，按码点区间选。
inline void append_utf8(std::string& out, std::uint32_t cp) {
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
        return;
    }
    if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        return;
    }
    if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        return;
    }
    out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
}

}  // namespace utf16_detail

/// UTF-16 → 标准 UTF-8。孤立代理项替换为 U+FFFD。
[[nodiscard]] inline std::string utf16_to_utf8(std::u16string_view utf16) {
    std::string out;
    out.reserve(utf16.size());
    std::size_t i = 0;
    while (i < utf16.size()) {
        const char16_t unit = utf16[i];
        if (unit >= 0xD800 && unit <= 0xDBFF && i + 1 < utf16.size()) {
            const char16_t low = utf16[i + 1];
            if (low >= 0xDC00 && low <= 0xDFFF) {
                // 合法代理对：合成 BMP 外码点，按四字节编码。
                const std::uint32_t cp = 0x10000 +
                    ((static_cast<std::uint32_t>(unit) - 0xD800) << 10) +
                    (static_cast<std::uint32_t>(low) - 0xDC00);
                utf16_detail::append_utf8(out, cp);
                i += 2;
                continue;
            }
        }
        if (unit >= 0xD800 && unit <= 0xDFFF) {
            // 孤立高代理 / 任何低代理：没有配对的另一半，只能是坏数据。
            utf16_detail::append_utf8(out, kReplacementCodePoint);
        } else {
            utf16_detail::append_utf8(out, unit);
        }
        ++i;
    }
    return out;
}

}  // namespace kivo::jni_bridge
