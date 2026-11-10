#pragma once

#include <string>
#include <string_view>

// JSON 字符串内容转义（250 D8）。
//
// 为什么单独成文件：这段逻辑原先内联在 `jni_smb2_bridge.cxx` 的两个地方，
// 而且只转义了 `"` 和 `\` —— 文件名里出现换行、制表符或其他控制字符时，
// 拼出来的 JSON 直接非法，`JSONArray()` 抛异常，用户看到的是"无法读取文件夹"。
// 一个字节就把整个目录废掉，而这条路径以前没有任何测试能覆盖到。
//
// 只做"字符串内容"的转义（不含两端引号）：调用方自己加引号，
// 这样同一段逻辑既能用于 `"name":"..."` 也能用于数组元素。

namespace kivo::jni_bridge {

/**
 * 把 [text] 作为 JSON 字符串内容追加到 [out]。
 *
 * 覆盖 RFC 8259 要求的全部必须转义项：
 * - `"` 与 `\`；
 * - 五个具名短转义（`\b \f \n \r \t`）；
 * - 其余所有 `< 0x20` 的控制字符 → `\u00XX`。
 *
 * 非 ASCII 字节原样透传：JSON 允许 UTF-8 直接出现，而且在这里解码一次、
 * 在 JNI 层再解码一次只会多一处可能出错的地方。非法 UTF-8 序列由
 * `utf8_codec` 在交出去的时候替换成 U+FFFD，不在这里丢信息。
 */
inline void append_json_string(std::string& out, std::string_view text) {
    static constexpr char kHex[] = "0123456789abcdef";
    for (const char raw : text) {
        const auto byte = static_cast<unsigned char>(raw);
        switch (byte) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\b':
                out += "\\b";
                break;
            case '\f':
                out += "\\f";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (byte < 0x20) {
                    out += "\\u00";
                    out += kHex[(byte >> 4) & 0x0F];
                    out += kHex[byte & 0x0F];
                } else {
                    out += raw;
                }
                break;
        }
    }
}

}  // namespace kivo::jni_bridge
